#ifndef	PLAYER_H
#define	PLAYER_H

#include <stdint.h>

#define	PL_MIN_VOLUME	0
#define	PL_MAX_VOLUME	16
#define	PL_DEF_VOLUME	8

enum pl_state_e {
	PL_UNKNOWN = 0,
	PL_STOP,
	PL_PLAYING,
	PL_BUFFERING,
};

enum pl_source_e {
	PS_UNKNOWN = 0,
	PS_NET,
	PS_FILE,
};

void pl_init(void);
void pl_play(const char *what);
void pl_stop(void);
void pl_poll(void);
const char *pl_what(void);
enum pl_state_e pl_state(void);
enum pl_source_e pl_source(void);

uint8_t pl_volume_get(void);
void pl_volume_set(uint8_t new_volume);
void pl_volume_up(void);
void pl_volume_down(void);

#endif
