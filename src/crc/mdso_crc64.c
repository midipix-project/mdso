/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2021  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <unistd.h>

#include <mdso/mdso.h>
#include <mdso/mdso_crc64.h>

static const uint64_t crc64_table[256] = MDSO_CRC64_TABLE;

uint64_t mdso_crc64_mbstr(const unsigned char * str, size_t * symlen)
{
	const unsigned char *	ch;
	uint64_t		crc64;

	crc64	= 0 ^ 0xFFFFFFFFFFFFFFFF;
	ch	= str;

	while (*ch) {
		crc64 = (crc64 >> 8) ^ crc64_table[(crc64 ^ *ch) & 0xFF];
		ch++;
	}

	if (symlen)
		*symlen = ch - str;

	return (crc64 ^ 0xFFFFFFFFFFFFFFFF);
}
