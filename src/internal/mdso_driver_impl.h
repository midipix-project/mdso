#ifndef MDSO_DRIVER_IMPL_H
#define MDSO_DRIVER_IMPL_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <mdso/mdso.h>

enum app_tags {
	TAG_HELP,
	TAG_VERSION,
	TAG_QUAD_PTR,
	TAG_LIBPATH,
	TAG_LIBNAME,
	TAG_DSTDIR,
	TAG_PRETTY,
	TAG_EXPSYMS,
};

struct mdso_expsyms {
	char *		buffer;
	const char *	syms[];
};

struct mdso_driver_ctx_impl {
	struct mdso_common_ctx	cctx;
	struct mdso_driver_ctx	ctx;
	char *			asmbase;
	int			fddst;
	int			fdtmpin;
};

struct mdso_unit_ctx_impl {
	const char *		path;
	struct mdso_input	map;
	struct mdso_expsyms *	expsyms;
	struct mdso_common_ctx	cctx;
	struct mdso_unit_ctx	uctx;
};

#endif
