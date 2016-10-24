/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <mdso/mdso.h>
#include "mdso_errinfo_impl.h"

static void mdso_init_asmname(char * buf, const char * fmt, const char * str)
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

mdso_api int  mdso_create_implib_sources(const struct mdso_driver_ctx * dctx)
{
	struct mdso_unit_ctx *	uctx;
	const char **		unit;
	FILE *			fout;
	char			asmname[PATH_MAX];
	const char * const *	sym;
	int			ret;

	mdso_init_asmname(asmname,"__%s_dso_meta.s",dctx->cctx->libname);

	if (!(fout = mdso_create_output(dctx,asmname)))
		return MDSO_NESTED_ERROR(dctx);

	ret = mdso_generate_dsometa(dctx,fout);

	if (fout != stdout)
		fclose(fout);

	if (ret < 0)
		return MDSO_NESTED_ERROR(dctx);

	for (unit=dctx->units; *unit; unit++) {
		if (mdso_get_unit_ctx(dctx,*unit,&uctx))
			return MDSO_NESTED_ERROR(dctx);

		for (sym=uctx->syms; *sym; sym++) {
			mdso_init_asmname(asmname,"__%s_sym_entry.s",*sym);

			if (!(fout = mdso_create_output(dctx,asmname)))
				return MDSO_NESTED_ERROR(dctx);

			ret = mdso_generate_symentry(dctx,*sym,fout);

			if (fout != stdout)
				fclose(fout);

			if (ret < 0)
				return MDSO_NESTED_ERROR(dctx);

			mdso_init_asmname(asmname,"__%s_sym_fn.s",*sym);

			if (!(fout = mdso_create_output(dctx,asmname)))
				return MDSO_NESTED_ERROR(dctx);

			ret = mdso_generate_symfn(*sym,fout);

			if (fout != stdout)
				fclose(fout);

			if (ret < 0)
				return MDSO_NESTED_ERROR(dctx);
		}

		mdso_free_unit_ctx(uctx);
	}

	return 0;
}
