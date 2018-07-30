/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <mdso/mdso.h>
#include <mdso/mdso_output.h>
#include "mdso_driver_impl.h"
#include "mdso_dprintf_impl.h"
#include "mdso_errinfo_impl.h"

static int pretty_header(int fdout, const struct mdso_common_ctx * cctx)
{
	return (cctx->fmtflags & MDSO_PRETTY_YAML)
		? mdso_dprintf(fdout,"exports:\n")
		: 0;
}

static int pretty_export_item(
	int				fdout,
	const struct mdso_common_ctx *	cctx,
	const char *			name)
{
	if (cctx->fmtflags & MDSO_PRETTY_YAML)
		return mdso_dprintf(fdout,"- %s\n",name);
	else
		return mdso_dprintf(fdout,"%s\n",name);
}

int mdso_output_export_symbols(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_unit_ctx *	uctx)
{
	int			fdout;
	const char * const *	sym;

	fdout = mdso_driver_fdout(dctx);

	if (!uctx->syms[0])
		return 0;

	if ((pretty_header(fdout,dctx->cctx)) < 0)
		return MDSO_SYSTEM_ERROR(dctx);

	for (sym=uctx->syms; *sym; sym++)
		if ((pretty_export_item(fdout,dctx->cctx,*sym)) < 0)
			return MDSO_SYSTEM_ERROR(dctx);

	return 0;
}
