#include "platform.h"
#include "libc.h"

#include "iobuffer.h"
#include "event.h"
#include "allocs.h"
#include "cmd.h"
#include "player.h"
#include "version.h"
#include "libmad.h"

char quit = 0;

void null_init(void) {

	con_init();

	xprintf("\n\n==== Init start ====\n");

	lcd_init();

	lcd_startup(LSS_BLANK);
	lcd_startup(LSS_INIT);

	io_init();
	lma_init();
	event_init();
	cmd_init();
	fs_init();
	net_init();
	snd_init();
	pl_init();

	lcd_startup(LSS_WELLCOME);

	version();

	xprintf("==== Init done ====\n\n");

	con_input_unlock();
	con_prompt();
}

void null_frame(void) {
	con_poll();
	fs_poll();
	net_poll();
	libmad_loop();
	snd_loop();
	event_loop();
	pl_poll();
	lcd_poll();
}

void null_done(void) {

	net_done();
	fs_done();
	snd_done();
	lcd_done();

	xprintf("=== Done ===\n\n");
}

int main(void) {

	null_init();

	while (!quit) {
		null_frame();
	}

	null_done();

	return 0;
}
