#ifndef	_NET_H_
#define	_NET_H_

#include <stdint.h>

#include "console.h"

#define	MAX_HOSTNAME	MAX_CONSOLE_INPUT
#define	MAX_URL		MAX_CONSOLE_INPUT
#define	HTTP_URI	"http://"

enum net_state_e {
	NET_UNKNOWN = 0,
	NET_PENDING,
	NET_OPEN,
	NET_CLOSED,
	NET_ERROR,
};

void net_init(void);
void net_done(void);
void net_open(const char *url);
void net_close(void);
void net_poll(void);
enum net_state_e net_state(void);

void net_parse_url(const char *address, char *hostname, int *port, char *remote);
int net_str2ip(const char *str, uint32_t * ip);

#endif
