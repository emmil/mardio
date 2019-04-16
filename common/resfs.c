#include "libc.h"
#include "cmd.h"

#include "resfs.h"


/////////////////////////////////////////////////////////////////////

#ifndef	HAVE_RESOURCES
struct rsentry_s	entries[] = {
				{NULL, 0, NULL, 0},
				{NULL, 0, NULL, 0},
				/* A second entry to make compiler happy */
				};
#else
#include "resources.h"
#endif

#ifdef	CONFIG_RESOURCES_DEBUG
#define	res_debug(...)	xprintf(__VA_ARGS__)
#else
#define	res_debug(...)
#endif

struct {
	int	count;
} rs;

/////////////////////////////////////////////////////////////////////

int8_t res_find_handle(const char *name) {
	int	i;

	if (name == NULL || rs.count == 0)
		goto exit;

	for (i = 0; i < rs.count; i++) {
		if (!uc_strncmp
		    (name, entries[i].name, uc_strlen(entries[i].name))) {
			return i;
		}
	}

exit:
	return -1;
}

/////////////////////////////////////////////////////////////////////

void res_init(void) {
	int	i = 0;

	res_debug("res_init: ");

	uc_memset(&rs, 0, sizeof(rs));

	if (entries[0].name == NULL) {
		res_debug("have no resources.\n");
		return;
	}

	while (entries[i].name != NULL) {
		i++;
	}

	rs.count = i;

	res_debug("%d resources found.\n", rs.count);
}

void res_done(void) {
	res_debug("res_done\n");
}

int res_open(const char *name) {
	int	i;

	res_debug("res_open ('%s'): ", name);

	i = res_find_handle(name);

	res_debug("%d\n", i);

	return i;
}

void res_close(int handle) {

	res_debug("res_close (handle = %d)\n", handle);

	if (handle > rs.count || handle < 0)
		return;

	entries[handle].pos = 0;
}

uint32_t res_read(int handle, void *buffer, uint32_t size) {
	int		toread = size;
	int		pos;
	const uint8_t	*data;

	/*
	res_debug ("res_read (handle = %d, buffer = 0x%x, size = 0x%x)\n",
			handle, buffer, size);
	*/

	if (handle > rs.count || handle < 0 || buffer == NULL)
		return 0;

	if (entries[handle].pos + toread > entries[handle].size) {
		toread = entries[handle].size - entries[handle].pos;
	}

	pos = entries[handle].pos;
	data = entries[handle].data + pos;
	uc_memcpy(buffer, data, toread);
	entries[handle].pos += toread;

	if (entries[handle].pos > entries[handle].size)
		entries[handle].pos = entries[handle].size;

	/*
	res_debug ("res_read: pos is %d (0x%x)\n",
			entries[handle].pos,
			entries[handle].pos);
	*/

	return toread;
}

void res_seek(int handle, uint32_t offset) {

	res_debug("res_seek (handle = %d, offset = 0x%x)\n", handle, offset);

	if (handle > rs.count || handle < 0)
		goto exit;

	if (entries[handle].size >= offset)
		entries[handle].pos = offset;
	else
		entries[handle].pos = entries[handle].size;

exit:
	return;
}

/////////////////////////////////////////////////////////////////////

const uint8_t *res_data_byname(const char *name) {
	int8_t		i;
	const uint8_t	*data = NULL;

	res_debug("res_data_byname (name = %s): ", name);

	i = res_find_handle(name);

	if (i > 0)
		data = entries[i].data;

	res_debug("0x%x\n", data);

	return data;
}

const uint8_t *res_data_byid(int8_t handle) {

	res_debug("res_data_byid (handle = %d)\n", handle);

	if (handle < 0 || handle > rs.count) {
		return NULL;
	}

	return entries[handle].data;
}

int32_t res_size_byname(const char *name) {
	int8_t	i;
	int32_t	size = -1;

	res_debug("res_size_byname (name = %s): ", name);

	i = res_find_handle(name);

	if (i > 0)
		size = entries[i].size;

	res_debug("%d\n", size);

	return size;
}

int32_t res_size_byid(int8_t handle) {

	res_debug("res_size_byid (handle = %d)\n", handle);

	if (handle < 0 || handle > rs.count) {
		return -1;
	}

	return entries[handle].size;
}

/////////////////////////////////////////////////////////////////////

void cmd_res_list(void) {
	int	i;

	if (rs.count == 0) {
		xprintf("No resources found.\n\n");
		return;
	}

	xprintf("No. | Size | Pos | Data | Name \n");
	for (i = 0; i < rs.count; i++) {
		xprintf("%2d, %4d, %4d, 0x%x, %s\n",
			i,
			entries[i].size,
			entries[i].pos,
			entries[i].data,
			entries[i].name);
	}
	xprintf("\n");
}
