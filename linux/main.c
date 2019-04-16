#include <stdio.h>

#include "libc.h"
#include "cmd.h"
#include "console.h"
#include "event.h"
#include "player.h"
#include "iobuffer.h"
#include "platform.h"
#include "libmad.h"
#include "version.h"

char	quit = 0;

void cmd_exit(void) {
	xprintf("Exiting ... \n\n");
	quit = 1;
}

void init(void) {
	printf ("\n\n=== Init Start ===\n");

	con_init();
	lcd_init();

	lcd_startup(LSS_BLANK);
	lcd_startup(LSS_INIT);

	io_init();
	event_init();
	cmd_init();
	fs_init();
	net_init();
	snd_init();
	pl_init();

	lcd_startup(LSS_WELLCOME);

	version();

	xprintf("=== Init Done ===\n\n");

	con_input_unlock();
	con_prompt();
	fflush(stdout);
}

void done(void) {
	xprintf("\n\n");

	xprintf("=== Shutdown ===\n");

	net_done();
	fs_done();
	snd_done();
	lcd_done();

	xprintf("=== Done ===\n\n");
}

void frame(void) {

	con_poll();
	fs_poll();
	net_poll();
	libmad_loop();
	snd_loop();
	event_loop();
	pl_poll();
	lcd_poll();

	fflush(stdout);
}

void run(void) {

	while (!quit) {

		frame();
	}
}

int main(void) {

	init();
	run();
	done();

	return 0;
}
