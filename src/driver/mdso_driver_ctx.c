/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015  Z. Gilboa                               */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#define ARGV_DRIVER

#include <mdso/mdso.h>
#include <mdso/mdso_output.h>
#include "mdso_driver_impl.h"
#include "argv/argv.h"

extern const struct argv_option mdso_default_options[];

struct mdso_driver_ctx_alloc {
	struct argv_meta *		meta;
	struct mdso_driver_ctx_impl	ctx;
	uint64_t			guard;
	const char *			units[];
};

static uint32_t mdso_argv_flags(uint32_t flags)
{
	uint32_t ret = 0;

	if (flags & MDSO_DRIVER_VERBOSITY_NONE)
		ret |= ARGV_VERBOSITY_NONE;

	if (flags & MDSO_DRIVER_VERBOSITY_ERRORS)
		ret |= ARGV_VERBOSITY_ERRORS;

	if (flags & MDSO_DRIVER_VERBOSITY_STATUS)
		ret |= ARGV_VERBOSITY_STATUS;

	return ret;
}

static int mdso_driver_usage(
	const char *			program,
	const char *			arg,
	const struct argv_option *	options,
	struct argv_meta *		meta)
{
	char header[512];

	snprintf(header,sizeof(header),
		"Usage: %s [options] <file>...\n" "Options:\n",
		program);

	argv_usage(stdout,header,options,arg);
	argv_free(meta);

	return MDSO_USAGE;
}

static struct mdso_driver_ctx_impl * mdso_driver_ctx_alloc(
	struct argv_meta *		meta,
	const struct mdso_common_ctx *	cctx,
	size_t				nunits)
{
	struct mdso_driver_ctx_alloc *	ictx;
	size_t				size;
	struct argv_entry *		entry;
	const char **			units;

	size =  sizeof(struct mdso_driver_ctx_alloc);
	size += (nunits+1)*sizeof(const char *);

	if (!(ictx = calloc(size,1)))
		return 0;

	if (cctx)
		memcpy(&ictx->ctx.cctx,cctx,sizeof(*cctx));

	for (entry=meta->entries,units=ictx->units; entry->fopt || entry->arg; entry++)
		if (!entry->fopt)
			*units++ = entry->arg;

	ictx->meta = meta;
	ictx->ctx.ctx.units = ictx->units;
	return &ictx->ctx;
}

static int mdso_dstdir_open(const struct mdso_common_ctx * cctx, const char * asmbase)
{
	int fdtop;
	int fddst;
	const int dirmode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

	if ((fdtop = openat(AT_FDCWD,cctx->dstdir,O_DIRECTORY)) < 0) {
		if (mkdirat(AT_FDCWD,cctx->dstdir,dirmode) < 0)
			return -1;
		else if ((fdtop = openat(AT_FDCWD,cctx->dstdir,O_DIRECTORY)) < 0)
			return -1;
	}

	if ((fddst = openat(fdtop,asmbase,O_DIRECTORY)) < 0) {
		if (mkdirat(fdtop,asmbase,dirmode) < 0)
			return -1;
		else if ((fddst = openat(fdtop,asmbase,O_DIRECTORY)) < 0)
			return -1;
	}

	close(fdtop);
	return fddst;
}

static int mdso_get_driver_ctx_fail(
	struct argv_meta *	meta,
	char *			asmbase,
	int			fddst)
{
	if (fddst >= 0)
		close(fddst);

	if (asmbase)
		free(asmbase);

	argv_free(meta);
	return -1;
}

int mdso_get_driver_ctx(
	const char **		argv,
	const char **		envp,
	uint32_t		flags,
	struct mdso_driver_ctx ** pctx)
{
	struct mdso_driver_ctx_impl *	ctx;
	struct mdso_common_ctx		cctx;
	const struct argv_option *	options;
	struct argv_meta *		meta;
	struct argv_entry *		entry;
	size_t				nunits;
	const char *			program;
	const char *			pretty;
	char *				asmbase;
	char *				dot;
	int				fddst;

	options = mdso_default_options;

	if (!(meta = argv_get(argv,options,mdso_argv_flags(flags))))
		return -1;

	nunits	= 0;
	pretty	= 0;
	asmbase = 0;
	fddst	= -1;
	program = argv_program_name(argv[0]);
	memset(&cctx,0,sizeof(cctx));

	if (!argv[1] && (flags & MDSO_DRIVER_VERBOSITY_USAGE))
		return mdso_driver_usage(program,0,options,meta);

	/* get options, count units */
	for (entry=meta->entries; entry->fopt || entry->arg; entry++) {
		if (entry->fopt) {
			switch (entry->tag) {
				case TAG_HELP:
					if (flags & MDSO_DRIVER_VERBOSITY_USAGE)
						return mdso_driver_usage(program,entry->arg,options,meta);

				case TAG_VERSION:
					cctx.drvflags |= MDSO_DRIVER_VERSION;
					break;

				case TAG_LIBNAME:
					cctx.libname = entry->arg;
					break;

				case TAG_DSTDIR:
					cctx.dstdir = entry->arg;
					break;

				case TAG_PRETTY:
					pretty = entry->arg;
					break;

				case TAG_EXPSYMS:
					cctx.fmtflags |= MDSO_OUTPUT_EXPORT_SYMS;
					break;
			}
		} else
			nunits++;
	}

	if (pretty && !strcmp(pretty,"yaml"))
		cctx.fmtflags |= MDSO_PRETTY_YAML;

	if (!cctx.libname)
		cctx.libname = "win32any";

	if (!(asmbase = strdup(cctx.libname)))
		return mdso_get_driver_ctx_fail(meta,asmbase,fddst);

	if ((dot = strchr(asmbase,'.')))
		*dot = '\0';

	if (cctx.dstdir && (fddst = mdso_dstdir_open(&cctx,asmbase)) < 0)
		return mdso_get_driver_ctx_fail(meta,asmbase,fddst);

	if (!(ctx = mdso_driver_ctx_alloc(meta,&cctx,nunits)))
		return mdso_get_driver_ctx_fail(meta,asmbase,fddst);

	ctx->asmbase		= asmbase;
	ctx->cctx.asmbase	= asmbase;

	ctx->fddst		= fddst;
	ctx->ctx.program	= program;
	ctx->ctx.cctx		= &ctx->cctx;

	*pctx = &ctx->ctx;
	return MDSO_OK;
}

int mdso_create_driver_ctx(
	const struct mdso_common_ctx *	cctx,
	struct mdso_driver_ctx **	pctx)
{
	struct argv_meta *		meta;
	struct mdso_driver_ctx_impl *	ctx;
	int				fddst  = -1;
	const char *			argv[] = {"mdso_driver",0};

	if (!(meta = argv_get(argv,mdso_default_options,0)))
		return -1;

	if (cctx->dstdir && (fddst = mdso_dstdir_open(cctx,cctx->asmbase)) < 0)
		return mdso_get_driver_ctx_fail(meta,0,fddst);

	if (!(ctx = mdso_driver_ctx_alloc(meta,cctx,0)))
		return mdso_get_driver_ctx_fail(meta,0,fddst);

	ctx->ctx.cctx = &ctx->cctx;
	*pctx = &ctx->ctx;
	return MDSO_OK;
}

static void mdso_free_driver_ctx_impl(struct mdso_driver_ctx_alloc * ictx)
{
	if (ictx->ctx.fddst >= 0)
		close(ictx->ctx.fddst);

	if (ictx->ctx.asmbase)
		free(ictx->ctx.asmbase);

	argv_free(ictx->meta);
	free(ictx);
}

void mdso_free_driver_ctx(struct mdso_driver_ctx * ctx)
{
	struct mdso_driver_ctx_alloc *	ictx;
	uintptr_t			addr;

	if (ctx) {
		addr = (uintptr_t)ctx - offsetof(struct mdso_driver_ctx_alloc,ctx);
		addr = addr - offsetof(struct mdso_driver_ctx_impl,ctx);
		ictx = (struct mdso_driver_ctx_alloc *)addr;
		mdso_free_driver_ctx_impl(ictx);
	}
}
