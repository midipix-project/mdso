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

static int mdso_create_output(
	const struct mdso_driver_ctx *	dctx,
	const char *			name)
{
	int	fddst;
	int	fdout;

	fddst = mdso_driver_fddst(dctx);

	if ((fdout = openat(fddst,name,
                        O_CREAT|O_TRUNC|O_WRONLY|O_NOCTTY|O_NOFOLLOW,
                        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
		return MDSO_SYSTEM_ERROR(dctx);

	return fdout;
}

static FILE * mdso_create_output_stream(
	const struct mdso_driver_ctx *	dctx,
	const char *			name)
{
	int	fdout;
	FILE *	fout;

	if ((fdout = mdso_create_output(dctx,name)) < 0)
		return 0;

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
	return mdso_create_output_stream(dctx,arname);
}

int mdso_create_asmsrc(
	const struct mdso_driver_ctx *	dctx,
	const char *			asmname)
{
	return dctx->cctx->dstdir
		? mdso_create_output(dctx,asmname)
		: mdso_driver_fdout(dctx);
}

FILE * mdso_create_object(
	const struct mdso_driver_ctx *	dctx,
	const char *			objname)
{
	return mdso_create_output_stream(dctx,objname);
}
