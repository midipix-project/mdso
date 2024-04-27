/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>

static inline void mdso_obj_write_short(unsigned char * ch, uint16_t val)
{
	ch[0] = val;
	ch[1] = val >> 8;
}

static inline void mdso_obj_write_long(unsigned char * ch, uint32_t val)
{
	ch[0] = val;
	ch[1] = val >> 8;
	ch[2] = val >> 16;
	ch[3] = val >> 24;
}

static inline void mdso_obj_write_quad(unsigned char * ch, uint64_t val)
{
	ch[0] = val;
	ch[1] = val >> 8;
	ch[2] = val >> 16;
	ch[3] = val >> 24;
	ch[4] = val >> 32;
	ch[5] = val >> 40;
	ch[6] = val >> 48;
	ch[7] = val >> 56;
}
