#include <stdint.h>

#include "platform.h"

void host_fs_init(void) {
}

void host_fs_done(void) {
}

int host_fs_open(const char *name) {
	return -1;
}

void host_fs_close(int handle) {
}

uint32_t host_fs_read(const int handle, void *buffer, const uint32_t size) {
	return 0;
}

void host_fs_seek(const int handle, const uint32_t offset) {
}
