#include "player.h"
#include "console.h"
#include "platform.h"
#include "libmad.h"
#include "libc.h"
#include "cmd.h"
#include "iobuffer.h"
#include "fs.h"
#include "lcd.h"
#include "event.h"

#ifdef	CONFIG_PLAYER_DEBUG
#define	pl_debug(...)	xprintf(__VA_ARGS__)
#else
#define	pl_debug(...)
#endif

struct {
	char			source_addr[MAX_CONSOLE_INPUT];
	enum pl_source_e	source_type;
	enum pl_state_e		state;
	uint8_t			volume;
} static pl;

///////////////////////////////////////
// Help functions

void pl_save_source(const char *what) {
	int	len;

	if (what == NULL)
		return;

	uc_memset(pl.source_addr, 0, sizeof(pl.source_addr));

	len = uc_strlen(what);
	if (len > sizeof(pl.source_addr)) {
		xprintf("pl_save_source: Saving of source failed.\n");
		return;
	}

	uc_memcpy(pl.source_addr, what, len);
}

///////////////////////////////////////

uint8_t pl_volume_get(void) {
	return pl.volume;
}

void pl_volume_set(uint8_t new_volume) {

	pl.volume = new_volume;

	if (pl.volume < PL_MIN_VOLUME)
		pl.volume = PL_MIN_VOLUME;

	if (pl.volume > PL_MAX_VOLUME)
		pl.volume = PL_MAX_VOLUME;

	lcd_volume();
}

void pl_volume_up(void) {
	pl_volume_set(pl_volume_get() + 1);
}

void pl_volume_down(void) {
	pl_volume_set(pl_volume_get() - 1);
}

///////////////////////////////////////

void pl_init(void) {

	pl_debug("pl_init\n");

	uc_memset(&pl, 0, sizeof(pl));
	pl.state = PL_STOP;
	pl.volume = PL_DEF_VOLUME;
}

void pl_play(const char *what) {

	pl_stop();

	pl_debug("pl_play (what='%s')\n", what);

	pl_save_source(what);

	io_init();
	libmad_init();
	lcd_reset();

	if (!uc_memcmp(what, HTTP_URI, uc_strlen(HTTP_URI))) {

		lcd_startup(LSS_INIT);
		pl.source_type = PS_NET;
		net_open(what);
		pl.state = PL_BUFFERING;

	} else {

		pl.source_type = PS_FILE;
		fs_play(what);

		switch (fs_playing()) {

		case FST_UNKNOWN:
		case FST_NONE:
			pl_stop();
			break;

		case FST_RIFF:
			pl.state = PL_PLAYING;
			break;

		case FST_MP3:
			pl.state = PL_BUFFERING;
			break;
		}
	}

	lcd_play();
}

void pl_stop(void) {

	pl_debug("pl_stop\n");

	if (pl.state == PL_PLAYING || pl.state == PL_BUFFERING) {
		fs_stop();
		net_close();
		libmad_done();
		pl.state = PL_STOP;
		lcd_stop();
	}
}

void pl_poll(void) {
	enum net_state_e	net_st = net_state();
	enum fs_state_e		fs_st = fs_state();
	enum sound_state_e	snd_st = snd_state();
	enum libmad_state_e	mad_st = libmad_state();

	if (pl.state != PL_PLAYING && pl.state != PL_BUFFERING)
		return;

	if (pl.state == PL_BUFFERING && i_free() == 0) {
		pl.state = PL_PLAYING;
		pl_debug("%s: playing.\n", __func__);
	}

	if (net_st == NET_ERROR || snd_st == SS_ERROR || mad_st == LM_ERROR) {

// this is the right place to intercept run errors that can be shown to user on UI
#if	1
		if (net_st == NET_ERROR) {
			xprintf("pl_loop: net error\n");
			lcd_error(LER_NET);
		}

		if (snd_st == SS_ERROR) {
			xprintf("pl_loop: sound error\n");
			lcd_error(LER_SOUND);
		}

		if (mad_st == LM_ERROR) {
			xprintf("pl_loop: libmad error\n");
			lcd_error(LER_LIBMAD);
		}

		if (fs_st == FSS_ERROR) {
			xprintf("pl_loop: fs error\n");
			lcd_error(LER_FS);
		}
#else
		xprintf("pl_loop: Stopping on error.\n");
		lcd_error(LER_COMMON);
#endif

		pl_stop();
	}


	if (pl.state == PL_PLAYING || pl.state == PL_BUFFERING) {
		if (pl.source_type == PS_FILE && fs_st == FSS_CLOSED)
			pl_stop();

		if (pl.source_type == PS_NET && net_st == NET_CLOSED)
			pl_stop();
	}

	if (pl.state == PL_PLAYING && io_starving()) {
		pl.state = PL_BUFFERING;
		pl_debug("%s: buffering.\n", __func__);
	}


}

const char *pl_what(void) {
	return pl.source_addr;
}

enum pl_state_e pl_state(void) {
	return pl.state;
}

enum pl_source_e pl_source(void) {
	return pl.source_type;
}

///////////////////////////////////////
// Command line interface
//

void cmd_pl_play(void) {
	const char	*arg;

	if (cmd_Argc() != 2) {
		xprintf("Enter name of file or URL to play.\n");
		return;
	}

	arg = cmd_Argv(1);

	event_push(E_PL_PLAY, 0, (void *) arg);

	xprintf("\n");
}

void cmd_pl_stop(void) {

	event_push(E_PL_STOP, 0, NULL);
	xprintf("\n");
}

void cmd_pl_status(void) {
	enum net_state_e	net_st = net_state();
	enum fs_state_e		fs_st = fs_state();
	enum sound_state_e	snd_st = snd_state();
	enum libmad_state_e	mad_st = libmad_state();

	xprintf("Player is ");
	switch (pl.state) {
	case PL_UNKNOWN:
		xprintf("not initialized, yet.\n");
		break;
	case PL_STOP:
		xprintf("not playing now.\n");
		break;
	case PL_PLAYING:
		xprintf("is playing");
		if (uc_strlen(pl_what()) > 0)
			xprintf(" '%s'.\n", pl_what());
		else
			xprintf(".\n");

		break;
	case PL_BUFFERING:
		xprintf("is buffering.\n");
		break;
	}

	xprintf("Network ");
	switch (net_st) {
	case NET_UNKNOWN:
		xprintf("is not initialized.\n");
		break;
	case NET_PENDING:
		xprintf("is waiting for a reply from remote.\n");
		break;
	case NET_OPEN:
		xprintf("connection is established.\n");
		break;
	case NET_CLOSED:
		xprintf("connection is closed.\n");
		break;
	case NET_ERROR:
		xprintf("encountered an error.\n");
		break;
	}

	switch (fs_st) {
	case FSS_UNKNOWN:
		xprintf("Files have been not initialized.\n");
		break;
	case FSS_OPEN:
		xprintf("A file is being played.\n");
		break;
	case FSS_CLOSED:
		xprintf("No file is being played.\n");
		break;
	case FSS_ERROR:
		xprintf("An error occuring while accessing a file.\n");
		break;
	}

	xprintf("Sound device ");
	switch (snd_st) {
	case SS_UNKNOWN:
		xprintf("has not been initialized.\n");
		break;
	case SS_INIT:
		xprintf("is not playing any sound.\n");
		break;
	case SS_PLAYING:
		xprintf("is playing now.\n");
		break;
	case SS_PAUSE:
		xprintf("is paused.\n");
		break;
	case SS_ERROR:
		xprintf("has encountered errors.\n");
		break;
	}

	xprintf("Libmad decoder ");
	switch (mad_st) {
	case LM_UNKNOWN:
		xprintf("is not initialized.\n");
		break;
	case LM_INIT:
		xprintf("is initalized.\n");
		break;
	case LM_DONE:
		xprintf("has been deinitialized.\n");
		break;
	case LM_ERROR:
		xprintf("has encountered error.\n");
		break;
	}

	xprintf("\n");
}

void cmd_snd_volume(void) {
	int		volume = pl_volume_get();
	int		len;
	const char	*param;

	if (cmd_Argc() != 2) {

		len = uc_strlen(cmd_Argv(0));
		if (!uc_memcmp(cmd_Argv(0), "mute", len)) {
			volume = 0;
			goto set_volume;
		}

		xprintf("Sound volume is %d.\n\n", volume);
		return;
	}

	param = cmd_Argv(1);
	len = uc_strlen(param);

	if (!uc_memcmp(param, "up", len) || !uc_memcmp(param, "+", len)) {
		volume++;
	} else
	    if (!uc_memcmp(param, "down", len) || !uc_memcmp(param, "-", len)) {
		volume--;
	} else if (!uc_memcmp(param, "mute", len)) {
		volume = 0;
	} else {
		volume = q3_atoi(param);
	}

set_volume:
	if (volume < PL_MIN_VOLUME)
		volume = PL_MIN_VOLUME;

	if (volume > PL_MAX_VOLUME)
		volume = PL_MAX_VOLUME;

	event_push(E_PL_VOLSET, volume, NULL);

	xprintf("\n");
}

/* This has to be defined globally, or else, the compiler will use memcpy */
struct {
	char	*desc;
	char	*url;
} const radios[] = {
	{"Slovensko 128 bit", "http://live.slovakradio.sk:8000/Slovensko_128.mp3"},
	{"Radio Devin 128 bit", "http://live.slovakradio.sk:8000/Devin_128.mp3"},
	{"Radio Litera 128 bit", "http://live.slovakradio.sk:8000/Litera_128.mp3"},
	{"Radio Junior 128 bit", "http://live.slovakradio.sk:8000/Junior_128.mp3"},
	{"Radio Paradise 128 bit", "http://stream-uk1.radioparadise.com:80/mp3-128"},
	{"Fun Radio 128 bit", "http://stream.funradio.sk:8000/fun128.mp3"},
	{"Slovensko 256 bit", "http://live.slovakradio.sk:8000/Slovensko_256.mp3"},
	{"Radio Litera 256 bit", "http://live.slovakradio.sk:8000/Litera_256.mp3"},
	{"Radio Devin 256 bit", "http://live.slovakradio.sk:8000/Devin_256.mp3"},
	{"Radio Junior 256 bit", "http://live.slovakradio.sk:8000/Junior_256.mp3"},
	{"Radio Paradise 192 bit", "http://stream-uk1.radioparadise.com:80/mp3-192"},
	{"Fun Radio 256 bit", "http://stream.funradio.sk:8000/fun256.mp3"},
	{"Slovensko 128 bit (IP)", "http://62.168.92.250:8000/Slovensko_128.mp3"},
	{"Slovensko 256 bit (IP)", "http://62.168.92.250:8000/Slovensko_256.mp3"},
//	{NULL, NULL},	// comment this line for ogg media tesing
	{"Radio 128 bit (ogg)", "http://live.slovakradio.sk:8000/Devin_128.ogg"},
	{"Fun Radio 128 bit (ogg)", "http://stream.funradio.sk:8000/fun128.ogg"},
	{NULL, NULL},
};

void cmd_radio(void) {
	int	choice = 0;
	int	max_choice = 0;

	if (cmd_Argc() != 2) {

		while (radios[choice].url != NULL) {
			xprintf("%d - %s\n", choice, radios[choice].desc);
			choice++;
		}
		goto exit;
	}

	while (radios[max_choice].url != NULL) {
		max_choice++;
	}

	choice = q3_atoi(cmd_Argv(1));
	if (choice < 0 || choice >= max_choice) {
		xprintf ("Invalid choice. Try 0 - %d.\n", (max_choice - 1));
		goto exit;
	}

	event_push(E_PL_PLAY, 0, radios[choice].url);
exit:

	xprintf("\n");
}
