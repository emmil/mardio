#ifndef ID3TAG_H
#define	ID3TAG_H

#include <stdint.h>


struct id3 {
	uint8_t		id[3];
	uint8_t		major;
	uint8_t		minor;
	uint8_t		flags;
	uint32_t	size;
};

#define	ID3_UNSYNC		(1 << 7)
#define	ID3_EXTENDED		(1 << 6)
#define	ID3_EXPERIMENTAL	(1 << 5)

signed long id3_tag_query(uint8_t const *data, uint32_t length);

#endif
