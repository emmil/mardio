#ifndef	_LMALLOC_H_
#define	_LMALLOC_H_

#include <stdint.h>

void lma_init(void);
void *lma_malloc(size_t size);
void lma_free(void *ptr);
void *lma_calloc(size_t nmemb, size_t size);

#endif
