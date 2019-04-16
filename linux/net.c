#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

#include "libc.h"
#include "iobuffer.h"
#include "platform.h"

#define	CONN_POLL_INT	20	// polling time for socket

#ifdef	CONFIG_NET_DEBUG
#define	net_debug(...)	xprintf(__VA_ARGS__)
#else
#define	net_debug(...)
#endif

struct {
	enum net_state_e	state;
	int			sock;
	struct pollfd		conn_poll;
	unsigned long		rx;
	unsigned long		packet_size;
	unsigned long		packet_count;
	unsigned long		packet_max;
} static ns;

void host_net_init(void) {

	uc_memset(&ns, 0, sizeof(ns));

	ns.state = NET_CLOSED;
	net_debug("%s\n", __func__);
}

void host_net_done(void) {
	net_close();
	net_debug("%s\n", __func__);
}

void host_net_open_tcp(const char *address, int port) {
	struct sockaddr_in	sin;
	struct hostent		*host;
	struct linger		l;
	int			ret;

	net_debug("%s: ", __func__);

	host = gethostbyname(address);
	if (host == NULL) {
		net_debug("dns error\n");
		goto error_dns;
	}

	ns.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ns.sock == -1) {
		net_debug("socket error\n");
		goto error_socket;
	}

	l.l_onoff = 1;
	l.l_linger = 5;

	ret = setsockopt (ns.sock, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof (l));
	if (ret < 0) {
		net_debug("setsockopt error\n");
		goto error_socket;
	}

	uc_memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr = *((struct in_addr *) host->h_addr_list[0]);

	ret = connect(ns.sock, (struct sockaddr *) &sin,
		    sizeof(struct sockaddr_in));
	if (ret < 0) {
		net_debug("connect error\n");
		goto error_socket;
	}

	net_debug("OK\n");
	return;

error_dns:
	ns.state = NET_ERROR;
	return;

error_socket:
	ns.state = NET_ERROR;
	return;
}

void host_net_request(const char *hostname, const char *filename) {
	char	httprequest[1024];
	int	len, ret;

	uc_memset(httprequest, 0, sizeof(httprequest));

	xsprintf(httprequest,
		 "GET %s HTTP/1.0\r\n"
		 "Pragma: no-cache\r\n"
		 "Host: %s\r\n"
		 "User-Agent: xmms/1.2.7\r\n"
		 "Accept: * / *\r\n" "\r\n",
		 filename, hostname);

	len = uc_strlen((char *) httprequest);

	net_debug("%s: request len: %d\n", __func__, len);

	ret = send(ns.sock, httprequest, len, 0);
	if (ret < 0) {
		ns.state = NET_ERROR;
		goto error;
	}

	uc_memset(httprequest, 0, sizeof(httprequest));
	ret = recv(ns.sock, httprequest, sizeof(httprequest), 0);
	if (ret < 0) {
		ns.state = NET_ERROR;
		goto error;
	}
//	net_debug ("http response\n%s\n", httprequest);
	net_debug("%s: response len: %d\n", __func__, ret);

	if (uc_strstr((char *) httprequest, "404") != NULL) {
		xprintf("%s: Error 404, media not found.\n", __func__);
		ns.state = NET_ERROR;
		goto error;
	}

	if (uc_strstr ((char*)httprequest, "application/ogg") != NULL) {
		xprintf ("%s: Requested media is OGG, which is not supported.\n", __func__);
		ns.state = NET_ERROR;
		goto error;
	}

	if (uc_strstr((char *) httprequest, "audio/mpeg") == NULL) {
		xprintf("%s: net_request: unknown media type.\n", __func__);
		xprintf("'%s'\n", httprequest);
		ns.state = NET_ERROR;
		goto error;
	}

error:
	return;
}

void host_net_open(const char *url) {
	char	hostname[MAX_HOSTNAME];
	char	filename[MAX_URL];
	int	port, flags;

	ns.state = NET_OPEN;
	ns.rx = 0;

	ns.packet_size = 0;
	ns.packet_count = 0;
	ns.packet_max = 0;

	net_debug("%s\n", __func__);

	uc_memset(hostname, 0, MAX_HOSTNAME);
	uc_memset(filename, 0, MAX_URL);

	net_parse_url(url, hostname, &port, filename);

	host_net_open_tcp(hostname, port);
	if (ns.state != NET_OPEN) {
		xprintf("%s: Error opening socket.\n", __func__);
		goto open;
	}

	net_debug("%s: Connected to '%s':'%d'\n", __func__, hostname, port);

	host_net_request(hostname, filename);
	if (ns.state != NET_OPEN) {
		xprintf("%s: Error requesting media.\n", __func__);
		goto request;
	}

	ns.conn_poll.fd = ns.sock;
	ns.conn_poll.events = POLLIN | POLLERR | POLLHUP | POLLRDHUP;

	flags = fcntl(ns.sock, F_GETFL, 0);
	fcntl(ns.sock, F_SETFL, flags | O_NONBLOCK);

	return;

request:
	close(ns.sock);

open:
	return;
}

void host_net_close(void) {

	if (ns.sock)
		close(ns.sock);

	ns.state = NET_CLOSED;

	net_debug("%s\n", __func__);
}

void host_net_poll(void) {
	struct	input_s *buff;
	int	error;
	int	ret;

	if (ns.state != NET_OPEN)
		return;

	if (i_free() < 1)
		return;

	ret = poll(&ns.conn_poll, 1, CONN_POLL_INT);
	if (ret > 0 && (ns.conn_poll.revents & POLLIN)) {

		buff = i_push();

		ret = recv(ns.sock, buff->data, buff->size, 0);
		buff->used = ret;

		ns.packet_size += ret;
		ns.packet_count++;
		if (ret > ns.packet_max)
			ns.packet_max = ret;

		ns.rx += ret;

	} else if (ret < 0) {

		error = errno;
		xprintf("%s: failed with error %d (%s).\n",
			__func__, error, strerror(error));
		ns.state = NET_ERROR;
		goto exit;

	}

	/* else don't care about ret == 0 */

	if (ns.conn_poll.revents & (POLLHUP | POLLRDHUP)) {

		xprintf("%s: connection hangup (%d).\n",
			__func__, ns.conn_poll.revents);
		ns.state = NET_ERROR;
		goto exit;

	}

	if (ns.conn_poll.revents & (POLLERR)) {
		xprintf("%s: connection error (%d).\n",
			__func__, ns.conn_poll.events);
		ns.state = NET_ERROR;
		goto exit;
	}
exit:
	return;
}

enum net_state_e host_net_state(void) {
	return (ns.state);
}

/////////// Shell interface ////////////////////


void cmd_nstop(void) {
	net_close();
}

void cmd_net_state(void) {

	xprintf("Connection state: ");
	switch (ns.state) {
	case NET_CLOSED:
		xprintf("closed");
		break;
	case NET_OPEN:
		xprintf("open");
		break;
	case NET_ERROR:
		xprintf("error");
		break;
	case NET_PENDING:
		xprintf("pending(?!)");
		break;
	case NET_UNKNOWN:
		xprintf("not initialized");
		break;
	}
	xprintf(", rx: %d KB.\n", ns.rx / 1024);

	if (ns.packet_count)
		xprintf ("Received %d packet of average size %d. Maximum was %d.\n",
			ns.packet_count, ns.packet_size/ns.packet_count, ns.packet_max);

	xprintf("\n");
}
