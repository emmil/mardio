#include <stdint.h>
#include <stdio.h>

#include "platform.h"

void host_lcd_init(void) {
}

void host_lcd_done(void) {
}

void host_lcd_draw(enum lcd_resource_e resource) {
}

void host_lcd_get_cfg(struct lcd_cfg_s *cfg) {

	if (cfg != NULL) {
		cfg->width = 0;
		cfg->height = 0;
		cfg->bpp = 0;
	}
}

void host_lcd_putpixel(uint16_t x, uint16_t y, uint8_t color) {
}

void host_lcd_update_screen(void) {
}

void host_lcd_clean_screen(void) {
}
