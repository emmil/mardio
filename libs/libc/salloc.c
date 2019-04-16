#include <stdint.h>
#include <stddef.h>

#include "salloc.h"
#include "strings.h"
#include "xprintf.h"

#define	SLOT(i) sas->slots[i]

#ifdef	CONFIG_SALLOC_DEBUG
#define	sa_debug(...)	xprintf(__VA_ARGS__)
#else
#define	sa_debug(...)
#endif

void sa_init(struct salloc_s *sas) {
	uint16_t	i;
	void		*ptr;

	ptr = sas->data;

	sa_debug("sa_init (name = %s, data_size = %d, slots = %d): ",
		 sas->name, sas->data_size, sas->slot_count);

	for (i = 0; i < sas->slot_count; i++) {
		SLOT(i).data = ptr;
		ptr += SLOT(i).size;
	}

	uc_memset(sas->data, 0, sas->data_size);

	sa_debug("OK\n");
}

void *sa_alloc(struct salloc_s *sas, size_t n) {
	uint16_t	i;

	i = 0;

	sa_debug("sa_alloc (name = %s, size = %d bytes): ", sas->name, n);

	if (sas->slot_count == sas->used_slots) {
		sa_debug("all slots are used.");
		goto error;
	}

	while (i < sas->slot_count) {
		if (SLOT(i).state == SS_EMPTY && SLOT(i).size >= n) {
			SLOT(i).state = SS_USED;
			SLOT(i).allocated = n;
			goto found;
		}
		i++;
	}

	sa_debug("not suitable slot found.");

error:
	sa_debug(" Failed\n");
	return NULL;

found:
	sa_debug("OK\n");
	sas->used_slots++;

	return SLOT(i).data;
}

void sa_free(struct salloc_s *sas, void *ptr) {
	int	i;

	sa_debug("sa_free (name = %s, ptr = 0x%x)\n", sas->name, ptr);

	if (ptr == NULL)
		return;

	i = 0;

	while (i < sas->slot_count) {
		if (SLOT(i).data == ptr) {
			SLOT(i).state = SS_EMPTY;
			SLOT(i).allocated = 0;
			sas->used_slots--;
			goto out;
		}
		i++;
	}

out:
	return;
}

void *sa_calloc(struct salloc_s *sas, size_t nmemb, size_t size) {
	uint32_t	total_size;
	void		*ptr;

	total_size = nmemb * size;

	if (total_size == 0)
		goto error;

	ptr = sa_alloc(sas, total_size);

	if (ptr == NULL)
		goto error;

	uc_memset(ptr, 0, total_size);

	return (ptr);

error:
	return NULL;
}

void *sa_realloc(struct salloc_s *sas, void *ptr, size_t size) {
	void	*nptr;

	if (ptr == NULL)
		goto error;

	if (ptr != NULL && size == 0) {
		sa_free(sas, ptr);
		goto error;
	}

	nptr = sa_alloc(sas, size);
	if (nptr == NULL)
		goto error;

	/* Not checking size of old alloc! */
	uc_memcpy(nptr, ptr, size);

error:
	return NULL;
}
