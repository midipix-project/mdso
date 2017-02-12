/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mdso/mdso.h>
#include "mdso_errinfo_impl.h"

static void mdso_free_uctx_vector(struct mdso_unit_ctx ** uctxv, FILE * fout)
{
	struct mdso_unit_ctx **	puctx;

	for (puctx=uctxv; *puctx; puctx++)
		mdso_free_unit_ctx(*puctx);

	free(uctxv);
	fclose(fout);
}

static int mdso_symcmp(const void * src, const void * dst)
{
	return strcmp(*(const char **)src,*(const char **)dst);
}

int  mdso_create_implib_archive(const struct mdso_driver_ctx * dctx)
{
	int			ret;
	FILE *			fout;
	size_t			nsym;
	struct mdso_unit_ctx **	uctxv;
	struct mdso_unit_ctx **	puctx;
	const char * const *	dsym;
	const char **		unit;
	const char **		psym;
	const char **		symv;
	const char *		asym[512];

	if (!dctx->cctx->implib)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_NAME);

	for (unit=dctx->units; *unit; unit++)
		(void)0;

	if (!(uctxv = calloc(++unit - dctx->units,sizeof(*uctxv))))
		return MDSO_SYSTEM_ERROR(dctx);

	if (!(fout = mdso_create_archive(dctx,dctx->cctx->implib)))
		return MDSO_NESTED_ERROR(dctx);

	for (puctx=uctxv,unit=dctx->units; *unit; unit++) {
		if (mdso_get_unit_ctx(dctx,*unit,puctx)) {
			mdso_free_uctx_vector(uctxv,fout);
			return MDSO_NESTED_ERROR(dctx);
		}
	}

	for (nsym=0,puctx=uctxv; *puctx; puctx++)
		for (dsym=puctx[0]->syms; *dsym; dsym++)
			nsym++;

	if (nsym < 512) {
		memset(asym,0,sizeof(asym));
		symv = asym;

	} else if (!(symv = calloc(nsym+1,sizeof(const char *)))) {
		mdso_free_uctx_vector(uctxv,fout);
		return MDSO_SYSTEM_ERROR(dctx);
	}

	for (psym=symv,puctx=uctxv; *puctx; puctx++)
		for (dsym=puctx[0]->syms; *dsym; dsym++)
			*psym++ = *dsym;

	qsort(symv,nsym,sizeof(*symv),mdso_symcmp);
	ret = mdso_argen_common(dctx,symv,fout,0);

	if (symv != asym)
		free(symv);

	mdso_free_uctx_vector(uctxv,fout);

	return ret ? MDSO_NESTED_ERROR(dctx) : 0;
}
