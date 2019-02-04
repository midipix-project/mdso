#ifndef MDSO_DRIVER_IMPL_H
#define MDSO_DRIVER_IMPL_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <mdso/mdso.h>
#include "mdso_dprintf_impl.h"
#include "argv/argv.h"

#ifdef  __PE__
#define MDSO_DRIVER_PE_HOST	(1)
#else
#define MDSO_DRIVER_PE_HOST	(0)
#endif

#ifdef  __WINNT__
#define MDSO_DRIVER_WINNT_HOST	(1)
#else
#define MDSO_DRIVER_WINNT_HOST	(0)
#endif

#define MDSO_OPTV_ELEMENTS 64

extern const struct argv_option mdso_default_options[];

enum app_tags {
	TAG_HELP,
	TAG_VERSION,
	TAG_ASM,
	TAG_OBJ,
	TAG_QUAD_PTR,
	TAG_IMPLIB,
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
	struct mdso_fd_ctx		fdctx;
	char *                          asmbase;
	char *				implib;
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

static inline int mdso_driver_fdin(const struct mdso_driver_ctx * dctx)
{
	struct mdso_fd_ctx fdctx;
	mdso_get_driver_fdctx(dctx,&fdctx);
	return fdctx.fdin;
}

static inline int mdso_driver_fdout(const struct mdso_driver_ctx * dctx)
{
	struct mdso_fd_ctx fdctx;
	mdso_get_driver_fdctx(dctx,&fdctx);
	return fdctx.fdout;
}

static inline int mdso_driver_fderr(const struct mdso_driver_ctx * dctx)
{
	struct mdso_fd_ctx fdctx;
	mdso_get_driver_fdctx(dctx,&fdctx);
	return fdctx.fderr;
}

static inline int mdso_driver_fdlog(const struct mdso_driver_ctx * dctx)
{
	struct mdso_fd_ctx fdctx;
	mdso_get_driver_fdctx(dctx,&fdctx);
	return fdctx.fdlog;
}

static inline int mdso_driver_fdcwd(const struct mdso_driver_ctx * dctx)
{
	struct mdso_fd_ctx fdctx;
	mdso_get_driver_fdctx(dctx,&fdctx);
	return fdctx.fdcwd;
}

static inline int mdso_driver_fddst(const struct mdso_driver_ctx * dctx)
{
	struct mdso_fd_ctx fdctx;
	mdso_get_driver_fdctx(dctx,&fdctx);
	return fdctx.fddst;
}

#endif
