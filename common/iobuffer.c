#include <stdint.h>

#include "iobuffer.h"
#include "libc.h"

#ifdef	CONFIG_IOBUFFER_DEBUG
#define	io_debug(...)	xprintf (__VA_ARGS__)
#else
#define	io_debug(...)
#endif

static struct io_s {
	const char	*name;
	uint32_t	head;
	uint32_t	tail;
	uint16_t	max_buffers;
} i, o;

struct input_s	input[MAX_INPUT_BUFFERS];
struct output_s	output[MAX_OUTPUT_BUFFERS];

const char	input_str[] = "input";
const char	output_str[] = "output";

///////////////////////////////////////

static uint16_t io_used(struct io_s *io) {
	return (io->head - io->tail);
}

static uint16_t io_free(struct io_s *io) {
	return (io->max_buffers - (io->head - io->tail));
}

static void io_input_reset_buffer(struct input_s *in) {

	uc_memset(in, 0, sizeof(struct input_s));
	in->size = IO_INPUT_BUFFER_SIZE;
}

static void io_output_reset_buffer(struct output_s *out) {

	uc_memset(out, 0, sizeof(struct output_s));
	out->size = IO_OUTPUT_BUFFER_SIZE;
}

void io_reset_input(void) {
	int	index;

	uc_memset(&i, 0, sizeof(i));
	i.max_buffers = MAX_INPUT_BUFFERS;
	i.name = input_str;

	for (index = 0; index < i.max_buffers; index++) {
		io_input_reset_buffer(&input[index]);
	}
}

void io_reset_output(void) {
	int	index;

	uc_memset(&o, 0, sizeof(o));
	o.max_buffers = MAX_OUTPUT_BUFFERS;
	o.name = output_str;

	for (index = 0; index < o.max_buffers; index++) {
		io_output_reset_buffer(&output[index]);
	}
}

void io_init(void) {

	io_debug("io_init\n");

	io_reset_input();
	io_reset_output();

	io_debug("    Input: %d buffers, each %d, total %dKB\n",
		 i.max_buffers,
		 input[0].size,
		 i.max_buffers * input[0].size / 1024);

	io_debug("    Output: %d buffers, each %d, total %dKB\n",
		 o.max_buffers,
		 output[0].size,
		 o.max_buffers * output[0].size / 1024);
}

uint16_t io_push(struct io_s *io) {
	uint16_t	index;

//	io_debug ("io_push (%s)\n", io->name);

	index = io->head % io->max_buffers;
	if (io->head - io->tail > io->max_buffers) {
		xprintf("io_push: io buffer '%s' is full, loosing data.\n",
			io->name);
		io->tail++;
	}

	io->head++;

	return (index);
}

uint16_t io_pop(struct io_s * io) {
	uint16_t	index;

//	io_debug ("io_pop (%s)\n", io->name);

	if (io->head > io->tail) {
		index = io->tail % io->max_buffers;
		io->tail++;

		return (index);
	}

	io_debug("io_pop: popping empty buffer.\n");

	return 0xFFFF;
}

///////////////////////////////////////

struct input_s *i_push(void) {
	uint16_t	index;

	index = io_push(&i);
	io_input_reset_buffer(&input[index]);
	return (&input[index]);
}

struct input_s *i_pop(void) {
	uint16_t	index;

	index = io_pop(&i);

	if (index == 0xFFFF) {
		io_debug("i_pop: popping null buffer.\n");
		return NULL;
	}

	return (&input[index]);
}

uint16_t i_free(void) {
	return (io_free(&i));
}

uint16_t i_used(void) {
	return (io_used(&i));
}

///////////////////////////////////////

struct output_s *o_push(void) {
	uint16_t	index;

	index = io_push(&o);
	io_output_reset_buffer(&output[index]);
	return (&output[index]);
}

struct output_s *o_pop(void) {
	uint16_t	index;

	index = io_pop(&o);

	if (index == 0xFFFF) {
		io_debug("o_pop: NULL!\n");
		return NULL;
	}

	return (&output[index]);
}

struct output_s *o_peek(void) {
	uint16_t	index;

//	io_debug ("o_peek\n");

	if (o.head > o.tail) {
		index = o.tail % o.max_buffers;

		return (&output[index]);
	}

	io_debug("o_peek: popping empty buffer.\n");

	return NULL;
}

uint16_t o_free(void) {
	return (io_free(&o));
}

uint16_t o_free_size(void) {
	return (io_free(&o) * IO_OUTPUT_BUFFER_SIZE);
}

uint16_t o_used(void) {
	return (io_used(&o));
}

///////////////////////////////////////

uint8_t io_starving(void) {

	if (i_used() == 0 && o_used() == 0)
		return 1;

	return 0;
}

///////////////////////////////////////
// Command line interface

void cmd_io_state(void) {

	xprintf("Input: Free %d/%d buffers.\n", io_free(&i), i.max_buffers);
	xprintf("Output: Free %d/%d buffers.\n", io_free(&o), o.max_buffers);

	xprintf("\n");
}
