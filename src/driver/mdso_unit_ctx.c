/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
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
#include "mdso_errinfo_impl.h"

static int mdso_free_unit_ctx_impl(struct mdso_unit_ctx_impl * ctx, int status)
{
	if (ctx) {
		if (ctx->expsyms && ctx->expsyms->buffer)
			free(ctx->expsyms->buffer);

		if (ctx->expsyms)
			free(ctx->expsyms);

		mdso_unmap_input(&ctx->map);
		free(ctx);
	}

	return status;
}

static int mdso_stdin_to_tmp(const struct mdso_driver_ctx * dctx)
{
	struct mdso_driver_ctx_impl *	ictx;
	uintptr_t			addr;
	int				fdtmp;

	ssize_t ret;
	ssize_t cnt;
	char *	ch;
	char	buf[4096];
	char	tmpname[] = "/tmp/mdso_stdin_to_tmp_XXXXXX";

	addr = (uintptr_t)dctx - offsetof(struct mdso_driver_ctx_impl,ctx);
	ictx = (struct mdso_driver_ctx_impl *)addr;

	if (ictx->fdtmpin >= 0)
		return dup(ictx->fdtmpin);

	if ((fdtmp = mkstemp(tmpname)) < 0)
		return -1;

	if ((ictx->fdtmpin = dup(fdtmp)) < 0) {
		close(fdtmp);
		unlink(tmpname);
		return -1;
	}

	strcpy(ictx->tmpname,tmpname);

	for (;;) {
		ret = read(0,buf,sizeof(buf)-1);

		while ((ret < 0) && (errno == EINTR))
			ret = read(0,buf,sizeof(buf)-1);

		if (ret < 0) {
			close(fdtmp);
			return -1;

		} else if (ret == 0) {
			return fdtmp;

		} else {
			ch  = buf;
			cnt = ret;
			ret = 0;

			for (; cnt; ) {
				ret = write(fdtmp,ch,cnt);

				while ((ret < 0) && (errno == EINTR))
					ret = write(fdtmp,ch,cnt);

				if (ret < 0) {
					close(fdtmp);
					return -1;
				}

				ch  += ret;
				cnt -= ret;
			}
		}
	}
}

static int mdso_create_symbol_vector(struct mdso_unit_ctx_impl * ctx)
{
	int		nsyms;
	size_t		nbytes;
	size_t		size;
	char *		dst;
	const char *	ch;
	const char **	sym;

	const char	exphdr[] = "EXPORTS\n";
	const char	imphdr[] = "IMPORTS\n";

	ch	= ctx->map.addr;
	nbytes	= ctx->map.size;

	for (nsyms=0; nbytes; ch++,nbytes--)
		nsyms += (*ch == '\n');

	size = offsetof(struct mdso_unit_ctx_impl,expsyms);
	size += (++nsyms)*sizeof(const char *);

	if (!(ctx->expsyms = calloc(1,size)))
		return -1;

	if (!(ctx->expsyms->buffer = calloc(1,ctx->map.size)))
		return -1;

	ch	= ctx->map.addr;
	nbytes	= ctx->map.size;
	sym	= ctx->expsyms->syms;
	dst	= ctx->expsyms->buffer;
	size	= strlen(exphdr);

	/* support .def input files */
	if ((nbytes >= size) && !(strncmp(ch,exphdr,size))) {
		ch	+= size;
		nbytes	-= size;
	}

	/* advance to first symbol */
	for (; nbytes && ((*ch==' ')
			|| (*ch=='\t')
			|| (*ch=='\r')
			|| (*ch=='\n')); nbytes--)
		ch++;

	/* support .def input files */
	size = strlen(imphdr);

	while (nbytes && ((nbytes < size) || (strncmp(ch,imphdr,size)))) {
		/* vector */
		*sym++ = dst;

		/* symbol */
		for (; nbytes && ((*ch!=' ')
				&& (*ch!='\t')
				&& (*ch!='\r')
				&& (*ch!='\n')); nbytes--)
			*dst++ = *ch++;

		dst++;

		/* discard rest of input line */
		for (; nbytes && (*ch!='\n'); nbytes--)
			ch++;

		/* advance to next symbol */
		for (; nbytes && ((*ch==' ')
				|| (*ch=='\t')
				|| (*ch=='\r')
				|| (*ch=='\n')); nbytes--)
			ch++;
	}

	return 0;
}

int mdso_get_unit_ctx(
	const struct mdso_driver_ctx *	dctx,
	const char *			path,
	struct mdso_unit_ctx **		pctx)
{
	struct mdso_unit_ctx_impl *	ctx;
	int				fd;

	if (!dctx)
		return MDSO_CUSTOM_ERROR(dctx,0);

	else if (!(ctx = calloc(1,sizeof(*ctx))))
		return MDSO_BUFFER_ERROR(dctx);

	mdso_driver_set_ectx(
		dctx,0,path);

	if (strcmp(path,"-"))
		fd = -1;

	else if ((fd = mdso_stdin_to_tmp(dctx)) < 0)
		return mdso_free_unit_ctx_impl(
			ctx,MDSO_FILE_ERROR(dctx));

	if (mdso_map_input(dctx,fd,path,PROT_READ,&ctx->map))
		return mdso_free_unit_ctx_impl(
			ctx,MDSO_NESTED_ERROR(dctx));

	if (fd >= 0)
		close(fd);

	if (mdso_create_symbol_vector(ctx))
		return mdso_free_unit_ctx_impl(
			ctx,MDSO_BUFFER_ERROR(dctx));

	memcpy(&ctx->cctx,dctx->cctx,
		sizeof(ctx->cctx));

	ctx->path	= path;

	ctx->uctx.path	= &ctx->path;
	ctx->uctx.map	= &ctx->map;
	ctx->uctx.cctx	= &ctx->cctx;
	ctx->uctx.syms	= ctx->expsyms->syms;

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
