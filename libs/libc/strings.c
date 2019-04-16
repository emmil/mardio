#include "strings.h"

void *uc_memset(void *s, int c, size_t n) {
	register unsigned char	*p = (unsigned char *) s;

	while (n) {
		*p++ = (unsigned char) c;
		--n;
	}

	return s;
}

void *uc_memcpy(void *s1, const void *s2, size_t n) {
	register char		*r1 = s1;
	register const char	*r2 = s2;

	while (n) {
		*r1++ = *r2++;
		--n;
	}

	return s1;
}

int uc_memcmp(const void *s1, const void *s2, size_t n) {
	register const unsigned char	*r1 = (const unsigned char *) s1;
	register const unsigned char	*r2 = (const unsigned char *) s2;
	int r = 0;

	while (n-- && ((r = ((int) (*r1++)) - *r2++) == 0));

	return r;
}

void *uc_memmove(void *s1, const void *s2, size_t n) {
	register char		*s = (char *) s1;
	register const char	*p = (const char *) s2;

	if (p >= s) {
		while (n) {
			*s++ = *p++;
			--n;
		}
	} else {
		while (n) {
			--n;
			s[n] = p[n];
		}
	}

	return s1;
}

void *uc_memchr(const void *s, int c, size_t n) {
	register const unsigned char	*r = (const unsigned char *) s;

	while (n) {
		if (*r == ((unsigned char) c)) {
			return (void *) r;	/* silence the warning */
		}
		++r;
		--n;
	}

	return NULL;
}

char *uc_strstr(const char *s1, const char *s2) {
	register const char	*s = s1;
	register const char	*p = s2;

	do {
		if (!*p) {
			return (char *) s1;;
		}
		if (*p == *s) {
			++p;
			++s;
		} else {
			p = s2;
			if (!*s) {
				return NULL;
			}
			s = ++s1;
		}
	} while (1);
}


size_t uc_strlen(const char *s) {
	register const char	*p;

	for (p = s; *p; p++);

	return p - s;
}

int uc_strcmp(register const char *s1, register const char *s2) {
	int	r;

	while (((r = ((int)(*((unsigned char *)s1))) - *((unsigned char *)s2++))
			== 0) && *s1++);

	return r;
}

int uc_strncmp(register const char *s1, register const char *s2, size_t n) {
	int	r = 0;

	while (n--
		&& ((r = ((int)(*((unsigned char *)s1))) - *((unsigned char *)s2++))
		== 0)
	       && *s1++);

	return r;
}

char *uc_strcpy(char *s1, const char *s2) {
	register char	*s = s1;

	while ((*s++ = *s2++) != 0);

	return s1;
}
