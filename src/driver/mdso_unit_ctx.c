/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015  Z. Gilboa                               */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <mdso/mdso.h>
#include "mdso_driver_impl.h"

static int mdso_free_unit_ctx_impl(struct mdso_unit_ctx_impl * ctx, int status)
{
	if (ctx) {
		mdso_unmap_input(&ctx->map);
		free(ctx);
	}

	return status;
}

int mdso_get_unit_ctx(
	const struct mdso_driver_ctx *	dctx,
	const char *			path,
	struct mdso_unit_ctx **		pctx)
{
	struct mdso_unit_ctx_impl *	ctx;

	if (!dctx || !(ctx = calloc(sizeof(*ctx),1)))
		return -1;

	if (mdso_map_input(-1,path,PROT_READ,&ctx->map))
		return mdso_free_unit_ctx_impl(ctx,-1);

	memcpy(&ctx->cctx,dctx->cctx,
		sizeof(ctx->cctx));

	ctx->path	= path;

	ctx->uctx.path	= &ctx->path;
	ctx->uctx.map	= &ctx->map;
	ctx->uctx.cctx	= &ctx->cctx;

	*pctx = &ctx->uctx;
	return 0;
}

void mdso_free_unit_ctx(struct mdso_unit_ctx * ctx)
{
	struct mdso_unit_ctx_impl *	ictx;
	uintptr_t			addr;

	if (ctx) {
		addr = (uintptr_t)ctx - offsetof(struct mdso_unit_ctx_impl,uctx);
		ictx = (struct mdso_unit_ctx_impl *)addr;
		mdso_free_unit_ctx_impl(ictx,0);
	}
}
