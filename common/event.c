#include <stdint.h>

#include "event.h"
#include "libc.h"
#include "platform.h"
#include "console.h"
#include "cmd.h"
#include "player.h"

struct {
	struct event_t	events[MAX_EVENTS];
	uint32_t	head;
	uint32_t	tail;
} static ev;


void event_init(void) {
	uc_memset(&ev, 0, sizeof(ev));
}

void event_push(event_type_e type, uint32_t par, void *ptr) {
	struct event_t	*e;

	e = &ev.events[ev.head % MAX_EVENTS];
	if (ev.head - ev.tail > MAX_EVENTS) {
		xprintf("event_push: event overflow\n");
		ev.tail++;
	}

	ev.head++;

	e->time = get_local_time();
	e->type = type;
	e->par = par;
	e->ptr = ptr;
}

struct event_t event_pop(void) {
	struct event_t	e;

	if (ev.head > ev.tail) {
		ev.tail++;
		return ev.events[(ev.tail - 1) % MAX_EVENTS];
	}

	uc_memset(&e, 0, sizeof(e));
	e.type = E_NONE;

	return (e);
}

void event_loop(void) {
	struct event_t	e;

	do {

		e = event_pop();

		switch (e.type) {

		case E_NONE:
			break;

		case E_CON_INPUT:
			cmd_input();
			con_reset();
			con_prompt();
			break;

		case E_CON_ECHO:
			if (e.par != 0)
				xprintf("%c", (char) e.par);
			break;

		case E_CON_DEL:
			xprintf("\b");
			break;

		case E_PL_STOP:
			pl_stop();
			break;

		case E_PL_PLAY:
			pl_play((char *) e.ptr);
			break;

		case E_PL_VOLUP:
			pl_volume_up();
			break;

		case E_PL_VOLDOWN:
			pl_volume_down();
			break;

		case E_PL_VOLSET:
			pl_volume_set(e.par);
			break;

		default:
			xprintf("Unknown event type %d\n", e.type);
		}

	} while (e.type != E_NONE);
}
