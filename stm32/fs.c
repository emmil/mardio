#include "stm32f4_discovery.h"
#include "ff.h"

#include "libc.h"
#include "cmd.h"
#include "iobuffer.h"
#include "platform.h"
#include "stm32.h"

#ifdef	CONFIG_FS_DEBUG
#define	fs_debug(...)	xprintf(__VA_ARGS__)
#else
#define	fs_debug(...)
#endif

#ifndef	MAX_OPEN_FILES
#define	MAX_OPEN_FILES	3
#endif

struct fs_s {
	FIL	f;
	int8_t	open;
};

struct {
	int	mounted;
	FATFS	fs;
	struct	fs_s f[MAX_OPEN_FILES];
} static fs;

const struct s_message {
	char	id;
	char	*message;
	char	*description;
} message[] = {
	{
	0, "FR_OK", "Succeeded"}, {
	1, "FR_DISK_ERR", " A hard error occurred in the low level disk I/O layer"}, {
	2, "FR_INT_ERR", "Assertion failed"}, {
	3, "FR_NOT_READY", "The physical drive cannot work"}, {
	4, "FR_NO_FILE", "Could not find the file"}, {
	5, "FR_NO_PATH", "Could not find the path"}, {
	6, "FR_INVALID_NAME", "The path name format is invalid"}, {
	7, "FR_DENIED", "Access denied due to prohibited access or directory full"}, {
	8, "FR_EXIST", "Access denied due to prohibited access"}, {
	9, "FR_INVALID_OBJECT", "The file/directory object is invalid"}, {
	10, "FR_WRITE_PROTECTED", "The physical drive is write protected"}, {
	11, "FR_INVALID_DRIVE", "The logical drive number is invalid"}, {
	12, "FR_NOT_ENABLED", "The volume has no work area"}, {
	13, "FR_NO_FILESYSTEM", "There is no valid FAT volume"}, {
	14, "FR_MKFS_ABORTED", "The f_mkfs() aborted due to any parameter error"}, {
	0, NULL, NULL},};

void fs_error(FRESULT res) {
	xprintf("%s: %s\n", message[res].message, message[res].description);
}

void host_fs_init(void) {
	int	i;

	uc_memset(&fs, 0, sizeof(fs));

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		fs.f[i].open = 0;
	}

	fs_debug("    fs_host_init\n");
}

void host_fd_done(void) {
}

void host_fs_close(int handle) {

	if (handle < 0 || handle > MAX_OPEN_FILES)
		return;

	if (fs.f[handle].open != 1)
		return;

	f_close(&fs.f[handle].f);
	fs.f[handle].open = 0;

	fs_debug("fs_close\n");
}

int host_fs_open(const char *name) {
	int	i;
	FRESULT	res;

	fs_debug("fs_host_open\n");

	if (fs.mounted == 0)
		return -1;

	if (name == NULL)
		return -1;

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (fs.f[i].open == 0) {
			res = f_open(&fs.f[i].f, name,
				   FA_READ | FA_OPEN_EXISTING);
			if (res) {
				fs_error(res);
				goto error;
			}
			fs.f[i].open = 1;
		}
	}

error:
	return -1;
}

uint32_t host_fs_read(int handle, void *buffer, uint32_t size) {
	FRESULT		res;
	uint32_t	got = -1;

	if (handle < 0 || handle > MAX_OPEN_FILES) {
		fs_debug("host_fs_read (handle = %d): Failed. Bad handle index\n", handle);
	}

	if (fs.mounted == 0) {
		fs_debug("host_fs_read: FS is not mounted.\n");
		return -1;
	}

	if (fs.f[handle].open == 0) {
		fs_debug("host_fs_read (handle = %d): file is not open.\n", handle);
		return -1;
	}

	if (buffer == NULL || size == 0) {
		fs_debug("host_fs_fread: Failed. Reading into empty buffer.\n");
		return -1;
	}

	res = f_read(&fs.f[handle].f, buffer, size, &got);
	if (res) {
		fs_error(res);
		return -1;
	}

	return got;
}

void host_fs_seek(int handle, uint32_t offset) {
	FRESULT	res;

	if (handle < 0 || handle > MAX_OPEN_FILES) {
		fs_debug("host_fs_read (handle = %d): Failed. Bad handle index\n", handle);
	}

	if (fs.mounted == 0) {
		fs_debug("host_fs_read: FS is not mounted.\n");
		return;
	}

	if (fs.f[handle].open == 0) {
		fs_debug("host_fs_read (handle = %d): file is not open.\n",
			 handle);
		return;
	}

	res = f_lseek(&fs.f[handle].f, offset);
	if (res)
		fs_error(res);
}

void fs_mount(void) {
	FRESULT	res;

	res = f_mount(&fs.fs, "", 1);
	if (res) {
		xprintf("ff_mount: ");
		fs_error(res);
		return;
	}

	fs.mounted = 1;

	STM_EVAL_LEDOn(LED_FS);

	fs_debug("FS mounted\n");
}

void fs_umount(void) {
	int	i;

	if (fs.mounted == 0)
		return;

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		host_fs_close(i);
	}

	f_mount(NULL, "", 1);

	fs.mounted = 0;

	STM_EVAL_LEDOff(LED_FS);

	fs_debug("FS is unmounted.\n");
}

int fs_ismounted(void) {
	return (fs.mounted);
}

void fs_poll_usb(void) {
	fs_poll();
}

//////////////// Shell interface comands //////////

void fs_list(const TCHAR * path) {
	FILINFO	finfo;
	FRESULT	res;
	DIR	dir;
	int	i;
#if	_USE_LFN
	TCHAR LFName[256];

	finfo.lfname = LFName;
	finfo.lfsize = sizeof(LFName);
#endif
	res = f_opendir(&dir, path);
	if (res) {
		xprintf("ff_list: ");
		fs_error(res);
		goto error;
	}
	for (;;) {
		res = f_readdir(&dir, &finfo);
		if (res != FR_OK || !finfo.fname[0])
			break;

		xprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s ",
			(finfo.fattrib & AM_DIR) ? 'D' : '-',
			(finfo.fattrib & AM_RDO) ? 'R' : '-',
			(finfo.fattrib & AM_HID) ? 'H' : '-',
			(finfo.fattrib & AM_SYS) ? 'S' : '-',
			(finfo.fattrib & AM_ARC) ? 'A' : '-',
			(finfo.fdate >> 9) + 1980, (finfo.fdate >> 5) & 15,
			finfo.fdate & 31, (finfo.ftime >> 11),
			(finfo.ftime >> 5) & 63, finfo.fsize, finfo.fname);

#if _USE_LFN
		for (i = uc_strlen(finfo.fname); i < 16; i++)
			xprintf(" ");
		xprintf("%s", LFName);
#endif
		xprintf("\n");

	}

	f_closedir(&dir);

error:
	return;
}

void fs_cd(TCHAR * path) {
	FRESULT	res;

	res = f_chdir(path);
	if (res) {
		xprintf("ff_cd: ");
		fs_error(res);
	}
}

////////////////////////////////////////////////////////////////////////////////

void cmd_fs_pwd(void) {
	TCHAR	line[256];
	FRESULT	res;

	res = f_getcwd(line, sizeof line / sizeof *line);
	if (res) {
		xprintf("fat_pwd: ");
		fs_error(res);
		goto error;
	}

	xprintf("'%s'\n\n", line);

error:
	return;
}

void cmd_fs_list(void) {

	if (cmd_Argc() == 1) {
		fs_list(".");
	} else {
		fs_list(cmd_Argv(1));
	}

	xprintf("\n");
}

void cmd_fs_chdir(void) {
	const char	*arg;

	if (cmd_Argc() == 1) {
		xprintf("parameter required.\n");
		return;
	}

	arg = cmd_Argv(1);
	fs_cd((TCHAR *) arg);

	xprintf("\n");
}


void cmd_fs(void) {

	xprintf("FS is %s.\n", fs.mounted ? "mounted" : "not mounted");
//	xprintf("File in %s.\n", fs.open ? "open" : "closed");

	xprintf("\n");
}
