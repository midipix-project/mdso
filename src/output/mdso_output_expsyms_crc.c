/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include <mdso/mdso.h>
#include <mdso/mdso_output.h>
#include "mdso_driver_impl.h"
#include "mdso_dprintf_impl.h"
#include "mdso_errinfo_impl.h"

static int pretty_expsym_crc32(
	int		fdout,
	const char *	name)
{
	return mdso_dprintf(
		fdout,"%08" PRIx32 "  %s\n",
		mdso_crc32_mbstr((const unsigned char *)name,0),
		name);
}

static int pretty_expsym_crc64(
	int		fdout,
	const char *	name)
{
	return mdso_dprintf(
		fdout,"%016" PRIx64 "  %s\n",
		mdso_crc64_mbstr((const unsigned char *)name,0),
		name);
}

int mdso_output_expsyms_crc32(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_unit_ctx *	uctx)
{
	int			fdout;
	const char * const *	sym;

	fdout = mdso_driver_fdout(dctx);

	if (!uctx->syms[0])
		return 0;

	for (sym=uctx->syms; *sym; sym++)
		if ((pretty_expsym_crc32(fdout,*sym)) < 0)
			return MDSO_SYSTEM_ERROR(dctx);

	return 0;
}

int mdso_output_expsyms_crc64(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_unit_ctx *	uctx)
{
	int			fdout;
	const char * const *	sym;

	fdout = mdso_driver_fdout(dctx);

	if (!uctx->syms[0])
		return 0;

	for (sym=uctx->syms; *sym; sym++)
		if ((pretty_expsym_crc64(fdout,*sym)) < 0)
			return MDSO_SYSTEM_ERROR(dctx);

	return 0;
}
