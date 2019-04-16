#include "libc.h"

#include "cmd.h"
#include "console.h"
#include "commands.h"

struct {
	char	token_buffer[CMD_INPUT_BUFFER_SIZE + MAX_TOKENS];
	int	cmd_argc;
	char	*cmd_argv[MAX_TOKENS];
} static cmd;

void cmd_init(void) {
	int	i;

	uc_memset(&cmd, 0, sizeof(cmd));
	for (i = 0; i < MAX_TOKENS; i++)
		cmd.cmd_argv[i] = NULL;
}

int cmd_Argc(void) {
	return cmd.cmd_argc;
}

const char *cmd_Argv(int i) {

	if (cmd.cmd_argc == 0 || i >= cmd.cmd_argc)
		return NULL_STR;

	return cmd.cmd_argv[i];
}

void cmd_debug(void) {
	int	i;

	xprintf("cmd_argc: %d\n", cmd.cmd_argc);

	for (i = 0; i < cmd.cmd_argc; i++) {
		xprintf("%d : '%s' (%d)\n",
			i,
			cmd.cmd_argv[i],
			uc_strlen(cmd.cmd_argv[i]));
	}
}


void cmd_tokenize(const char *text_in) {
	const char	*text;
	char		*text_out;

	cmd.cmd_argc = 0;

	if (!text_in)
		return;

	uc_memset(cmd.token_buffer, 0, sizeof cmd.token_buffer);

	text = text_in;
	text_out = cmd.token_buffer;

	while (1) {
		if (cmd.cmd_argc == MAX_TOKENS) {
			return;
		}

		// C++ style comments '//'
		if (text[0] == '/' && text[1] == '/')
			return;

		// bash style comments '#'
		if (*text == '#')
			return;

		while (*text == ' ') {
			text++;
		}

		if (*text == '\n') {
			return;
		}

		if (*text == 13) {
			return;
		}

		if (!*text) {
			return;
		}

		if (*text == 0x00) {
			return;
		}

		if (*text == '"') {
			cmd.cmd_argv[cmd.cmd_argc] = text_out;
			cmd.cmd_argc++;
			text++;
			while (*text != '"' && *text) {
				*text_out++ = *text++;
			}
			*text_out++ = 0;
			if (!*text) {
				return;
			}
			text++;
			continue;
		}

		cmd.cmd_argv[cmd.cmd_argc] = text_out;
		cmd.cmd_argc++;

		while (*text > ' ') {
			*text_out++ = *text++;
		}

		*text_out++ = 0;

		if (!*text) {
			return;
		}
	}
}

void cmd_exec(void) {
	int	index;

	if (!cmd_Argc())
		return;

	for (index = 0; commands[index].function != NULL; index++) {
		if (!uc_memcmp
		    (commands[index].name, cmd_Argv(0), uc_strlen(cmd_Argv(0)))
		    && !uc_memcmp(commands[index].name, cmd_Argv(0),
				  uc_strlen(commands[index].name))) {

			commands[index].function();

			return;
		}
	}

	xprintf("'%s' command not found.\n\n", cmd_Argv(0));
}

void cmd_input(void) {
	cmd_tokenize(con_input_data());
//	cmd_debug ();
	cmd_exec();
}

///////////// Build in commands  ////////////////

void cmd_help(void) {
	int	index;

	xprintf("List of known commands: \n");
	for (index = 0; commands[index].function != NULL; index++) {
		xprintf("%s ", commands[index].name);
	}
	xprintf("\n\n");
}

void cmd_echo(void) {
	int	index;

	for (index = 1; index < cmd_Argc(); index++)
		xprintf("%s ", cmd_Argv(index));

	xprintf("\n");
}

void cmd_cls(void) {
	xprintf("\033[2J");
	xprintf("\033[1;1H");
}
