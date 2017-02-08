#ifndef MDSO_DRIVER_IMPL_H
#define MDSO_DRIVER_IMPL_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <mdso/mdso.h>
#include "argv/argv.h"

#define MDSO_OPTV_ELEMENTS 64

extern const struct argv_option mdso_default_options[];

enum app_tags {
	TAG_HELP,
	TAG_VERSION,
	TAG_ASM,
	TAG_OBJ,
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
	struct mdso_common_ctx          cctx;
	struct mdso_driver_ctx          ctx;
	char *                          asmbase;
	int                             fddst;
	int                             fdtmpin;
	const struct mdso_unit_ctx *    euctx;
	const char *                    eunit;
	struct mdso_error_info**        errinfp;
	struct mdso_error_info**        erricap;
	struct mdso_error_info *        erriptr[64];
	struct mdso_error_info          erribuf[64];
};

struct mdso_unit_ctx_impl {
	const char *		path;
	struct mdso_input	map;
	struct mdso_expsyms *	expsyms;
	struct mdso_common_ctx	cctx;
	struct mdso_unit_ctx	uctx;
};

static inline struct mdso_driver_ctx_impl * mdso_get_driver_ictx(
	const struct mdso_driver_ctx * dctx)
{
        uintptr_t addr;

        if (dctx) {
                addr = (uintptr_t)dctx - offsetof(struct mdso_driver_ctx_impl,ctx);
                return (struct mdso_driver_ctx_impl *)addr;
        }

        return 0;
}

static inline void mdso_driver_set_ectx(
	const struct mdso_driver_ctx * dctx,
	const struct mdso_unit_ctx *   uctx,
	const char *                   unit)
{
	struct mdso_driver_ctx_impl *  ictx;

	ictx        = mdso_get_driver_ictx(dctx);
	ictx->euctx = uctx;
	ictx->eunit = unit;
}

#endif
