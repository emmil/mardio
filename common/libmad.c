#include <mad.h>

#include "libmad.h"
#include "libc.h"
#include "iobuffer.h"
#include "id3tag.h"
#include "player.h"

#define	MAX_LIBMAD_ERRORS	4

#define	LIBMAD_BUFFER_SIZE	((2*1024) + 32)

// cca the ratio between compressed and uncompressed data
#define	DECOMPRESS_SIZE		(LIBMAD_BUFFER_SIZE*8)


#ifdef	CONFIG_LIBMAD_DEBUG
#define	libmad_debug(...)	xprintf(__VA_ARGS__)
#else
#define	libmad_debug(...)
#endif

unsigned char	libmad_buffer[LIBMAD_BUFFER_SIZE + MAD_BUFFER_GUARD];

struct {
	struct mad_stream	Stream;
	struct mad_frame	Frame;
	struct mad_synth	Synth;
	mad_timer_t		Timer;
	long int		Frames;
	int			Banner;
	int			err_count;
	enum libmad_state_e	state;
} static ls;

void libmad_init(void) {

	mad_stream_init(&ls.Stream);
	mad_frame_init(&ls.Frame);
	mad_synth_init(&ls.Synth);
	mad_timer_reset(&ls.Timer);

	uc_memset(libmad_buffer, 0, sizeof(libmad_buffer));

	ls.Frames = 0;
	ls.Banner = 0;
	ls.err_count = 0;
	ls.state = LM_INIT;

	libmad_debug("libmad_init (libmad buffer = %d, DECOMPRESS_SIZE = %d)\n",
		     sizeof(libmad_buffer), DECOMPRESS_SIZE);
}

void libmad_done(void) {

	mad_synth_finish(&ls.Synth);
	mad_frame_finish(&ls.Frame);
	mad_stream_finish(&ls.Stream);

	ls.state = LM_DONE;

	libmad_debug("libmad_done\n");
}

enum libmad_state_e libmad_state(void) {
	return (ls.state);
}

void libmad_headerinfo(struct mad_header const *header) {

	xprintf("Layer: ");
	switch (header->layer) {
	case MAD_LAYER_I:
		xprintf("I");
		break;
	case MAD_LAYER_II:
		xprintf("II");
		break;
	case MAD_LAYER_III:
		xprintf("III");
		break;
	}

	xprintf(", ");
	switch (header->mode) {
	case MAD_MODE_SINGLE_CHANNEL:
		xprintf("single channel");
		break;
	case MAD_MODE_DUAL_CHANNEL:
		xprintf("dual channel");
		break;
	case MAD_MODE_JOINT_STEREO:
		xprintf("joint (MS/intensity) stereo");
		break;
	case MAD_MODE_STEREO:
		xprintf("normal LR stereo");
		break;
	}

	xprintf(", bitrate: %d", header->bitrate);
	xprintf(", samplerate: %d", header->samplerate);
#ifdef	CONFIG_LIBMAD_DEBUG
	xprintf(", flags: 0x%x", header->flags);
#endif
	xprintf("\n");


	ls.Banner = 1;
}

static enum mad_flow libmad_error(struct mad_stream *Stream,
				  struct mad_frame *Frame) {
	int	tagsize;

	switch (Stream->error) {

	case MAD_ERROR_BADCRC:
		mad_frame_mute(Frame);
		return MAD_FLOW_IGNORE;

	case MAD_ERROR_BADDATAPTR:
		return MAD_FLOW_CONTINUE;

	case MAD_ERROR_LOSTSYNC:
		tagsize = id3_tag_query (Stream->this_frame, Stream->bufend - Stream->this_frame);
		if (tagsize) {
			libmad_debug("libmad_error: Skipping tag of size %d.\n",
				     tagsize);
			mad_stream_skip(Stream, tagsize);
		}
		break;

	default:
#ifdef	CONFIG_LIBMAD_DEBUG
		libmad_debug
		    ("libmad_error: decoding error 0x%04x (%s) at byte offset %u\n",
		     Stream->error, mad_stream_errorstr(Stream),
		     Stream->this_frame - libmad_buffer);
		put_dump(Stream->this_frame, 0x0, 16, 1);
		libmad_debug("\n");
#endif
		ls.err_count += 1;

		if (ls.err_count > MAX_LIBMAD_ERRORS)
			ls.state = LM_ERROR;

		break;
	}

	return MAD_FLOW_CONTINUE;
}

static inline signed int scale(mad_fixed_t sample) {

	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	sample = sample >> (MAD_F_FRACBITS + 1 - 16);

	return sample;
}

static
enum mad_flow libmad_input(struct mad_stream *Stream) {
	struct input_s		*buff;
	unsigned int		remain = Stream->bufend - Stream->next_frame;

	if (remain)
		uc_memmove(libmad_buffer, Stream->next_frame, remain);

	buff = i_pop();

	if (buff == NULL) {
		xprintf("libmad_input: got a NULL input buffer.\n");
		return MAD_FLOW_STOP;
	}

	if ((buff->used + remain) > LIBMAD_BUFFER_SIZE) {
		xprintf("libmad_input: overflowing data in %d bytes.\n",
			((buff->used + remain) - LIBMAD_BUFFER_SIZE));
		ls.state = LM_ERROR;
		return MAD_FLOW_STOP;
	}

	uc_memcpy(libmad_buffer + remain, buff->data, buff->used);

	if ((buff->used + remain) == 0) {
		return MAD_FLOW_STOP;
	}

	mad_stream_buffer(Stream, libmad_buffer, buff->used + remain);

	return MAD_FLOW_CONTINUE;
}

void libmad_output(void) {
	struct output_s		*buff;
	mad_fixed_t const	*left_ch, *right_ch;
	signed int		sample;
	int			nsamples;
	int			nchannels;
	register unsigned char	*ptr;

	left_ch = ls.Synth.pcm.samples[0];
	right_ch = ls.Synth.pcm.samples[1];
	nchannels = ls.Synth.pcm.channels;

	buff = o_push();

	ptr = buff->data;
	buff->channels = nchannels;
	buff->used = nchannels * ls.Synth.pcm.length * 2;
	buff->rate = snd_freq2rate(ls.Synth.pcm.samplerate);

	if (buff->used > buff->size) {
		xprintf("libmad_output: snd buffer overflow.\n");
		ls.state = LM_ERROR;
		return;
	}

	for (nsamples = 0; nsamples < ls.Synth.pcm.length; nsamples++) {

		sample = scale(*left_ch++);

		*ptr++ = (unsigned char) (sample >> 0) & 0xFF;
		*ptr++ = (unsigned char) (sample >> 8) & 0xFF;

		if (nchannels == 2) {
			sample = scale(*right_ch++);
			*ptr++ = (unsigned char) (sample >> 0) & 0xFF;
			*ptr++ = (unsigned char) (sample >> 8) & 0xFF;
		}

	}
}

void libmad_loop(void) {
	int	ret;

	if (ls.state != LM_INIT)
		return;

	if (pl_state() == PL_BUFFERING)
		return;

	// if have something to read and where to push
	if (i_used() == 0)
		return;

	if (o_free_size() < DECOMPRESS_SIZE)
		return;

	ret = libmad_input(&ls.Stream);
	switch (ret) {
	case MAD_FLOW_STOP:
		goto done;
	case MAD_FLOW_BREAK:
		goto fail;
	case MAD_FLOW_IGNORE:
		break;
	case MAD_FLOW_CONTINUE:
		break;
	}

	while (1) {

		if (ls.state != LM_INIT)
			goto done;

		ret = mad_header_decode(&ls.Frame.header, &ls.Stream);
		if (ret == -1) {
			if (!MAD_RECOVERABLE(ls.Stream.error))
				break;

			ret = libmad_error(&ls.Stream, &ls.Frame);
			switch (ret) {
			case MAD_FLOW_STOP:
				goto done;
			case MAD_FLOW_BREAK:
				goto fail;
			case MAD_FLOW_IGNORE:
				break;
			case MAD_FLOW_CONTINUE:
			default:
				continue;
			}

		}
#ifdef  CONFIG_LIBMAD_DEBUG
		/* Not everytime the first header contains correct values */
		if (ls.Frames == 4 && ls.Banner == 0)
			libmad_headerinfo(&ls.Frame.header);
#endif

		ret = mad_frame_decode(&ls.Frame, &ls.Stream);
		if (ret == -1) {
			if (!MAD_RECOVERABLE(ls.Stream.error))
				break;

			ret = libmad_error(&ls.Stream, &ls.Frame);
			switch (ret) {
			case MAD_FLOW_STOP:
				goto done;
			case MAD_FLOW_BREAK:
				goto fail;
			case MAD_FLOW_IGNORE:
				break;
			case MAD_FLOW_CONTINUE:
			default:
				continue;
			}
		}

		mad_timer_add(&ls.Timer, ls.Frame.header.duration);

		// add filter here if needed

		mad_synth_frame(&ls.Synth, &ls.Frame);

		libmad_output();

		ls.Frames++;
	}

fail:

done:
	return;
}

/////////// Shell interface ////////////////////

void cmd_libmad_state(void) {
	char	buff[64];

	uc_memset(buff, 0, sizeof(buff));

	mad_timer_string(ls.Timer, buff, "%lu:%02lu.%03u",
			 MAD_UNITS_MINUTES, MAD_UNITS_MILLISECONDS, 0);

	xprintf("Encoded %d frames, %s seconds.\n", ls.Frames, buff);

	xprintf("Libmad is ");
	switch (ls.state) {
	case LM_UNKNOWN:
		xprintf("not initialized, yet");
		break;
	case LM_INIT:
		xprintf("initialized");
		break;
	case LM_DONE:
		xprintf("deinitialized");
		break;
	case LM_ERROR:
		xprintf("in error sate");
		break;
	}

	xprintf(", error count: %d.\n", ls.err_count);

	/* Sometimes the last frame is errorous and might contain bogus */
	if (ls.Frames > 4 && pl_state() == PL_PLAYING)
		libmad_headerinfo(&ls.Frame.header);

	uc_memset(buff, 0, sizeof(buff));

	xprintf("\n");
}
