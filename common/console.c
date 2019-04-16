#include "console.h"
#include "platform.h"

#include "libc.h"
#include "event.h"

const char PROMPT[] = "] ";

struct {
	uint8_t		lock;
	uint16_t	index;
	char		input[MAX_CONSOLE_INPUT];
} static con;


void con_reset(void) {
	uc_memset(&con.input, 0, sizeof(con.input));
	con.index = 0;
	con_input_unlock();
}

void con_init(void) {
	uc_memset(&con, 0, sizeof(con));
	host_con_init();
}

void con_poll(void) {
	host_con_poll();
}

void con_prompt(void) {
	xprintf("%s", PROMPT);
}

void con_input_lock(void) {
	con.lock = 1;
}

void con_input_unlock(void) {
	con.lock = 0;
}

uint8_t con_input_islocked(void) {
	return con.lock;
}

uint16_t con_input_hasinput(void) {
	return con.index;
}

void con_input_push(char ch) {

	if (con.lock || con.index >= MAX_CONSOLE_INPUT)
		return;

	con.input[con.index] = ch;
	con.index++;

}

void con_input_del(void) {

	if (con.index) {
		con.index--;
		con.input[con.index] = 0x00;
	}
}

const char *con_input_data(void) {
	return con.input;
}

char con_input_echo(void) {

	if (con.index) {
		return con.input[con.index - 1];
	}

	return 0;
}
