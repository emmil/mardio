#ifndef	IOBUFFER_H
#define	IOBUFFER_H

#include <stdint.h>

#include "snd.h"

#define	MAX_INPUT_BUFFERS	4
#define	MAX_OUTPUT_BUFFERS	4

#define	IO_INPUT_BUFFER_SIZE	1024
#define	IO_OUTPUT_BUFFER_SIZE	4608

struct input_s {
	uint8_t		data[IO_INPUT_BUFFER_SIZE];
	uint16_t	size;
	uint16_t	used;
};

struct output_s {
	uint8_t			data[IO_OUTPUT_BUFFER_SIZE];
	uint16_t		size;
	uint16_t		used;
	enum sound_rate_e	rate;
	int			channels;
};

void io_init(void);

struct input_s *i_push(void);
struct input_s *i_pop(void);
uint16_t i_free(void);
uint16_t i_used(void);

struct output_s *o_push(void);
struct output_s *o_pop(void);
struct output_s *o_peek(void);
uint16_t o_free(void);
uint16_t o_used(void);
uint16_t o_free_size(void);

uint8_t io_starving(void);

#endif
