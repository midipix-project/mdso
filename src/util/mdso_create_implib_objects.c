/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <mdso/mdso.h>
#include "mdso_hexfmt_impl.h"
#include "mdso_errinfo_impl.h"

static void mdso_init_symentry(char * ch, const char * str)
{
	uint64_t crc64;
	unsigned slen = 11;

	ch[0] = '.';
	ch++;

	if (strlen(str) > (PATH_MAX - slen - 1)) {
		crc64 = mdso_crc64_mbstr((unsigned char *)str,0);
		mdso_write_hex_64(ch,crc64);
		ch = &ch[16];
	} else {
		for (; *str; )
			*ch++ = *str++;
	}

	memcpy(ch,"_symentry.o",slen);
	ch[slen] = '\0';
}

static void mdso_init_dsometa(char * ch, const char * str)
{
	uint64_t crc64;
	unsigned slen = 9;

	memcpy(ch,".dsometa_",slen);
	ch = &ch[slen];

	if (strlen(str) > (PATH_MAX - slen - 1)) {
		crc64 = mdso_crc64_mbstr((unsigned char *)str,0);
		mdso_write_hex_64(ch,crc64);
		ch = &ch[16];
	} else {
		for (; *str; )
			*ch++ = *str++;
	}

	ch[0] = '.';
	ch[1] = 'o';
	ch[2] = '\0';
}

static void mdso_init_object(struct mdso_object * obj, const char * objname)
{
	memset(obj,0,sizeof(*obj));
	obj->name = objname;
}

int  mdso_create_implib_objects(const struct mdso_driver_ctx * dctx)
{
	const char **		unit;
	struct mdso_unit_ctx *	uctx;
	struct mdso_object	obj;
	const char * const *	sym;
	char			objname[PATH_MAX];

	/* symentry */
	for (unit=dctx->units; *unit; unit++) {
		if (mdso_get_unit_ctx(dctx,*unit,&uctx))
			return MDSO_NESTED_ERROR(dctx);

		for (sym=uctx->syms; *sym; sym++) {
			mdso_init_symentry(objname,*sym);
			mdso_init_object(&obj,objname);

			if (mdso_objgen_symentry(dctx,*sym,&obj) < 0)
				return MDSO_NESTED_ERROR(dctx);
		}

		mdso_free_unit_ctx(uctx);
	}

	/* dsometa */
	mdso_init_dsometa(objname,dctx->cctx->libname);
	mdso_init_object(&obj,objname);

	if (mdso_objgen_dsometa(dctx,&obj) < 0)
		return MDSO_NESTED_ERROR(dctx);

	return 0;
}
