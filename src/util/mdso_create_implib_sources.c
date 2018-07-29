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


	/* symentry, symfn */
	for (unit=dctx->units; *unit; unit++) {
		if (mdso_get_unit_ctx(dctx,*unit,&uctx))
			return MDSO_NESTED_ERROR(dctx);

		for (sym=uctx->syms; *sym; sym++) {
			mdso_init_asmname(asmname,".%s_symentry.s",*sym);

			if (!(fout = mdso_create_asmsrc(dctx,asmname)))
				return MDSO_NESTED_ERROR(dctx);

			ret = mdso_asmgen_symentry(dctx,*sym,fout);

			if (fout != stdout)
				fclose(fout);

			if (ret < 0)
				return MDSO_NESTED_ERROR(dctx);

			if (uctx->stype[sym-uctx->syms] == MDSO_SYMBOL_TYPE_CODE) {
				mdso_init_asmname(asmname,".%s_symfn.s",*sym);

				if (!(fout = mdso_create_asmsrc(dctx,asmname)))
					return MDSO_NESTED_ERROR(dctx);

				ret = mdso_asmgen_symfn(dctx,*sym,fout);

				if (fout != stdout)
					fclose(fout);

				if (ret < 0)
					return MDSO_NESTED_ERROR(dctx);
			}
		}

		mdso_free_unit_ctx(uctx);
	}

	/* dsometa */
	mdso_init_asmname(asmname,".dsometa_%s.s",dctx->cctx->libname);

	if (!(fout = mdso_create_asmsrc(dctx,asmname)))
		return MDSO_NESTED_ERROR(dctx);

	ret = mdso_asmgen_dsometa(dctx,fout);

	if (fout != stdout)
		fclose(fout);

	return (ret < 0) ? MDSO_NESTED_ERROR(dctx) : 0;
}
