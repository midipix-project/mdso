/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015  Z. Gilboa                               */
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

FILE * mdso_create_output(
	const struct mdso_driver_ctx *	dctx,
	const char *			asmname)
{
	struct mdso_driver_ctx_impl *	ictx;
	uintptr_t			addr;
	int				fdout;

	if (!dctx->cctx->dstdir)
		return stdout;

	addr = (uintptr_t)dctx - offsetof(struct mdso_driver_ctx_impl,ctx);
	ictx = (struct mdso_driver_ctx_impl *)addr;

	if ((fdout = openat(ictx->fddst,asmname,
                        O_CREAT|O_TRUNC|O_WRONLY|O_NOCTTY|O_NOFOLLOW,
                        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
		return 0;

	return fdopen(fdout,"w");
}
