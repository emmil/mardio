#ifndef	_SND_H_
#define	_SND_H_

enum sound_state_e {
	SS_UNKNOWN = 0,
	SS_INIT,
	SS_PLAYING,
	SS_PAUSE,
	SS_ERROR
};

enum sound_rate_e {
	SR_UNKNOWN = 0,
	SR_8000_HZ,
	SR_11025_HZ,
	SR_12000_HZ,
	SR_16000_HZ,
	SR_22050_HZ,
	SR_24000_HZ,
	SR_32000_HZ,
	SR_44100_HZ,
	SR_48000_HZ
};

void snd_init(void);
void snd_done(void);
void snd_loop(void);
enum sound_state_e snd_state(void);

uint16_t snd_rate2freq(enum sound_rate_e rate);
enum sound_rate_e snd_freq2rate(uint16_t freq);
void pcm_apply_volume(int16_t * samples, uint32_t count, uint16_t volume);

#endif
