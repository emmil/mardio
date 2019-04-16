#ifndef PLATFORM_H
#define	PLATFORM_H

#include "lcd.h"
#include "snd.h"
#include "net.h"
#include "fs.h"

/* Timer interface */
uint32_t get_local_time(void);

/* Console interface */
void host_con_init(void);
void host_con_poll(void);

/* File system interface */
void host_fs_init(void);
void host_fs_done(void);
int host_fs_open(const char *name);
void host_fs_close(int handle);
uint32_t host_fs_read(int handle, void *buffer, uint32_t size);
void host_fs_seek(int handle, uint32_t offset);

/* Network interface */
void host_net_init(void);
void host_net_done(void);
void host_net_open(const char *url);
void host_net_close(void);
void host_net_poll(void);
enum net_state_e host_net_state(void);

/* Sound interface */
void host_snd_init(void);
void host_snd_done(void);
void host_snd_loop(void);
enum sound_state_e host_snd_state(void);

/* LCD interface */
void host_lcd_init(void);
void host_lcd_done(void);
void host_lcd_draw(enum lcd_resource_e resource);
void host_lcd_get_cfg(struct lcd_cfg_s *cfg);
void host_lcd_putpixel(uint16_t x, uint16_t y, uint8_t color);
void host_lcd_update_screen(void);
void host_lcd_clean_screen(void);

#endif
