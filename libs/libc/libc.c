#include "xprintf.h"

void abort(void) {
	xprintf("abort\n");
	while (1);
}

int q3_atoi(const char *string) {
	int	sign;
	int	value;
	int	c;

	// skip whitespace
	while (*string <= ' ') {
		if (!*string) {
			return 0;
		}
		string++;
	}

	// check sign
	switch (*string) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	do {
		c = *string++;
		if (c < '0' || c > '9') {
			break;
		}
		c -= '0';
		value = value * 10 + c;
	} while (1);

	// not handling 10e10 notation...

	return value * sign;
}
