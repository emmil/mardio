#include "libc.h"
#include "lcd.h"
#include "platform.h"
#include "player.h"

#ifdef	CONFIG_LCD_DEBUG
#define	lcd_debug(...)	xprintf(__VA_ARGS__)
#else
#define	lcd_debug(...)
#endif

#ifdef	NO_LCD
void lcd_init(void) {
}

void lcd_poll(void) {
}

void lcd_done(void) {
}

void lcd_startup(enum lcd_ss_e stage) {
}

void lcd_play(void) {
}

void lcd_stop(void) {
}

void lcd_error(enum lcd_err_e error) {
}

void lcd_volume(void) {
}

void lcd_reset(void) {
}

void lcd_putpixel(uint16_t x, uint16_t y, uint8_t color) {
}

void lcd_update_screen(void) {
}

void lcd_clean_screen(void) {
}

void lcd_get_cfg(struct lcd_cfg_s *cfg) {

	if (cfg != NULL) {
		cfg->width = 0;
		cfg->height = 0;
		cfg->bpp = 0;
	}
}
#else


struct lcd_timer_s {
	uint32_t	timeout;
	uint8_t		active;
};

struct {
	struct lcd_timer_s	volume;
	struct lcd_timer_s	media;
	int			error;
} static ls;


void lcd_reset(void) {
	lcd_debug("%s\n", __func__);

	uc_memset(&ls, 0, sizeof(ls));
}

void lcd_init(void) {
	struct lcd_cfg_s	cfg;

	lcd_debug("%s\n", __func__);

	lcd_reset();

	host_lcd_init();

	lcd_get_cfg(&cfg);

	lcd_debug("%s: Got configuration %dx%d pixels, %dbpp\n",
		  __func__, cfg.width, cfg.height, cfg.bpp);
}

void lcd_done(void) {

	host_lcd_done();
	lcd_debug("%s\n", __func__);
}

void lcd_draw(enum lcd_resource_e resource) {

	if (ls.error)
		return;

	host_lcd_draw(resource);
}

void lcd_timer(void) {
	uint32_t	time = get_local_time();

	if (ls.volume.active != 0 && ls.volume.timeout < time) {
		ls.volume.active = 0;
		ls.volume.timeout = 0;
		goto event;
	}

	if (ls.media.active != 0 && ls.media.timeout < time) {
		ls.media.active = 0;
		ls.media.timeout = 0;
		goto event;
	}


	goto out;

event:
	if (pl_state() == PL_PLAYING) {
		switch (pl_source()) {
		case PS_NET:
			lcd_draw(LR_RADIO);
			break;
		case PS_FILE:
			lcd_draw(LR_FILE);
			break;
		default:
			xprintf
			    ("%s: Unknown player source while playing?\n", __func__);
		}
	} else {
		lcd_draw(LR_WELLCOME);
	}

out:
	return;
}

void lcd_poll(void) {

	if (ls.error)
		return;

	lcd_timer();
}

void lcd_startup(enum lcd_ss_e stage) {

	switch (stage) {

	case LSS_BLANK:
		lcd_draw(LR_BLANK);
		break;

	case LSS_INIT:
		lcd_draw(LR_STARTUP);
		break;

	case LSS_WELLCOME:
		lcd_draw(LR_WELLCOME);
	}
}

void lcd_error(enum lcd_err_e error) {

	switch (error) {
	case LER_COMMON:
		lcd_draw(LR_ERROR);
		break;

	case LER_INIT:
		lcd_draw(LR_ERROR_INIT);
		break;

	case LER_NET:
		lcd_draw(LR_ERROR_NET);
		break;

	case LER_FS:
		lcd_draw(LR_ERROR_FS);
		break;

	case LER_SOUND:
		lcd_draw(LR_ERROR_SOUND);
		break;

	case LER_LIBMAD:
		lcd_draw(LR_ERROR_LIBMAD);
		break;
	}

	ls.error = 1;
}

void lcd_play(void) {

	lcd_draw(LR_PLAY);

	ls.media.timeout = LCD_TIMEOUT_MEDIA + get_local_time();
	ls.media.active = 1;
}

void lcd_stop(void) {

	lcd_draw(LR_STOP);

	ls.media.timeout = LCD_TIMEOUT_MEDIA + get_local_time();
	ls.media.active = 1;
}

void lcd_volume(void) {
	uint8_t	volume = pl_volume_get();

	lcd_draw(volume + LR_VOLUME_00);

	ls.volume.timeout = LCD_TIMEOUT_VOLUME + get_local_time();
	ls.volume.active = 1;
}

void lcd_get_cfg(struct lcd_cfg_s *cfg) {

	if (cfg != NULL)
		host_lcd_get_cfg(cfg);
}

void lcd_putpixel(uint16_t x, uint16_t y, uint8_t color) {
	host_lcd_putpixel(x, y, color);
}

void lcd_update_screen(void) {
	host_lcd_update_screen();
}

void lcd_clean_screen(void) {
	host_lcd_clean_screen();
}

#endif
