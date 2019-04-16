#include <stdint.h>

#include "platform.h"

void host_net_init(void) {
}

void host_net_done(void) {
}

void host_net_open(const char *url) {
}

void host_net_close(void) {
}

void host_net_poll(void) {
}

enum net_state_e host_net_state(void) {
	return NET_CLOSED;
}

/////////// Shell interface ////////////////////

void cmd_net_state(void) {
}
