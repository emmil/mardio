#include <stdint.h>
#include <stdio.h>

#include "platform.h"

void snd_init(void) {
	host_snd_init();
}

void snd_done(void) {
	host_snd_done();
}

void snd_loop(void) {
	host_snd_loop();
}

enum sound_state_e snd_state(void) {
	return host_snd_state();
}

uint16_t snd_rate2freq(enum sound_rate_e rate) {
	uint16_t	freq = 0;

	switch (rate) {
	case SR_UNKNOWN:
		freq = 0;
		break;
	case SR_8000_HZ:
		freq = 8000;
		break;
	case SR_11025_HZ:
		freq = 11025;
		break;
	case SR_12000_HZ:
		freq = 12000;
		break;
	case SR_16000_HZ:
		freq = 16000;
		break;
	case SR_22050_HZ:
		freq = 22050;
		break;
	case SR_24000_HZ:
		freq = 24000;
		break;
	case SR_32000_HZ:
		freq = 32000;
		break;
	case SR_44100_HZ:
		freq = 44100;
		break;
	case SR_48000_HZ:
		freq = 48000;
		break;
	default:
		freq = 0;
	}

	return freq;
}

enum sound_rate_e snd_freq2rate(uint16_t freq) {
	enum sound_rate_e	rate = SR_UNKNOWN;

	switch (freq) {
	case 8000:
		rate = SR_8000_HZ;
		break;
	case 11025:
		rate = SR_11025_HZ;
		break;
	case 12000:
		rate = SR_12000_HZ;
		break;
	case 16000:
		rate = SR_16000_HZ;
		break;
	case 22050:
		rate = SR_22050_HZ;
		break;
	case 24000:
		rate = SR_24000_HZ;
		break;
	case 32000:
		rate = SR_32000_HZ;
		break;
	case 44100:
		rate = SR_44100_HZ;
		break;
	case 48000:
		rate = SR_48000_HZ;
		break;
	default:
		rate = SR_UNKNOWN;
	}

	return rate;
}

void pcm_apply_volume(int16_t * samples, uint32_t count, uint16_t volume) {
	uint32_t	index;
	int16_t		*psample = samples;
	int16_t		sample;

	if (samples == NULL || count == 0)
		return;

	for (index = 0; index < count; index++) {
		sample = psample[index];
//		sample = sample >> volume;
		sample = ((sample * volume * volume) >> 8);
		psample[index] = sample;
	}
}

