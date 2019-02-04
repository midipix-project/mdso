/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <mdso/mdso.h>
#include "mdso_errinfo_impl.h"

static void mdso_init_objname(char * buf, const char * fmt, const char * str)
{
	char			hexstr[24];
	long long unsigned int	crc64;

	if (strlen(str) + strlen(fmt) > (PATH_MAX - 1)) {
		crc64 = mdso_crc64_mbstr((const unsigned char *)str,0);
		sprintf(hexstr,"%llx",crc64);
		sprintf(buf,fmt,hexstr);
	} else
		sprintf(buf,fmt,str);
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
			mdso_init_objname(objname,".%s_symentry.o",*sym);
			mdso_init_object(&obj,objname);

			if (mdso_objgen_symentry(dctx,*sym,&obj) < 0)
				return MDSO_NESTED_ERROR(dctx);
		}

		mdso_free_unit_ctx(uctx);
	}

	/* dsometa */
	mdso_init_objname(objname,".dsometa_%s.o",dctx->cctx->libname);
	mdso_init_object(&obj,objname);

	if (mdso_objgen_dsometa(dctx,&obj) < 0)
		return MDSO_NESTED_ERROR(dctx);

	return 0;
}
