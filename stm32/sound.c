#include "stm32f4_discovery.h"

#include "libc.h"
#include "iobuffer.h"
#include "misc.h"
#include "cmd.h"
#include "player.h"
#include "platform.h"
#include "stm32.h"

#ifdef	CONFIG_SOUND_DEBUG
#define	s_debug(...)	xprintf(__VA_ARGS__)
#else
#define	s_debug(...)
#endif

#include "Audio.h"

struct {
	enum sound_state_e	state;
	enum sound_rate_e	rate;
	int			channels;
} static ss;


///////////////////////////////////////

static void audio_setup(enum sound_rate_e new_rate, int new_channels) {
	unsigned int	freq = snd_rate2freq(new_rate);
	int		set = 1;

	s_debug("%s (%d, %d)\n", __func__, freq, new_channels);

	switch (new_rate) {

	case SR_8000_HZ:
		InitializeAudio(Audio8000HzSettings);
		break;

	case SR_16000_HZ:
		InitializeAudio(Audio16000HzSettings);
		break;

	case SR_22050_HZ:
		InitializeAudio(Audio22050HzSettings);
		break;

	case SR_32000_HZ:
		InitializeAudio(Audio32000HzSettings);
		break;

	case SR_44100_HZ:
		InitializeAudio(Audio44100HzSettings);
		break;

	case SR_48000_HZ:
		InitializeAudio(Audio48000HzSettings);
		break;

	default:
		xprintf("%s: Unsupported frequency %dHz\n", __func__, freq);
		set = 0;
	}

	if (set) {
		ss.channels = new_channels;
		ss.rate = new_rate;
	}

}

void audio_callback(void *context, int buffer) {

	if (buffer) {
		STM_EVAL_LEDOn(LED_SOUND);
	} else {
		STM_EVAL_LEDOff(LED_SOUND);
	}

}

void host_snd_init(void) {

	s_debug("%s\n\t", __func__);

	uc_memset(&ss, 0, sizeof(ss));

	audio_setup(SR_44100_HZ, 2);

	SetAudioVolume(255);

	PlayAudioWithCallback(audio_callback, 0);

	ss.state = SS_INIT;
}

void host_snd_done(void) {
}

void host_snd_poll(void) {
	uint8_t		volume = pl_volume_get();
	uint32_t	samples;
	struct		output_s *buff = NULL;

	buff = o_pop();

	if (buff == NULL) {
		xprintf("%s: buff is null!\n", __func__);
		ss.state = SS_ERROR;
		return;
	}
	// a bogus frame?
	if (buff->rate == 0 || buff->channels == 0 || buff->size == 0) {
		s_debug("%s: bogus frame (rate: %d, channels: %d, size: %d)\n",
		     __func__, buff->rate, buff->channels, buff->size);
		return;
	}

	if (buff->rate != ss.rate || buff->channels != ss.channels)
		audio_setup(buff->rate, buff->channels);

	if (ss.state == SS_ERROR)
		return;

	if (ss.state == SS_PAUSE)
		AudioOn();

	pcm_apply_volume((int16_t *) buff->data, (buff->used / 2), volume);

	samples = (buff->used / (buff->channels));
	ProvideAudioBuffer((void *) buff->data, samples);

	ss.state = SS_PLAYING;
}

void host_snd_loop(void) {

	do {

		if (ss.state == SS_ERROR)
			return;

		if (o_used() == 0) {
			// suspending when there is no data
			if (ss.state == SS_PLAYING) {
				ss.state = SS_PAUSE;
				//AudioOff();
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
