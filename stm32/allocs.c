#include "salloc.h"

static uint8_t	libmad_pool [3*1024 + 5*1024];

static struct salloc_s libmad_alloc = {
	.name = "libmad",
	.data = libmad_pool,
	.data_size = sizeof (libmad_pool),
	.slot_count = 2,
	.used_slots = 0,
	.slots = {
		SA_ENTRY (3*1024),
		SA_ENTRY (5*1024),
	},
};

void lma_init(void) {
	sa_init (&libmad_alloc);
}

void *lma_malloc(size_t size) {
	return sa_alloc (&libmad_alloc, size);
}

void lma_free(void *ptr) {
	sa_free (&libmad_alloc, ptr);
}

void *lma_calloc(size_t nmemb, size_t size) {
	return (sa_calloc (&libmad_alloc, nmemb, size));
}

