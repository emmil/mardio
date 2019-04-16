#ifndef SALLOC_H
#define	SALLOC_H

#include <stdint.h>
#include <stddef.h>

#define	SA_ENTRY(entry_size) {.state = SS_EMPTY, .size = entry_size, .allocated = 0, .data = NULL}

enum salloc_slot_state_e {
	SS_EMPTY = 0,
	SS_USED
};

struct slot_s {
	enum salloc_slot_state_e	state;
	uint32_t			size;
	uint32_t			allocated;
	void				*data;
};

struct salloc_s {
	const char			*name;
	void				*data;
	int				data_size;
	const uint16_t			slot_count;
	uint16_t			used_slots;
	struct slot_s			slots[];
};


void sa_init(struct salloc_s *sas);
void *sa_alloc(struct salloc_s *sas, size_t n);
void sa_free(struct salloc_s *sas, void *ptr);
void *sa_calloc(struct salloc_s *sas, size_t nmemb, size_t size);
void *sa_realloc(struct salloc_s *sas, void *ptr, size_t size);

#endif
