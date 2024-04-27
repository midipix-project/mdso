/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <unistd.h>

#include <mdso/mdso.h>
#include <mdso/mdso_crc32.h>

static const uint32_t crc32_table[256] = MDSO_CRC32_TABLE;

uint32_t mdso_crc32_mbstr(const unsigned char * str, size_t * symlen)
{
	const unsigned char *	ch;
	uint32_t		crc32;

	crc32	= 0 ^ 0xFFFFFFFF;
	ch	= str;

	while (*ch) {
		crc32 = (crc32 >> 8) ^ crc32_table[(crc32 ^ *ch) & 0xFF];
		ch++;
	}

	if (symlen)
		*symlen = ch - str;

	return (crc32 ^ 0xFFFFFFFF);
}
