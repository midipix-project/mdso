/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

#include <mdso/mdso.h>
#include "mdso_driver_impl.h"
#include "mdso_errinfo_impl.h"

static FILE * mdso_create_output(
	const struct mdso_driver_ctx *	dctx,
	const char *			name,
	int				fdat)
{
	struct mdso_driver_ctx_impl *	ictx;
	uintptr_t			addr;
	int				fdout;
	FILE *				fout;

	addr = (uintptr_t)dctx - offsetof(struct mdso_driver_ctx_impl,ctx);
	ictx = (struct mdso_driver_ctx_impl *)addr;
	fdat = (fdat == AT_FDCWD) ? AT_FDCWD : ictx->fddst;

	if ((fdout = openat(fdat,name,
                        O_CREAT|O_TRUNC|O_WRONLY|O_NOCTTY|O_NOFOLLOW,
                        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
		MDSO_SYSTEM_ERROR(dctx);
		return 0;
	}

	if (!(fout = fdopen(fdout,"w"))) {
		close(fdout);
		MDSO_SYSTEM_ERROR(dctx);
		return 0;
	}

	return fout;
}

FILE * mdso_create_archive(
	const struct mdso_driver_ctx *	dctx,
	const char *			arname)
{
	return mdso_create_output(dctx,arname,AT_FDCWD);
}

FILE * mdso_create_asmsrc(
	const struct mdso_driver_ctx *	dctx,
	const char *			asmname)
{
	if (!dctx->cctx->dstdir)
		return stdout;

	return mdso_create_output(dctx,asmname,-1);
}

FILE * mdso_create_object(
	const struct mdso_driver_ctx *	dctx,
	const char *			objname)
{
	if (!dctx->cctx->dstdir) {
		MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_DSTDIR);
		return 0;
	}

	return mdso_create_output(dctx,objname,-1);
}
