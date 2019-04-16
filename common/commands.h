#ifndef	COMMANDS_H
#define	COMMANDS_H

// Build in commands
void cmd_help(void);
void cmd_exit(void);
void cmd_echo(void);
void cmd_cls(void);
void cmd_version(void);

// Player commands
void cmd_pl_play(void);
void cmd_pl_stop(void);
void cmd_pl_status(void);

// Interim state stuff
void cmd_io_state(void);
void cmd_snd_state(void);
void cmd_libmad_state(void);
void cmd_net_state(void);
void cmd_snd_volume(void);

// STM32 specific stuff
void cmd_fs(void);
void cmd_fs_chdir(void);
void cmd_fs_list(void);
void cmd_fs_pwd(void);
void cmd_usb(void);
void cmd_net_ifconfig(void);

// debug stuff, has to be removed at some point
void cmd_radio(void);
void snd_init(void);
void cmd_fstate(void);

void cmd_res_list(void);

const struct command_s commands[] = {

	{"help", cmd_help},		// print list of known commands
	{"echo", cmd_echo},		// just print the content of input to output
	{"cls", cmd_cls},		// clear screen

	{"play", cmd_pl_play},
	{"stop", cmd_pl_stop},

	{"svol", cmd_snd_volume},	// volume adjustment
	{"mute", cmd_snd_volume},	// volume adjustment

	{"radio", cmd_radio},		// pre-selected radios

#ifdef	__STM32__
	{"cd", cmd_fs_chdir},
	{"ls", cmd_fs_list},
	{"pwd", cmd_fs_pwd},

	{"fs", cmd_fs},			// sate of usb mount
	{"usb", cmd_usb},		// usb device info
	{"ifc", cmd_net_ifconfig},	// net device configuration
#endif

// Interim state stuff
	{"io", cmd_io_state},		// ring buffer state
	{"net", cmd_net_state},		// net module state
	{"snd", cmd_snd_state},		// sound state
	{"libmad", cmd_libmad_state},	// mp3 decoder state
	{"pls", cmd_pl_status},		// player state
	{"fss", cmd_fstate},		// state of FS module
	{"rls", cmd_res_list},		// list resources

#ifdef	__STM32__
	{"snd_init", snd_init},
#endif

	{"ver", cmd_version},		// print version number

#ifdef	__LINUX__
	{"exit", cmd_exit},
#endif
	{NULL, NULL}			// terminator
};

#endif
