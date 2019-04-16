#include "id3tag.h"

enum tagtype {
	TAG_NONE = 0,
	TAG_ID3V1,
	TAG_ID3V2,
	TAG_ID3V2_FOOTER
};

enum {
	ID3_TAG_FLAG_UNSYNCHRONISATION     = 0x80,
	ID3_TAG_FLAG_EXTENDEDHEADER        = 0x40,
	ID3_TAG_FLAG_EXPERIMENTALINDICATOR = 0x20,
	ID3_TAG_FLAG_FOOTERPRESENT         = 0x10,

	ID3_TAG_FLAG_KNOWNFLAGS            = 0xf0
};

uint32_t parse_uint(uint8_t const **ptr, uint16_t bytes) {
	uint32_t	value = 0;

	switch (bytes) {
	case 4:
		value = (value << 8) | *(*ptr)++;
	case 3:
		value = (value << 8) | *(*ptr)++;
	case 2:
		value = (value << 8) | *(*ptr)++;
	case 1:
		value = (value << 8) | *(*ptr)++;
	}

	return value;
}

uint32_t parse_syncsafe(uint8_t const **ptr, uint16_t bytes) {
	uint32_t	value = 0;

	switch (bytes) {
	case 5:
		value = (value << 4) | (*(*ptr)++ & 0x0F);
	case 4:
		value = (value << 7) | (*(*ptr)++ & 0x7F);
		value = (value << 7) | (*(*ptr)++ & 0x7F);
		value = (value << 7) | (*(*ptr)++ & 0x7F);
		value = (value << 7) | (*(*ptr)++ & 0x7F);
	}

	return value;
}

enum tagtype tagtype(uint8_t const *data, uint32_t length) {

	if (length > 3 && data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
		return TAG_ID3V1;

	if (length > 10 &&
	    ((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
	     (data[0] == '3' && data[1] == 'D' && data[2] == 'I')) &&
	    data[3] < 0xFF && data[4] < 0xFF &&
	    data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80
	    && data[9] < 0x80)
		return data[0] == 'I' ? TAG_ID3V2 : TAG_ID3V2_FOOTER;

	return TAG_NONE;
}

void parse_header(uint8_t const **ptr,
		  uint16_t * version, int16_t * flags, uint32_t * size) {

	*ptr += 3;

	*version = parse_uint (ptr, 2);
	*flags   = parse_uint (ptr, 1);
	*size    = parse_syncsafe (ptr, 4);
}

signed long id3_tag_query(uint8_t const *data, uint32_t length) {
	uint16_t	version;
	int16_t		flags;
	uint32_t	size;

	switch (tagtype(data, length)) {
	case TAG_ID3V1:
		return 128;

	case TAG_ID3V2:
		parse_header(&data, &version, &flags, &size);
		if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
			size += 10;

		return 10 + size;

	case TAG_ID3V2_FOOTER:
		parse_header(&data, &version, &flags, &size);
		return -size - 10;

	case TAG_NONE:
		break;
	}

	return 0;
}
