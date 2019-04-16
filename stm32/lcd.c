#include <stdint.h>

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx.h"

#include "libc.h"
#include "stm32.h"
#include "platform.h"
#include "player.h"
#include "iobuffer.h"

#include "gfx.h"

#ifdef	CONFIG_LCD_DEBUG
#define	lcd_debug(...)	xprintf(__VA_ARGS__)
#else
#define	lcd_debug(...)
#endif

#define PCD8544_WIDTH				84
#define PCD8544_HEIGHT				48

struct {
	unsigned char	data[PCD8544_WIDTH * PCD8544_HEIGHT / 8];
	unsigned char	min_x, min_y, max_x, max_y;
} pcd;

//////// pcb8544 functions ///////////////////

/*

| PIN | Function | Group | Note |
---------------------------------
| PB3  | CLK     | SPI1  |      |
| PB4  | MISO    | SPI1  | NC   |
| PB5  | MOSI    | SPI1  |      |
| PC13 | CE      | GPIOC |      |
| PC14 | D/C     | GPIOC |      |
| PC15 | RST     | GPIOC |      |

*/

#define	PIN_CE	GPIO_Pin_13
#define	PIN_DC	GPIO_Pin_14
#define	PIN_RST	GPIO_Pin_15

// General commands
#define PCD8544_POWERDOWN		0x04
#define PCD8544_ENTRYMODE		0x02
#define PCD8544_EXTENDEDINSTRUCTION	0x01
#define PCD8544_DISPLAYBLANK		0x00
#define PCD8544_DISPLAYNORMAL		0x04
#define PCD8544_DISPLAYALLON		0x01
#define PCD8544_DISPLAYINVERTED		0x05
// Normal instruction set
#define PCD8544_FUNCTIONSET		0x20
#define PCD8544_DISPLAYCONTROL		0x08
#define PCD8544_SETYADDR		0x40
#define PCD8544_SETXADDR		0x80
// Extended instruction set
#define PCD8544_SETTEMP			0x04
#define PCD8544_SETBIAS			0x10
#define PCD8544_SETVOP			0x80
// Display presets
#define PCD8544_LCD_BIAS		0x03	// Range: 0-7 (0x00-0x07)
#define PCD8544_LCD_TEMP		0x02	// Range: 0-3 (0x00-0x03)
#define PCD8544_LCD_CONTRAST		0x46	// Range: 0-127 (0x00-0x7F)

enum pcd8544_mode {
	PCD_DATA,
	PCD_COMMAND
};

void pcd8544_init_hw(void) {
	GPIO_InitTypeDef	GPIO_InitStruct;
	SPI_InitTypeDef		SPI_InitStruct;

	//Enable clock for all pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	//Common settings for all pins
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;

	// GPIO pin
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Pin = PIN_RST | PIN_DC | PIN_CE;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	// SPI pins
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// Reset pin HIGH
	GPIO_SetBits(GPIOC, PIN_RST);

	// CE HIGH
	GPIO_SetBits(GPIOC, PIN_CE);

	// Initialize SPI
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);	// clk
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);	// miso
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);	// mosi

	SPI_StructInit(&SPI_InitStruct);
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI1, &SPI_InitStruct);
	SPI_Cmd(SPI1, ENABLE);
}

void pcd8544_reset(void) {
	GPIO_ResetBits(GPIOC, PIN_RST);
	Delay(100);
	GPIO_SetBits(GPIOC, PIN_RST);
}

void pcd8544_setmode(enum pcd8544_mode mode) {

	switch (mode) {

	case PCD_DATA:
		GPIO_SetBits(GPIOC, PIN_DC);
		break;

	case PCD_COMMAND:
		GPIO_ResetBits(GPIOC, PIN_DC);
		break;

	}
}

void pcd8544_write(const unsigned char data) {

	GPIO_ResetBits(GPIOC, PIN_CE);

	SPI1->DR = data;						//Fill output buffer with data
	while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));		//Wait for transmission to complete
	while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE));	//Wait for received data to complete
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY));		//Wait for SPI to be ready

	GPIO_SetBits(GPIOC, PIN_CE);
}

void pcd8544_setcontrast(const unsigned char contrast) {
	unsigned char	tmp = contrast;

	if (tmp > 0x7F) {
		tmp = 0x7F;
	}
	pcd8544_write(PCD8544_SETVOP | tmp);
}

void pcd8544_invert(const unsigned char val) {

	if (val)
		pcd8544_write(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYINVERTED);
	else
		pcd8544_write(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);
}

void pcd8544_updatebox(unsigned char new_min_x, unsigned char new_min_y,
		       unsigned char new_max_x, unsigned char new_max_y) {

	if (new_min_x < pcd.min_x)
		pcd.min_x = new_min_x;

	if (new_min_y < pcd.min_y)
		pcd.min_x = new_min_y;

	if (new_max_x > pcd.max_x)
		pcd.max_x = new_max_x;

	if (new_max_y > pcd.max_y)
		pcd.max_y = new_max_y;

}

void pcd8544_putpixel(const unsigned char x, const unsigned char y,
		      const unsigned char color) {

	if (x >= PCD8544_WIDTH) {
		return;
	}
	if (y >= PCD8544_HEIGHT) {
		return;
	}

	if (color) {
		pcd.data[x + (y / 8) * PCD8544_WIDTH] |= 1 << (y % 8);
	} else {
		pcd.data[x + (y / 8) * PCD8544_WIDTH] &= ~(1 << (y % 8));
	}

	pcd8544_updatebox(x, y, x, y);
}

void pcd8544_putpixel_lazy(const unsigned char x, const unsigned char y,
			   const unsigned char color) {

	if (x >= PCD8544_WIDTH) {
		return;
	}
	if (y >= PCD8544_HEIGHT) {
		return;
	}

	if (color) {
		pcd.data[x + (y / 8) * PCD8544_WIDTH] |= 1 << (y % 8);
	} else {
		pcd.data[x + (y / 8) * PCD8544_WIDTH] &= ~(1 << (y % 8));
	}

}

void pcd8544_update(void) {
	unsigned char	i, j;

	for (i = 0; i < 6; i++) {

		if (pcd.min_y > ((i + 1) * 8))
			continue;

		if ((i * 8) > pcd.max_y)
			break;

		pcd8544_setmode(PCD_COMMAND);
		pcd8544_write(PCD8544_SETYADDR | i);
		pcd8544_write(PCD8544_SETXADDR | pcd.min_x);

		pcd8544_setmode(PCD_DATA);

		for (j = pcd.min_x; j <= pcd.max_x; j++)
			pcd8544_write(pcd.data[(i * PCD8544_WIDTH) + j]);

	}

	pcd8544_updatebox(0, 0, PCD8544_WIDTH - 1, PCD8544_HEIGHT - 1);
}

void pcd8544_clean(void) {
	uc_memset(&pcd, 0, sizeof(pcd));
	pcd8544_update();
}

void pcd8544_init(unsigned char contrast) {

	pcd8544_init_hw();
	pcd8544_reset();
	pcd8544_setmode(PCD_COMMAND);
	pcd8544_write(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION);	// Go to extended mode
	pcd8544_write(PCD8544_SETBIAS | 0x4);					// LCD bias select
	pcd8544_setcontrast(contrast);						// set VOP
	pcd8544_write(PCD8544_FUNCTIONSET);					// normal mode
	pcd8544_write(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);		// Set display to Normal
	pcd8544_clean();
}

void pcd8544_draw_resource(const uint8_t * data) {
	uc_memcpy(pcd.data, data, sizeof(pcd.data));
}

//////// LCD api callbacks ///////////////////

void host_lcd_init(void) {
	pcd8544_init(0x38);
	pcd8544_update();
}

void host_lcd_done(void) {
}

void host_lcd_putpixel(uint16_t x, uint16_t y, uint8_t color) {
	if (x < PCD8544_WIDTH && y < PCD8544_HEIGHT)
		pcd8544_putpixel_lazy(x, y, !!color);
}

void host_lcd_update_screen(void) {
	pcd8544_update();
}

void host_lcd_clean_screen(void) {
	uc_memset(&pcd, 0, sizeof(pcd));
}

void host_lcd_get_cfg(struct lcd_cfg_s *cfg) {

	if (cfg != NULL) {
		cfg->width = PCD8544_WIDTH;
		cfg->height = PCD8544_HEIGHT;
		cfg->bpp = 1;
	}
}

void host_lcd_draw(enum lcd_resource_e resource) {

	lcd_debug("host_lcd_draw (resource = %d)\n", resource);

	pcd8544_clean();

	switch (resource) {
	case LR_BLANK:
		break;

	case LR_STARTUP:
		pcd8544_draw_resource(startup);
		break;

	case LR_WELLCOME:
		pcd8544_draw_resource(wellcome);
		break;

	case LR_ERROR:
		pcd8544_draw_resource(error);
		break;

	case LR_ERROR_INIT:
		pcd8544_draw_resource(error_init);
		break;

	case LR_ERROR_NET:
		pcd8544_draw_resource(error_net);
		break;

	case LR_ERROR_FS:
		pcd8544_draw_resource(error_fs);
		break;

	case LR_ERROR_SOUND:
		pcd8544_draw_resource(error_sound);
		break;

	case LR_ERROR_LIBMAD:
		pcd8544_draw_resource(error_libmad);
		break;

	case LR_RADIO:
		pcd8544_draw_resource(radio);
		break;

	case LR_FILE:
		pcd8544_draw_resource(file);
		break;

	case LR_PLAY:
		pcd8544_draw_resource(play);
		break;

	case LR_STOP:
		pcd8544_draw_resource(stop);
		break;

	case LR_VOLUME_00:
		pcd8544_draw_resource(volume_00);
		break;

	case LR_VOLUME_01:
		pcd8544_draw_resource(volume_01);
		break;

	case LR_VOLUME_02:
		pcd8544_draw_resource(volume_02);
		break;

	case LR_VOLUME_03:
		pcd8544_draw_resource(volume_03);
		break;

	case LR_VOLUME_04:
		pcd8544_draw_resource(volume_04);
		break;

	case LR_VOLUME_05:
		pcd8544_draw_resource(volume_05);
		break;

	case LR_VOLUME_06:
		pcd8544_draw_resource(volume_06);
		break;

	case LR_VOLUME_07:
		pcd8544_draw_resource(volume_07);
		break;

	case LR_VOLUME_08:
		pcd8544_draw_resource(volume_08);
		break;

	case LR_VOLUME_09:
		pcd8544_draw_resource(volume_09);
		break;

	case LR_VOLUME_10:
		pcd8544_draw_resource(volume_10);
		break;

	case LR_VOLUME_11:
		pcd8544_draw_resource(volume_11);
		break;

	case LR_VOLUME_12:
		pcd8544_draw_resource(volume_12);
		break;

	case LR_VOLUME_13:
		pcd8544_draw_resource(volume_13);
		break;

	case LR_VOLUME_14:
		pcd8544_draw_resource(volume_14);
		break;

	case LR_VOLUME_15:
		pcd8544_draw_resource(volume_15);
		break;
	}

	pcd8544_update();
}

//////////////////////////////////////////////////
