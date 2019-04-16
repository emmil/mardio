#ifndef	FS_H
#define	FS_H

#include <stdint.h>

#define	MAX_OPEN_FILES		3

enum fs_state_e {
	FSS_UNKNOWN = 0,
	FSS_OPEN,
	FSS_CLOSED,
	FSS_ERROR,
};

enum fs_type_e {
	FST_UNKNOWN = 0,
	FST_NONE,
	FST_RIFF,
	FST_MP3,
};

enum fs_type_e fs_type(const char *name);

void fs_poll(void);
void fs_init(void);
void fs_done(void);
void fs_stop(void);
void fs_play(const char *name);
enum fs_state_e fs_state(void);
enum fs_type_e fs_playing(void);
int fs_open(const char *name);
void fs_close(int handle);
uint32_t fs_read(int handle, void *buffer, uint32_t size);
void fs_seek(int handle, uint32_t offset);

#endif
