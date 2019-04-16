#ifndef	UC_STRINGS_H
#define	UC_STRINGS_H

#include <stddef.h>

void *uc_memset(void *s, int c, size_t n);
void *uc_memcpy(void *s1, const void *s2, size_t n);
int uc_memcmp(const void *s1, const void *s2, size_t n);
void *uc_memmove(void *s1, const void *s2, size_t n);
void *uc_memchr(const void *s, int c, size_t n);
char *uc_strstr(const char *s1, const char *s2);
size_t uc_strlen(const char *s);
int uc_strcmp(register const char *s1, register const char *s2);
int uc_strncmp(register const char *s1, register const char *s2, size_t n);
char *uc_strcpy(char *s1, const char *s2);

#endif
