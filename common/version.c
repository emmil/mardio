#include "version.h"
#include "libc.h"

void version (void) {
	xprintf ("%s %s %s\n",
		MARDIO_STR,
		MARDIO_VER,
		MARDIO_OPT);
}

///// Command line stuff /////////

void cmd_version (void) {

	version ();
	xprintf ("\n");
}
