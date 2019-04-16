#ifndef	CONSOLE_H
#define	CONSOLE_H

#include <stdint.h>

#define	MAX_CONSOLE_INPUT	128

void con_init(void);
void con_poll(void);
void con_reset(void);

void con_prompt(void);

void con_input_lock(void);
void con_input_unlock(void);
uint8_t con_input_islocked(void);
uint16_t con_input_hasinput(void);
void con_input_push(char ch);
void con_input_del(void);
const char *con_input_data(void);
char con_input_echo(void);

#endif
