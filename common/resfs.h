#ifndef	RESFS_H
#define	RESFS_H

struct rsentry_s {
	const char	*name;
	const int32_t	size;
	const uint8_t	*data;
	int32_t		pos;
};

void res_init(void);
void res_done(void);
int res_open(const char *name);
void res_close(int handle);
uint32_t res_read(int handle, void *buffer, uint32_t size);
void res_seek(int handle, uint32_t offset);

const uint8_t *res_data_byname(const char *name);
const uint8_t *res_data_byid(int8_t handle);
int32_t res_size_byname(const char *name);
int32_t res_size_byid(int8_t handle);

#endif
