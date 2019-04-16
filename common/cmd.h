#ifndef	CMD_H
#define	CMD_H

#define	CMD_INPUT_BUFFER_SIZE	MAX_CONSOLE_INPUT
#define	MAX_TOKENS		16

#define	NULL_STR		""

typedef void (*xcommand_t)(void);

struct command_s {
	char		*name;
	xcommand_t	function;
};

int cmd_Argc(void);
const char *cmd_Argv(int i);
void cmd_init(void);
void cmd_input(void);

#endif
