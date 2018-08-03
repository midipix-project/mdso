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
#include <sys/mman.h>

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

static int mdso_map_output(
	const struct mdso_driver_ctx *	dctx,
	struct mdso_object *		obj,
	int				fd)
{
	void * addr;

	if (ftruncate(fd,obj->size)) {
		close(fd);
		return MDSO_SYSTEM_ERROR(dctx);
	}

	addr = mmap(
		0,obj->size,
		PROT_WRITE,MAP_SHARED,
		fd,0);

	close(fd);

	if (addr == MAP_FAILED)
		return MDSO_SYSTEM_ERROR(dctx);

	obj->addr = addr;

	return 0;
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

static int mdso_create_mapped_output(
	const struct mdso_driver_ctx *	dctx,
	struct mdso_object *		obj)
{
	int fd;

	if ((fd = mdso_create_output(dctx,obj->name)) < 0)
		return MDSO_NESTED_ERROR(dctx);

	if (mdso_map_output(dctx,obj,fd) < 0)
		return MDSO_NESTED_ERROR(dctx);

	return 0;
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

int mdso_create_object(
	const struct mdso_driver_ctx *	dctx,
	struct mdso_object *		obj)
{
	return mdso_create_mapped_output(dctx,obj);
}
