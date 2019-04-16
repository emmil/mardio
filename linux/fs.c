#include <stdio.h>
#include <stdlib.h>

#include "libc.h"
#include "platform.h"

#ifdef	CONFIG_FS_DEBUG
#define	fs_debug(...)	xprintf(__VA_ARGS__)
#else
#define	fs_debug(...)
#endif

#ifndef	MAX_OPEN_FILES
#define	MAX_OPEN_FILES	5
#endif

struct fs_s {
	FILE	*f;
};

struct {
	struct fs_s	fs[MAX_OPEN_FILES];
} static hfs;

void host_fs_init(void) {
	int	i;

	uc_memset(&hfs, 0, sizeof(hfs));

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		hfs.fs[i].f = NULL;
	}
}

void host_fs_done(void) {
	int	i;

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		host_fs_close(i);
	}
}

int host_fs_open(const char *name) {
	int	i;

	fs_debug("%s ('%s'): ", __func__, name);

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (hfs.fs[i].f == NULL) {

			hfs.fs[i].f = fopen(name, "r");

			if (hfs.fs[i].f == NULL)
				goto error;

			fs_debug("OK\n");

			return i;
		}
	}
error:
	fs_debug("failed\n");

	return -1;
}

void host_fs_close(int handle) {

	if (handle < 0 || handle > MAX_OPEN_FILES)
		return;

	if (hfs.fs[handle].f != NULL) {
		fclose(hfs.fs[handle].f);
		hfs.fs[handle].f = NULL;
		fs_debug("%s (handle = %d)\n", __func__, handle);
	}
}

uint32_t host_fs_read(int handle, void *buffer, uint32_t size) {
	uint32_t	got;

	if (handle < 0 || handle > MAX_OPEN_FILES) {
		fs_debug("%s (handle = %d): Failed. Bad handle index\n",
			 __func__, handle);
		return 0;
	}

	if (hfs.fs[handle].f == NULL) {
		fs_debug("%s (handle = %d): Failed. File is not open\n",
			 __func__, handle);
		return 0;
	}

	if (buffer == NULL || size == 0) {
		fs_debug("%s (handle = %d): Failed. Reading into empty buffer.\n",
		     __func__, handle);
		return 0;
	}

	got = fread(buffer, 1, size, hfs.fs[handle].f);

//	fs_debug ("fs_fread (buffer = 0x%x, requested %d): got %d\n", buffer, size, got);

	return (got);
}

void host_fs_seek(int handle, uint32_t offset) {

	if (handle < 0 || handle > MAX_OPEN_FILES) {
		fs_debug("%s (handle = %d): Failed. Bad handle index\n",
			 __func__, handle);
		return;
	}

	if (hfs.fs[handle].f == NULL) {
		fs_debug("%s (handle = %d): Failed. File is not open\n",
			 __func__, handle);
		return;
	}

	fseek(hfs.fs[handle].f, offset, SEEK_SET);
}
