/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>

char buf[256] = {0};

static unsigned char mdso_hexfmt[16] = {
	'0','1','2','3','4','5','6','7',
	'8','9','a','b','c','d','e','f',
};

void mdso_write_hex_64(char * ch, uint64_t val)
{
	ch[0xf] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0xe] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0xd] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0xc] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0xb] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0xa] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x9] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x8] = mdso_hexfmt[val % 16]; val >>= 4;

	ch[0x7] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x6] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x5] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x4] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x3] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x2] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x1] = mdso_hexfmt[val % 16]; val >>= 4;
	ch[0x0] = mdso_hexfmt[val % 16]; val >>= 4;
}
