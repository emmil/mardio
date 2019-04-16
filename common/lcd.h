#ifndef LCD_H
#define	LCD_H

#include <stdint.h>

#define	LCD_TIMEOUT_VOLUME	3000
#define	LCD_TIMEOUT_MEDIA	3000

enum lcd_resource_e {
	LR_BLANK = 0,
	LR_STARTUP,
	LR_WELLCOME,
	LR_ERROR,
	LR_ERROR_INIT,
	LR_ERROR_NET,
	LR_ERROR_FS,
	LR_ERROR_SOUND,
	LR_ERROR_LIBMAD,
	LR_RADIO,
	LR_FILE,
	LR_PLAY,
	LR_STOP,

	LR_VOLUME_00,
	LR_VOLUME_01,
	LR_VOLUME_02,
	LR_VOLUME_03,
	LR_VOLUME_04,
	LR_VOLUME_05,
	LR_VOLUME_06,
	LR_VOLUME_07,
	LR_VOLUME_08,
	LR_VOLUME_09,
	LR_VOLUME_10,
	LR_VOLUME_11,
	LR_VOLUME_12,
	LR_VOLUME_13,
	LR_VOLUME_14,
	LR_VOLUME_15,
};

enum lcd_ss_e {
	LSS_BLANK = 0,
	LSS_INIT,
	LSS_WELLCOME,
};

enum lcd_err_e {
	LER_COMMON = 0,
	LER_INIT,
	LER_NET,
	LER_FS,
	LER_SOUND,
	LER_LIBMAD,
};

struct lcd_cfg_s {
	uint16_t	width;
	uint16_t	height;
	uint8_t		bpp;
};

void lcd_init(void);
void lcd_poll(void);
void lcd_done(void);

void lcd_startup(enum lcd_ss_e stage);
void lcd_play(void);
void lcd_stop(void);
void lcd_error(enum lcd_err_e error);
void lcd_volume(void);
void lcd_reset(void);

void lcd_get_cfg(struct lcd_cfg_s *cfg);

void lcd_putpixel(uint16_t x, uint16_t y, uint8_t color);
void lcd_update_screen(void);
void lcd_clean_screen(void);

#endif
