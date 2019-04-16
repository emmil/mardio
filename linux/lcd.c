#include <SDL/SDL.h>

#include "libc.h"
#include "platform.h"

#ifdef	CONFIG_LCD_DEBUG
#define	sdl_debug(...)	xprintf(__VA_ARGS__)
#else
#define	sdl_debug(...)
#endif

/*

https://www.libsdl.org/release/SDL-1.2.15/docs/html/guidevideo.html
https://wiki.libsdl.org/SDL_Surface?highlight=%28%5CbCategoryStruct%5Cb%29%7C%28SDLStructTemplate%29

*/

#define	LCD_FACTOR	4

#define	LCD_WIDTH	(84*LCD_FACTOR)
#define	LCD_HEIGHT	(48*LCD_FACTOR)
#define	LCD_BPP		8	/* Bits per pixel */

enum lcd_state_e {
	LCDS_UNKNOWN = 0,
	LCDS_INIT,
	LCDS_ERROR
};

struct {
	SDL_Surface		*screen;
	enum lcd_state_e	state;
	uint16_t		width;
	uint16_t		height;
} static lcd;

void putpixel(SDL_Surface * surface, int x, int y, uint32_t pixel) {
	int	bpp = surface->format->BytesPerPixel;

	/* Here p is the address to the pixel we want to set */
	uint8_t	*p = (uint8_t *) surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(uint16_t *) p = pixel;
		break;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(uint32_t *) p = pixel;
		break;
	}
}

void display_bmp(char *file_name) {
	SDL_Surface	*image;

	image = SDL_LoadBMP(file_name);
	if (image == NULL) {
		xprintf("Couldn't load %s: %s\n", file_name, SDL_GetError());
		return;
	}

	if (image->format->palette && lcd.screen->format->palette) {
		SDL_SetColors(lcd.screen, image->format->palette->colors, 0,
			      image->format->palette->ncolors);
	}

	if (SDL_BlitSurface(image, NULL, lcd.screen, NULL) < 0)
		xprintf("BlitSurface error: %s\n", SDL_GetError());

	SDL_UpdateRect(lcd.screen, 0, 0, image->w, image->h);

	SDL_FreeSurface(image);
}


void host_lcd_init(void) {
	SDL_version	compiled;
	int		ret;

	uc_memset(&lcd, 0, sizeof(lcd));

	lcd.screen = NULL;
	lcd.width = LCD_WIDTH;
	lcd.height = LCD_HEIGHT;

	SDL_VERSION(&compiled);

	sdl_debug("%s: SDL version %d.%d.%d\n",
		  __func__, compiled.major, compiled.minor, compiled.patch);
	sdl_debug("%s: (%dx%d) ", __func__, LCD_WIDTH, LCD_HEIGHT);

	ret = SDL_Init(SDL_INIT_VIDEO);
	if (ret != 0) {
		sdl_debug("failed (init)\n");
		lcd.state = LCDS_ERROR;
		return;
	}

	lcd.screen = SDL_SetVideoMode(lcd.width, lcd.height, LCD_BPP, SDL_SWSURFACE);
	if (lcd.screen == NULL) {
		sdl_debug("failed (set mode) '%s'\n", SDL_GetError());
		lcd.state = LCDS_ERROR;
		return;
	}

	SDL_FillRect(lcd.screen, NULL,
		     SDL_MapRGB(lcd.screen->format, 255, 255, 255));

	SDL_UpdateRect(lcd.screen, 0, 0, lcd.width, lcd.height);

	sdl_debug("OK\n");
	lcd.state = LCDS_INIT;
}

void host_lcd_done(void) {
	SDL_Quit();
	sdl_debug("%s\n", __func__);
}

void host_lcd_draw(enum lcd_resource_e resource) {

	if (lcd.state == LCDS_ERROR)
		return;

	sdl_debug("%s: %d ", __func__, resource);


	switch (resource) {
	case LR_BLANK:
		sdl_debug("blank");
		display_bmp("libs/artwork/bmp/blank.bmp");
		break;

	case LR_STARTUP:
		sdl_debug("startup");
		display_bmp("libs/artwork/bmp/boot.bmp");
		break;

	case LR_WELLCOME:
		sdl_debug("wellcome");
		display_bmp("libs/artwork/bmp/wellcome.bmp");
		break;

	case LR_ERROR:
		sdl_debug("error");
		display_bmp("libs/artwork/bmp/error.bmp");
		break;

	case LR_ERROR_INIT:
		display_bmp("libs/artwork/bmp/error_init.bmp");
		break;

	case LR_ERROR_NET:
		display_bmp("libs/artwork/bmp/error_net.bmp");
		break;

	case LR_ERROR_FS:
		display_bmp("libs/artwork/bmp/error_file.bmp");
		break;

	case LR_ERROR_SOUND:
		display_bmp("libs/artwork/bmp/error_snd.bmp");
		break;

	case LR_ERROR_LIBMAD:
		display_bmp("libs/artwork/bmp/error_mp3.bmp");
		break;

	case LR_RADIO:
		sdl_debug("radio");
		display_bmp("libs/artwork/bmp/radio.bmp");
		break;

	case LR_FILE:
		sdl_debug("file");
		display_bmp("libs/artwork/bmp/file.bmp");
		break;

	case LR_PLAY:
		sdl_debug("play");
		display_bmp("libs/artwork/bmp/play.bmp");
		break;

	case LR_STOP:
		sdl_debug("stop");
		display_bmp("libs/artwork/bmp/stop.bmp");
		break;

	case LR_VOLUME_00:
		display_bmp("libs/artwork/bmp/volume_00.bmp");
		break;

	case LR_VOLUME_01:
		display_bmp("libs/artwork/bmp/volume_01.bmp");
		break;

	case LR_VOLUME_02:
		display_bmp("libs/artwork/bmp/volume_02.bmp");
		break;

	case LR_VOLUME_03:
		display_bmp("libs/artwork/bmp/volume_03.bmp");
		break;

	case LR_VOLUME_04:
		display_bmp("libs/artwork/bmp/volume_04.bmp");
		break;

	case LR_VOLUME_05:
		display_bmp("libs/artwork/bmp/volume_05.bmp");
		break;

	case LR_VOLUME_06:
		display_bmp("libs/artwork/bmp/volume_06.bmp");
		break;

	case LR_VOLUME_07:
		display_bmp("libs/artwork/bmp/volume_07.bmp");
		break;

	case LR_VOLUME_08:
		display_bmp("libs/artwork/bmp/volume_08.bmp");
		break;

	case LR_VOLUME_09:
		display_bmp("libs/artwork/bmp/volume_09.bmp");
		break;

	case LR_VOLUME_10:
		display_bmp("libs/artwork/bmp/volume_10.bmp");
		break;

	case LR_VOLUME_11:
		display_bmp("libs/artwork/bmp/volume_11.bmp");
		break;

	case LR_VOLUME_12:
		display_bmp("libs/artwork/bmp/volume_12.bmp");
		break;

	case LR_VOLUME_13:
		display_bmp("libs/artwork/bmp/volume_13.bmp");
		break;

	case LR_VOLUME_14:
		display_bmp("libs/artwork/bmp/volume_14.bmp");
		break;

	case LR_VOLUME_15:
		display_bmp("libs/artwork/bmp/volume_15.bmp");
		break;
	}
	sdl_debug("\n");
}

void host_lcd_putpixel(uint16_t x, uint16_t y, uint8_t color) {
	if (x < lcd.width && y < lcd.height)
		putpixel(lcd.screen, x, y, color);
}

void host_lcd_update_screen(void) {
	SDL_UpdateRect(lcd.screen, 0, 0, lcd.width, lcd.height);
}

void host_lcd_clean_screen(void) {
	SDL_FillRect(lcd.screen, NULL,
		     SDL_MapRGB(lcd.screen->format, 255, 255, 255));
}

void host_lcd_get_cfg(struct lcd_cfg_s *cfg) {

	if (cfg != NULL) {
		cfg->width = lcd.width;
		cfg->height = lcd.height;
		cfg->bpp = LCD_BPP;
	}
}
