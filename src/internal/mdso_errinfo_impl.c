/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <mdso/mdso.h>
#include "mdso_driver_impl.h"
#include "mdso_errinfo_impl.h"

int mdso_record_error(
	const struct mdso_driver_ctx *	dctx,
	int				syserror,
	int				liberror,
	const char *			function,
	int				line,
	unsigned			flags,
	void *				ctx)
{
	struct mdso_driver_ctx_impl *	ictx;
	struct mdso_error_info *	erri;

	ictx = mdso_get_driver_ictx(dctx);

	if (ictx->errinfp == ictx->erricap)
		return -1;

	*ictx->errinfp = &ictx->erribuf[ictx->errinfp - ictx->erriptr];
	erri = *ictx->errinfp;

	erri->syserror = syserror;
	erri->liberror = liberror;
	erri->function = function;
	erri->line     = line;
	erri->flags    = flags;
	erri->ctx      = ctx;

	ictx->errinfp++;

	return -1;
}
