#include <stdint.h>
#include <stdio.h>

#include "libc.h"
#include "platform.h"

#ifdef	CONFIG_NET_DEBUG
#define	net_debug(...)	xprintf(__VA_ARGS__)
#else
#define	net_debug(...)
#endif

void net_init(void) {
	host_net_init();
}

void net_done(void) {
	host_net_done();
}

void net_open(const char *url) {
	host_net_open(url);
}

void net_close(void) {
	host_net_close();
}

void net_poll(void) {
	host_net_poll();
}

enum net_state_e net_state(void) {
	return host_net_state();
}

void net_parse_url(const char *address, char *hostname, int *port, char *remote) {
	char const	*index = address;
	char const	*old_index;
	size_t		input_str = uc_strlen(address);
	uint16_t	len = 0;

	net_debug("%s: address lenght is %d.\n", __func__, input_str);

	/* skipp URI prefix */
	if (!(uc_memcmp(address, HTTP_URI, uc_strlen(HTTP_URI))))
		index += uc_strlen(HTTP_URI);

	old_index = index;

	/* get hostname */
	while (index[0] != ':' && index[0] != '/'
	       && (index - address) < input_str) {
		index++;
		len++;
	}

	uc_memcpy(hostname, old_index, len);

	/* get port */

	if (index[0] == ':') {

		index++;
		old_index = index;

		while (index[0] >= '0' && index[0] <= '9'
		       && (index - address) < input_str) {
			index++;
			len++;
		}

		*port = q3_atoi(old_index);
	}

	/* get remote file address */
	if ((index - address) < input_str)
		uc_memcpy(remote, index, input_str - (index - address));
}

int net_str2ip(const char *str, uint32_t * ip) {
	const char	*head, *tail;
	char		buff[32];
	int		len;
	uint32_t	tmp_ip = 0;
	uint32_t	tmp_seg = 0;

	if (str == NULL)
		return -1;

	len = uc_strlen(str);
	if (len == 0 || len < 7)
		return -1;

	head = str;
	tail = str;

	while ((head - str) <= len) {
		if (*head == '.' || (head - str == len)) {

			if (head - tail > 3) {
				return -1;
			}

			uc_memset(buff, 0, sizeof(buff));
			uc_memcpy(buff, tail, (head - tail));
			tmp_ip = tmp_ip << 8;
			tmp_seg = q3_atoi(buff);

			if (tmp_seg > 255)
				return -1;

			tmp_ip |= tmp_seg;

			head++;
			tail = head;
		}

		head++;
	}

	*ip = tmp_ip;

	return 0;
}
