#include <time.h>
#include <stdint.h>

static uint32_t	start_time = 0;

uint32_t get_local_time(void) {
	struct timespec	tp;
	int		curtime;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	if (!start_time) {
		start_time = tp.tv_sec;
		return (tp.tv_nsec / 1000000);
	}

	curtime = (tp.tv_sec - start_time) * 1000 + tp.tv_nsec / 1000000;

	return (curtime);
}
