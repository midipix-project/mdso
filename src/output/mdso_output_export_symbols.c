/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <mdso/mdso.h>
#include <mdso/mdso_output.h>
#include "mdso_errinfo_impl.h"

static int pretty_header(const struct mdso_common_ctx * cctx, FILE * fout)
{
	return (cctx->fmtflags & MDSO_PRETTY_YAML)
		? fputs("exports:\n",fout)
		: 0;
}

static int pretty_export_item(const struct mdso_common_ctx * cctx, const char * name, FILE * fout)
{
	if (cctx->fmtflags & MDSO_PRETTY_YAML)
		return fprintf(fout,"- %s\n",name);
	else
		return fprintf(fout,"%s\n",name);
}

int mdso_output_export_symbols(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_unit_ctx *	uctx,
	FILE *				fout)
{
	const char * const * sym;

	if (!uctx->syms[0])
		return 0;

	if ((pretty_header(dctx->cctx,fout)) < 0)
		return MDSO_FILE_ERROR(dctx);

	for (sym=uctx->syms; *sym; sym++)
		if ((pretty_export_item(dctx->cctx,*sym,fout)) < 0)
			return MDSO_FILE_ERROR(dctx);

	return 0;
}
