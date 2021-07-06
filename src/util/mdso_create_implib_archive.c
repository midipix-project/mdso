/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2021  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mdso/mdso.h>
#include "mdso_errinfo_impl.h"

static void mdso_free_uctx_vector(
	struct mdso_unit_ctx **	uctxv,
	const char **		symv)
{
	struct mdso_unit_ctx **	puctx;

	if (symv)
		free(symv);

	for (puctx=uctxv; *puctx; puctx++)
		mdso_free_unit_ctx(*puctx);

	free(uctxv);
}

int  mdso_create_implib_archive(const struct mdso_driver_ctx * dctx)
{
	size_t			nsym;
	struct mdso_unit_ctx **	uctxv;
	struct mdso_unit_ctx **	puctx;
	struct mdso_object	obj;
	const char * const *	dsym;
	const char **		unit;
	const char **		psym;
	const char **		symv;

	if (!dctx->cctx->implib)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_NAME);

	for (unit=dctx->units; *unit; unit++)
		(void)0;

	if (!(uctxv = calloc(++unit - dctx->units,sizeof(*uctxv))))
		return MDSO_SYSTEM_ERROR(dctx);

	for (puctx=uctxv,unit=dctx->units; *unit; unit++) {
		if (mdso_get_unit_ctx(dctx,*unit,puctx++)) {
			mdso_free_uctx_vector(uctxv,0);
			return MDSO_NESTED_ERROR(dctx);
		}
	}

	for (nsym=0,puctx=uctxv; *puctx; puctx++)
		for (dsym=puctx[0]->syms; *dsym; dsym++)
			nsym++;

	if (!(symv = calloc(nsym+1,sizeof(const char *)))) {
		mdso_free_uctx_vector(uctxv,0);
		return MDSO_SYSTEM_ERROR(dctx);
	}

	for (psym=symv,puctx=uctxv; *puctx; puctx++) {
		for (dsym=puctx[0]->syms; *dsym; dsym++)
			*psym++ = *dsym;
	}

	memset(&obj,0,sizeof(obj));
	obj.name = dctx->cctx->implib;

	if (mdso_argen_common(dctx,symv,&obj) < 0) {
		mdso_free_uctx_vector(uctxv,symv);
		return MDSO_NESTED_ERROR(dctx);
	}

	return 0;
}
