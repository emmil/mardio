#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"

#include "console.h"
#include "libc.h"
#include "iobuffer.h"
#include "event.h"
#include "player.h"
#include "platform.h"
#include "cmd.h"
#include "libmad.h"
#include "stm32.h"
#include "allocs.h"
#include "../common/version.h"

void init(void) {
	RCC_ClocksTypeDef	RCC_Clocks;

	STM_EVAL_LEDInit(LED_FS);
	STM_EVAL_LEDInit(LED_SYSTEM);
	STM_EVAL_LEDInit(LED_SOUND);
	STM_EVAL_LEDInit(LED_NET);

	STM_EVAL_LEDOff(LED_FS);
	STM_EVAL_LEDOff(LED_SYSTEM);
	STM_EVAL_LEDOff(LED_SOUND);
	STM_EVAL_LEDOff(LED_NET);

	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);

	con_init();

	xprintf("\n\n==== Init start ====\n");

	lcd_init();

	lcd_startup(LSS_BLANK);
	lcd_startup(LSS_INIT);

	rng_init();
	io_init();
	lma_init();
	event_init();
	cmd_init();
	fs_init();
	net_init();
	snd_init();
	pl_init();
//	usb_init ();

	lcd_startup(LSS_WELLCOME);

	version();

	xprintf("==== Init done ====\n\n");

	STM_EVAL_LEDOn(LED_SYSTEM);

	con_input_unlock();
	con_prompt();
}

void frame(void) {

	con_poll();
//	usb_loop ();
	fs_poll();
	net_poll();
	libmad_loop();
	snd_loop();
	pl_poll();
	event_loop();
	lcd_poll();
}

int main(void) {

	init();

	while (1) {
		frame();
		net_timers();
	}

	return 0;
}
