#ifndef	LIBMAD_H
#define	LIBMAD_H

enum libmad_state_e {
	LM_UNKNOWN = 0,
	LM_INIT,
	LM_DONE,
	LM_ERROR,
};

void libmad_init(void);
void libmad_done(void);
void libmad_loop(void);
enum libmad_state_e libmad_state(void);

#endif
