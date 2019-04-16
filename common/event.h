#ifndef	EVENT_H
#define	EVENT_H

#include <stdint.h>

#define	MAX_EVENTS	8

typedef enum {
	E_NONE = 0,
	E_CON_INPUT,
	E_CON_ECHO,
	E_CON_DEL,
	E_PL_STOP,
	E_PL_PLAY,
	E_PL_VOLUP,
	E_PL_VOLDOWN,
	E_PL_VOLSET,
} event_type_e;

struct event_t {
	uint32_t	time;	// time of event
	event_type_e	type;	// type of event
	uint32_t	par;	// parameter
	void		*ptr;	// parameter pointer
};

void event_init(void);
void event_push(event_type_e type, uint32_t par, void *ptr);
void event_loop(void);

#endif
