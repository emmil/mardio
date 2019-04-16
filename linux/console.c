#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "libc.h"
#include "console.h"
#include "event.h"

#define	POLL_INTERVAL	20	// 20 milliseconds
#define	STDIN		1

static struct pollfd	fds;

void host_con_init(void) {
	int	flags;

	fds.fd = STDIN;
	fds.events = POLLIN | POLLERR;

	flags = fcntl(STDIN, F_GETFL, 0);
	fcntl(STDIN, F_SETFL, flags | O_NONBLOCK);

	xdev_out(putchar);
}

void host_con_poll(void) {
	int	ret;
	int	ch;

	ret = poll(&fds, 1, POLL_INTERVAL);

	if (ret) {
		do {
			ret = read(STDIN, &ch, 1);

			if (ret > 0 && !con_input_islocked()) {
				/* Read for some reason now modifies the high half of
				 * 16bit variable, even if reading just 8 bits */
				if ((ch & 0xFF) == '\n') {	// \n
					con_input_lock();
					event_push(E_CON_INPUT, 0, NULL);
				}
				con_input_push(ch);
			}

		} while (ret > 0);
	}
}
