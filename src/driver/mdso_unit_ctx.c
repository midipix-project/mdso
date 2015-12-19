/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015  Z. Gilboa                               */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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

static FILE * mdso_stdin_to_tmp(void)
{
	FILE *	ftmp;
	char	buf[4096];
	ssize_t	nread;
	int	ret;

	if (!(ftmp = tmpfile()))
		return 0;

	nread = read(0,buf,sizeof(buf)-1);

	while (nread) {
		if (nread > 0) {
			buf[nread] = '\0';
			ret = fputs(buf,ftmp);
		} else
			ret = (errno == EINTR) ? 0 : -1;

		if (ret < 0) {
			fclose(ftmp);
			return 0;
		}

		nread = read(0,buf,sizeof(buf)-1);
	}

	return ftmp;
}

int mdso_get_unit_ctx(
	const struct mdso_driver_ctx *	dctx,
	const char *			path,
	struct mdso_unit_ctx **		pctx)
{
	struct mdso_unit_ctx_impl *	ctx;
	FILE *				ftmp;
	int				fd;

	if (!dctx || !(ctx = calloc(sizeof(*ctx),1)))
		return -1;

	if (strcmp(path,"-"))
		fd = -1;
	else if (!(ftmp = mdso_stdin_to_tmp()))
		return mdso_free_unit_ctx_impl(ctx,-1);
	else if ((fd = dup(fileno(ftmp))) < 0)
		return mdso_free_unit_ctx_impl(ctx,-1);
	else
		fclose(ftmp);

	if (mdso_map_input(fd,path,PROT_READ,&ctx->map))
		return mdso_free_unit_ctx_impl(ctx,-1);

	if (fd > 0)
		close(fd);

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
