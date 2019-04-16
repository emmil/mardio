#include <alsa/asoundlib.h>

#include "libc.h"
#include "iobuffer.h"
#include "platform.h"
#include "player.h"

#ifdef	CONFIG_SOUND_DEBUG
#define	s_debug(...)	xprintf(__VA_ARGS__)
#else
#define	s_debug(...)
#endif

struct {
	enum sound_state_e	state;
	enum sound_rate_e	rate;
	int			channels;
	snd_pcm_t		*handle;
} static ss;

const char device[] = "default";

///////////////////////////////////////

void pcm_state(snd_pcm_state_t state) {

	switch (state) {
	case SND_PCM_STATE_OPEN:
		xprintf("Open");
		break;
	case SND_PCM_STATE_SETUP:
		xprintf("Setup installed");
		break;
	case SND_PCM_STATE_PREPARED:
		xprintf("Ready to start");
		break;
	case SND_PCM_STATE_RUNNING:
		xprintf("Running");
		break;
	case SND_PCM_STATE_XRUN:
		xprintf("Stopped: underrun (playback) or overrun (capture) detected");
		break;
	case SND_PCM_STATE_DRAINING:
		xprintf("Draining: running (playback) or stopped (capture)");
		break;
	case SND_PCM_STATE_PAUSED:
		xprintf("Paused");
		break;
	case SND_PCM_STATE_SUSPENDED:
		xprintf("Hardware is suspended");
		break;
	case SND_PCM_STATE_DISCONNECTED:
		xprintf("Hardware is disconnected");
		break;
	case SND_PCM_STATE_PRIVATE1:
		xprintf("Private - used internally in the library");
		break;
	}
}

static void alsa_setup(enum sound_rate_e new_rate, int new_channels) {
	unsigned int	freq = snd_rate2freq(new_rate);
	int		err;

	s_debug("alsa_setup (%d, %d)\n", freq, new_channels);

	ss.channels = new_channels;
	ss.rate = new_rate;

	err = snd_pcm_set_params (ss.handle,
		SND_PCM_FORMAT_S16,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		ss.channels,
		freq,
		1,
		500000);

	if (err < 0) {
		ss.state = SS_ERROR;
		xprintf ("alsa_setup: error setting new alsa parameters (freq %d, channels %d)\n",
			freq, ss.channels);
	}
}

///////////////////////////////////////
// Platform callbacks

void host_snd_init(void) {
	int	err;

	s_debug("%s: alsa version: %s\n", __func__, SND_LIB_VERSION_STR);

	uc_memset(&ss, 0, sizeof(ss));

	err = snd_pcm_open (&ss.handle,
			device,
			SND_PCM_STREAM_PLAYBACK,
			SND_PCM_ASYNC);

	if (err < 0) {
		xprintf("   alsa initialization failed: %d.\n", err);
		ss.state = SS_ERROR;
	} else {
		ss.state = SS_INIT;
	}
}


void host_snd_done(void) {

	snd_pcm_drop(ss.handle);
	snd_pcm_close(ss.handle);

	s_debug("%s\n", __func__);
}

void host_snd_poll(void) {
	snd_pcm_sframes_t	frames = 0;
	struct output_s		*buff = NULL;
	uint32_t		samples;
	uint8_t			volume = pl_volume_get();

	buff = o_pop();

	if (buff == NULL) {
		xprintf("%s: buff is null!\n", __func__);
		ss.state = SS_ERROR;
		return;
	}
	// a bogus frame?
	if (buff->rate == 0 || buff->channels == 0 || buff->size == 0) {

		s_debug ("%s: bogus frame (rate: %d, channels: %d, size: %d)\n",
			__func__, buff->rate, buff->channels, buff->size);

		if (ss.state == SS_PLAYING) {
			snd_pcm_drain(ss.handle);
			ss.state = SS_PAUSE;
		}
		return;
	}

	if (buff->rate != ss.rate || buff->channels != ss.channels)
		alsa_setup(buff->rate, buff->channels);

	// Is size of PCM sample always 16 bits?
	pcm_apply_volume((int16_t *) buff->data, (buff->used / 2), volume);

	// resuming audio device
	if (ss.state == SS_PAUSE) {
		snd_pcm_prepare(ss.handle);
	}

	samples = (buff->used / (buff->channels * 2));
	frames = snd_pcm_writei(ss.handle, buff->data, samples);

	ss.state = SS_PLAYING;

	// something wrong?
	if (frames < 0) {
		xprintf ("%s: error writing to sound device (frames = %d) %s\n",
			__func__, frames, snd_strerror (frames));
		ss.state = SS_ERROR;
	}
}

void host_snd_loop(void) {

	do {
		if (ss.state == SS_ERROR)
			return;

		if (o_used() == 0) {
			// suspending when there is no data
			if (ss.state == SS_PLAYING) {
				snd_pcm_drain(ss.handle);
				ss.state = SS_PAUSE;
				s_debug("%s: suspending sound device.\n",
					__func__);
			}
			return;
		}


		if (pl_state() != PL_PLAYING)
			return;

		host_snd_poll();

	} while (o_used() != 0);
}

enum sound_state_e host_snd_state(void) {
	return (ss.state);
}

///////////////////////////////////////
// Command line stuff

void cmd_snd_state(void) {
	uint16_t	freq;

	xprintf("snd_state: ");
	switch (ss.state) {
	case SS_UNKNOWN:
		xprintf("not initialized.\n");
		break;
	case SS_INIT:
		xprintf("initialized.\n");
		break;
	case SS_PLAYING:
		xprintf("playing.\n");
		break;
	case SS_PAUSE:
		xprintf("paused.\n");
		break;
	case SS_ERROR:
		xprintf("sum ting wong.\n");
		break;
	}

	xprintf("pcm_state: ");
	pcm_state(snd_pcm_state(ss.handle));
	xprintf("\n");


	xprintf("snd_rate: ");
	freq = snd_rate2freq(ss.rate);
	switch (freq) {
	case 0:
		xprintf("not set yet.\n");
		break;
	default:
		xprintf("%dHz\n", freq);
	}

	xprintf("channels: %d\n", ss.channels);
	xprintf("volume: %d\n", pl_volume_get());

	xprintf("\n");
}
