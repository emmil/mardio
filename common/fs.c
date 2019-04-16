#include "fs.h"
#include "platform.h"
#include "libc.h"
#include "cmd.h"
#include "id3tag.h"
#include "iobuffer.h"
#include "player.h"

#include "resfs.h"

#ifdef	CONFIG_FS_DEBUG
#define	fs_debug(...)	xprintf(__VA_ARGS__)
#else
#define	fs_debug(...)
#endif

#define RIFF_SIGNATURE		('R' << 0  | 'I' << 8 | 'F' << 16 | 'F' << 24)
#define RIFF_FORMAT		('W' << 0  | 'A' << 8 | 'V' << 16 | 'E' << 24)
#define RIFF_FMT		('f' << 0  | 'm' << 8 | 't' << 16 | ' ' << 24)
#define RIFF_DATA		('d' << 0  | 'a' << 8 | 't' << 16 | 'a' << 24)
#define RIFF_FACT		('f' << 0  | 'a' << 8 | 'c' << 16 | 't' << 24)

#define	RIFF_FMT_PCM		0x0001
#define	RIFF_FMT_GSM		0x0031

enum fssource_e {
	FSO_CLOSED = 0,
	FSO_RESOURCE,
	FSO_HOST,
};

struct riff_header_s {
	uint32_t signature;
	uint32_t size;
	uint32_t format;
};

struct riff_block_s {
	uint32_t signature;
	uint32_t size;
};

struct riff_fmt_s {
	uint16_t format;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t byterate;
	uint16_t blockalign;
	uint16_t bitspersample;
};

struct riff_fact_s {
	uint32_t samplelength;
};

struct fshandle_s {
	enum fssource_e	source;
	int		handle;
};

struct {
	enum fs_state_e		state;
	enum fs_type_e		type;
	enum sound_rate_e	rate;
	int			channels;
	int			handle;				// for the file that is being played (polled)
	struct fshandle_s	handles[MAX_OPEN_FILES];	// for files that can be opened, not played
} static fss;

///////////////////////////////////////

void fs_init(void) {

	fs_debug("fs_init\n");

	uc_memset(&fss, 0, sizeof(fss));

	host_fs_init();
	res_init();

	fss.state = FSS_CLOSED;
	fss.handle = -1;
}

void fs_done(void) {

	fs_stop();
	res_done();

	fs_debug("fs_done\n");
}

int fs_get_handle(void) {
	int	i;

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (fss.handles[i].source == FSO_CLOSED)
			return i;
	}

	return -1;
}

int fs_open(const char *name) {
	int	fs_handle = -1;
	int	host_handle = -1;

	fs_debug("fs_open (name = '%s')\n", name);

	if (name == NULL)
		goto error;

	fs_handle = fs_get_handle();
	if (fs_handle == -1) {
		fs_debug("fs_open: Maximum of open files reached (%d)\n",
			 MAX_OPEN_FILES);
		goto error;
	}
	// try host FS first
	host_handle = host_fs_open(name);
	if (host_handle != -1) {
		fss.handles[fs_handle].handle = host_handle;
		fss.handles[fs_handle].source = FSO_HOST;
		goto found;
	}
	// then resource FS
	host_handle = res_open(name);
	if (host_handle != -1) {
		fss.handles[fs_handle].handle = host_handle;
		fss.handles[fs_handle].source = FSO_RESOURCE;
		goto found;
	}

error:
	return -1;

found:
	return fs_handle;

}

void fs_close(int handle) {

	fs_debug("fs_close (handle = %d)\n", handle);

	if (handle < 0 || handle > MAX_OPEN_FILES)
		return;

	switch (fss.handles[handle].source) {

	case FSO_HOST:
		host_fs_close(fss.handles[handle].handle);
		fss.handles[handle].source = FSO_CLOSED;
		break;

	case FSO_RESOURCE:
		res_close(fss.handles[handle].handle);
		fss.handles[handle].source = FSO_CLOSED;
		break;

	default:
		xprintf
		    ("fs_close: someone closing a not open file (handle = %d)?\n",
		     handle);
	}
}

uint32_t fs_read(int handle, void *buffer, uint32_t size) {
	int	got = -1;

	/*
	fs_debug ("fs_read (handle = %d, buffer = 0x%x, size = %d)\n",
			handle, buffer, size);
	*/

	if (handle < 0 || handle > MAX_OPEN_FILES)
		goto exit;

	if (size == 0 || buffer == NULL)
		goto exit;

	switch (fss.handles[handle].source) {

	case FSO_HOST:
		got = host_fs_read(fss.handles[handle].handle, buffer, size);
		break;

	case FSO_RESOURCE:
		got = res_read(fss.handles[handle].handle, buffer, size);
		break;

	default:
		xprintf
		    ("fs_read: someone reading in a closed file (handle = %d)?\n",
		     handle);
	}

exit:
	return got;
}

void fs_seek(int handle, uint32_t offset) {

	if (handle < 0 || handle > MAX_OPEN_FILES)
		return;

	switch (fss.handles[handle].source) {

	case FSO_HOST:
		host_fs_seek(fss.handles[handle].handle, offset);
		break;

	case FSO_RESOURCE:
		res_seek(fss.handles[handle].handle, offset);
		break;

	default:
		xprintf
		    ("fs_seek: someone seeking in a closed file (handle = %d)?\n",
		     handle);
	}
}

///////////////////////////////////////

enum fs_type_e fs_type(const char *name) {
	enum fs_type_e		fstype = FST_UNKNOWN;
	struct riff_header_s	*header = NULL;
	unsigned char		buff[64];
	int			ret;
	int			f;

	f = fs_open(name);
	if (f == -1) {
		fs_debug("fs_type: Unable to open file '%s'.\n", name);
		fstype = FST_NONE;
		goto exit;
	}

	ret = fs_read(f, (void *) &buff, sizeof(buff));

	fs_close(f);

	if (ret < 44) {
		fs_debug
		    ("fs_type: File '%s' is too small to contain sound data.\n",
		     name);
		fstype = FST_UNKNOWN;
		goto exit;
	}

	header = (struct riff_header_s *) buff;

	if (header->signature == RIFF_SIGNATURE
	    && header->format == RIFF_FORMAT) {
		fstype = FST_RIFF;
		goto exit;
	}

	ret = id3_tag_query(buff, sizeof(buff));
	if (ret != 0) {
		fstype = FST_MP3;
		goto exit;
	}

exit:
	return fstype;
}

void fs_open_riff(const char *name) {
	struct riff_header_s	*header = NULL;
	struct riff_block_s	*block = NULL;
	struct riff_fmt_s	*fmt = NULL;
	struct riff_fact_s	*fact = NULL;
	unsigned char		buff[128];
	void			*ptr;
	uint32_t		ret;
	int			have_fmt = 0;
	enum sound_rate_e	rate;
	uint32_t		dataoff = 0;
	int			handle;

	/* make compiler happy */
	fact = fact;
	header = header;

	handle = fs_open(name);
	if (handle == -1) {
		xprintf("%s: Unable to open file '%s'.\n", __func__);
		goto fail;
	}

	ret = fs_read(handle, (void *) &buff, sizeof(buff));
	if (ret == -1)
		goto fail;

	header = (struct riff_header_s *) buff;

	fs_debug("%s: RIFF header:\n", __func__);
	fs_debug("%s:  size: %d\n", __func__, header->size);

	ptr = buff + sizeof(struct riff_header_s);

try_again:
	block = (struct riff_block_s *) ptr;

	fs_debug("%s: RIFF block (0x%x):\n", __func__,
		 ((void *) ptr - (void *) buff));
	fs_debug("%s:  size: %d (0x%x)\n", __func__, block->size, block->size);
	fs_debug("%s:  signature: 0x%x (%c%c%c%c)\n",
		 __func__,
		 block->signature,
		 ((block->signature >> 0) & 0xFF),
		 ((block->signature >> 8) & 0xFF),
		 ((block->signature >> 16) & 0xFF),
		 ((block->signature >> 24) & 0xFF));

	switch (block->signature) {

	case RIFF_FMT:
		fs_debug("%s:  FMT section:\n", __func__);
		fmt = (struct riff_fmt_s *) (ptr + sizeof(struct riff_block_s));

		fs_debug("%s:    format: %d\n", __func__, fmt->format);
		fs_debug("%s:    channels: %d\n", __func__, fmt->channels);
		fs_debug("%s:    samplerate: %d\n", __func__, fmt->samplerate);
		fs_debug("%s:    byterate: %d\n", __func__, fmt->byterate);
		fs_debug("%s:    blockalign: %d\n", __func__, fmt->blockalign);
		fs_debug("%s:    bitspersample: %d\n", __func__,
			 fmt->bitspersample);

		have_fmt = 1;

		ptr += block->size + sizeof(struct riff_block_s);
		goto try_again;

	case RIFF_FACT:
		fs_debug("%s:  FACT section at offset 0x%x\n", __func__,
			 (void *) ptr - (void *) buff);

		fact = (struct riff_fact_s *) (ptr + sizeof(struct riff_block_s));

		fs_debug("%s:    sample length: %d\n", __func__,
			 fact->samplelength);

		ptr += block->size + sizeof(struct riff_block_s);
		goto try_again;


	case RIFF_DATA:
		fs_debug("%s:  DATA section at offset 0x%x\n", __func__,
			 (void *) ptr - (void *) buff);
		break;

	default:
		xprintf("%s: Unknown section 0x%x\n", __func__,
			block->signature);
		goto fail;
	}

	if (have_fmt == 0) {
		xprintf("%s: FMT section not present in file. Cannot play.\n",
			__func__);
		goto fail;
	}
//      if (fmt->format != RIFF_FMT_PCM && fmt->format != RIFF_FMT_GSM) {
	if (fmt->format != RIFF_FMT_PCM) {
		xprintf("%s: Not a PCM file. Cannot play.\n", __func__);
		goto fail;
	}

	rate = snd_freq2rate(fmt->samplerate);
	if (rate == SR_UNKNOWN) {
		xprintf("%s: Unknown sample rate (%d). Cannot play.\n",
			__func__, fmt->samplerate);
		goto fail;
	}

	xprintf("%s: RIFF WAVE ", __func__);
	switch (fmt->format) {
	case RIFF_FMT_PCM:
		xprintf("Microsoft PCM");
		break;
	case RIFF_FMT_GSM:
		xprintf("GSM 6.10");
		break;
	default:
		xprintf("unknown (0x%x)", fmt->format);
	}

	xprintf(" %s, bitrate: %d, samplerate %dHz\n",
		fmt->channels == 2 ? "stereo" : "mono",
		fmt->byterate, fmt->samplerate);

	fss.rate = rate;
	fss.channels = fmt->channels;
	fss.type = FST_RIFF;
	fss.state = FSS_OPEN;
	fss.handle = handle;

	dataoff = ((void *) ptr - (void *) buff);
	fs_debug("%s: Seeking file to offset %d (0x%x).\n",
		__func__,
		dataoff, dataoff);
	fs_seek(handle, dataoff);

	return;

fail:
	fs_stop();
}

static void fs_poll_riff(void) {
	struct output_s	*out;
	uint32_t	got;
	int		free = o_free();

	if (!free)
		return;

	out = o_push();
	got = fs_read(fss.handle, out->data, out->size);

	if (got == -1) {
		xprintf("fs_poll_riff: Error reading file.\n");
		goto close;
	}
	if (got == 0) {
		fs_debug("fs_poll_riff: File finished.\n");
		goto close;
	} else {
		out->used = got;
		out->rate = fss.rate;
		out->channels = fss.channels;
	}

	return;

close:
	fs_stop();
}


void fs_open_mp3(const char *name) {
	uint32_t	ret;

	ret = fs_open(name);
	if (ret == -1) {
		xprintf("fs_open_mp3: Unable to open file '%s'.\n");
		goto fail;
	}

	fss.type = FST_MP3;
	fss.state = FSS_OPEN;
	fss.handle = ret;

	return;

fail:
	fs_stop();
}

static void fs_poll_mp3(void) {
	struct input_s	*in;
	uint32_t	got;
	int		free = i_free();

	if (!free)
		return;

	in = i_push();
	got = fs_read(fss.handle, in->data, in->size);

	if (got == -1) {
		xprintf("fs_poll_mp3: Error reading file.\n");
		goto close;
	}
	if (got == 0) {
		fs_debug("fs_poll_mp3: File finished.\n");
		goto close;
	}

	in->used = got;

	return;

close:
	fs_stop();
}

void fs_play(const char *name) {
	enum fs_type_e	type;

	type = fs_type(name);

	fs_debug("fs_play: Detect file: ");
	switch (type) {
	case FST_RIFF:
		fs_debug("RIFF/WAVE\n");
		fs_open_riff(name);
		break;
	case FST_MP3:
		fs_debug("MP3\n");
		fs_open_mp3(name);
		break;
	case FST_NONE:
		xprintf("File '%s' does not exist.\n", name);
		break;
	default:
		xprintf("Type of file '%s' is not known.\n", name);
	}
}

void fs_poll(void) {

	if (fss.handle == -1)
		return;

	switch (fss.type) {
	case FST_RIFF:
		fs_poll_riff();
		break;
	case FST_MP3:
		fs_poll_mp3();
		break;
	default:
		break;
	}
}

void fs_stop(void) {

	fs_debug("fs_stop\n");

	if (fss.handle != -1) {
		fs_close(fss.handle);
		fss.handle = -1;
		fss.state = FSS_CLOSED;
	}

}

enum fs_state_e fs_state(void) {
	return (fss.state);
}

enum fs_type_e fs_playing(void) {
	return (fss.type);
}

///////////////////////////////////////
// Command line interface

void cmd_ftype(void) {
	enum fs_type_e	ftype;

	if (cmd_Argc() != 2) {
		xprintf("Enter file name to determine file type.\n\n");
		return;
	}

	ftype = fs_type(cmd_Argv(1));
	switch (ftype) {
	case FST_MP3:
		xprintf("MP3\n");
		break;
	case FST_RIFF:
		xprintf("RIFF Wave.\n");
		break;
	default:
		xprintf("unknown (%d).\n", ftype);
		break;
	}

	xprintf("\n");
}

void cmd_fplay(void) {

	if (cmd_Argc() != 2) {
		xprintf("Enter file name to play file.\n\n");
		return;
	}

	fs_play(cmd_Argv(1));

	xprintf("\n");
}

void cmd_fstate(void) {
	int	i;

	xprintf("Played file is ");
	switch (fss.state) {

	case FSS_UNKNOWN:
		xprintf("in unknown state??\n");
		goto rest_fs;

	case FSS_CLOSED:
		xprintf("closed.\n");
		goto rest_fs;

	case FSS_ERROR:
		xprintf("in terrible plain.\n");
		goto rest_fs;

	case FSS_OPEN:
		xprintf("playing ");
	}

	switch (fss.type) {

	case FST_UNKNOWN:
		xprintf("unknown type??\n");
		goto rest_fs;

	case FST_NONE:
		xprintf("none type??\n");
		goto rest_fs;

	case FST_MP3:
		xprintf("MP3 file.\n");
		goto rest_fs;

	case FST_RIFF:
		xprintf("WAV/RIFF file.\n");
	}

rest_fs:
	xprintf("Played file handle is set to %d.\n\n", fss.handle);

	xprintf("Max open files is set to %d.\n", MAX_OPEN_FILES);

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		xprintf("Handle %d is ", i);
		switch (fss.handles[i].source) {
		case FSO_CLOSED:
			xprintf("closed.\n");
			break;
		case FSO_HOST:
			xprintf("open on host fs, handle is %d.\n",
				fss.handles[i].handle);
			break;
		case FSO_RESOURCE:
			xprintf("open on resource fs, handle is %d.\n",
				fss.handles[i].handle);
			break;
		}
	}

	xprintf("\n");
}
