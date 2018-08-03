/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
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
#include <mdso/mdso_specs.h>
#include "mdso_version.h"
#include "mdso_driver_impl.h"
#include "argv/argv.h"

#define MDSO_LOADER_FLAGS_MASK	(MDSO_FLAG_LOADER_PATH \
				| MDSO_FLAG_LDSO_LIB   \
				| MDSO_FLAG_PEB_PATH   \
				| MDSO_FLAG_SYSTEM32)

/* package info */
static const struct mdso_source_version mdso_src_version = {
	MDSO_TAG_VER_MAJOR,
	MDSO_TAG_VER_MINOR,
	MDSO_TAG_VER_PATCH,
	MDSO_GIT_VERSION
};

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
	int				fdout,
	const char *			program,
	const char *			arg,
	const struct argv_option **	optv,
	struct argv_meta *		meta)
{
	char header[512];

	snprintf(header,sizeof(header),
		"Usage: %s [options] <file>...\n" "Options:\n",
		program);

	argv_usage(fdout,header,optv,arg);
	argv_free(meta);

	return MDSO_USAGE;
}

static struct mdso_driver_ctx_impl * mdso_driver_ctx_alloc(
	struct argv_meta *		meta,
	const struct mdso_fd_ctx *	fdctx,
	const struct mdso_common_ctx *	cctx,
	size_t				nunits)
{
	struct mdso_driver_ctx_alloc *	ictx;
	size_t				size;
	struct argv_entry *		entry;
	const char **			units;
	int				elements;

	size =  sizeof(struct mdso_driver_ctx_alloc);
	size += (nunits+1)*sizeof(const char *);

	if (!(ictx = calloc(1,size)))
		return 0;

	memcpy(&ictx->ctx.fdctx,fdctx,sizeof(*fdctx));
	memcpy(&ictx->ctx.cctx,cctx,sizeof(*cctx));

	for (entry=meta->entries,units=ictx->units; entry->fopt || entry->arg; entry++)
		if (!entry->fopt)
			*units++ = entry->arg;

	elements = sizeof(ictx->ctx.erribuf) / sizeof(*ictx->ctx.erribuf);

	ictx->ctx.errinfp  = &ictx->ctx.erriptr[0];
	ictx->ctx.erricap  = &ictx->ctx.erriptr[--elements];

	ictx->meta = meta;
	ictx->ctx.fdtmpin = -1;
	ictx->ctx.ctx.units = ictx->units;
	ictx->ctx.ctx.errv  = ictx->ctx.errinfp;
	return &ictx->ctx;
}

static int mdso_dstdir_open(int fdcwd, const char * dstdir, const char * asmbase)
{
	int fdtop;
	int fddst;
	int dirmode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

	if (!dstdir)
		fdtop = fdcwd;

	else if ((fdtop = openat(fdcwd,dstdir,O_DIRECTORY)) >= 0)
		(void)0;

	else if (mkdirat(fdcwd,dstdir,dirmode) < 0)
			return -1;

	else if ((fdtop = openat(fdcwd,dstdir,O_DIRECTORY)) < 0)
		return -1;


	if ((fddst = openat(fdtop,asmbase,O_DIRECTORY)) >= 0)
		(void)0;

	else if (mkdirat(fdtop,asmbase,dirmode))
		fddst = AT_FDCWD;

	else if ((fddst = openat(fdtop,asmbase,O_DIRECTORY)) < 0)
		fddst = AT_FDCWD;

	close(fdtop);

	return fddst;
}

static int mdso_get_driver_ctx_fail(
	struct argv_meta *	meta,
	char *			implib,
	char *			asmbase,
	int			fddst)
{
	if (fddst != AT_FDCWD)
		close(fddst);

	if (implib)
		free(implib);

	if (asmbase)
		free(asmbase);

	argv_free(meta);

	return -1;
}

int mdso_get_driver_ctx(
	char **				argv,
	char **				envp,
	uint32_t			flags,
	const struct mdso_fd_ctx *	fdctx,
	struct mdso_driver_ctx **	pctx)
{
	struct mdso_driver_ctx_impl *	ctx;
	struct mdso_common_ctx		cctx;
	const struct argv_option *	optv[MDSO_OPTV_ELEMENTS];
	struct argv_meta *		meta;
	struct argv_entry *		entry;
	struct argv_entry *		machine;
	size_t				nunits;
	const char *			program;
	const char *			pretty;
	char *				implib;
	char *				asmbase;
	char *				dot;
	int				fddst;

	(void)envp;

	if (!fdctx) {
		fdctx = &(const struct mdso_fd_ctx) {
			.fdin  = STDIN_FILENO,
			.fdout = STDOUT_FILENO,
			.fderr = STDERR_FILENO,
			.fdlog = (-1),
			.fdcwd = AT_FDCWD,
			.fddst = AT_FDCWD,
		};
	}

	argv_optv_init(mdso_default_options,optv);

	if (!(meta = argv_get(
			argv,optv,
			mdso_argv_flags(flags),
			fdctx->fderr)))
		return -1;

	/* cctx init, option defaults */
	memset(&cctx,0,sizeof(cctx));

	machine = 0;
	nunits	= 0;
	pretty	= 0;
	implib  = 0;
	asmbase = 0;
	program = argv_program_name(argv[0]);

	cctx.drvflags = flags;
	cctx.dsoflags = MDSO_FLAG_LOADER_PATH;

	if (!argv[1] && (flags & MDSO_DRIVER_VERBOSITY_USAGE))
		return mdso_driver_usage(
			fdctx->fderr,
			program,0,
			optv,meta);

	/* get options, count units */
	for (entry=meta->entries; entry->fopt || entry->arg; entry++) {
		if (entry->fopt) {
			switch (entry->tag) {
				case TAG_HELP:
					if (flags & MDSO_DRIVER_VERBOSITY_USAGE)
						return mdso_driver_usage(
							fdctx->fdout,
							program,
							entry->arg,
							optv,meta);

				case TAG_VERSION:
					cctx.drvflags |= MDSO_DRIVER_VERSION;
					break;

				case TAG_ASM:
					cctx.drvflags |= MDSO_DRIVER_GENERATE_ASM;
					break;

				case TAG_OBJ:
					cctx.drvflags |= MDSO_DRIVER_GENERATE_OBJECTS;
					break;

				case TAG_QUAD_PTR:
					machine = entry;

					if (!(strcmp(entry->arg,"64")))
						cctx.drvflags |= MDSO_DRIVER_QUAD_PTR;
					else
						cctx.drvflags &= ~(uint64_t)MDSO_DRIVER_QUAD_PTR;
					break;

				case TAG_IMPLIB:
					cctx.implib = entry->arg;
					break;

				case TAG_LIBPATH:
					cctx.dsoflags &= ~(uint64_t)MDSO_LOADER_FLAGS_MASK;

					if (!(strcmp(entry->arg,"loader")))
						cctx.dsoflags |= MDSO_FLAG_LOADER_PATH;

					else if (!(strcmp(entry->arg,"ldso")))
						cctx.dsoflags |= MDSO_FLAG_LDSO_LIB;

					else if (!(strcmp(entry->arg,"peb")))
						cctx.dsoflags |= MDSO_FLAG_PEB_PATH;

					else if (!(strcmp(entry->arg,"system32")))
						cctx.dsoflags |= MDSO_FLAG_SYSTEM32;
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


	if (!machine && MDSO_DRIVER_PE_HOST && (sizeof(size_t) == 8))
		cctx.drvflags |= MDSO_DRIVER_QUAD_PTR;

	else if (!machine && MDSO_DRIVER_WINNT_HOST && (sizeof(size_t) == 8))
		cctx.drvflags |= MDSO_DRIVER_QUAD_PTR;

	else if (!machine && strstr(program,"64"))
		cctx.drvflags |= MDSO_DRIVER_QUAD_PTR;


	if (!nunits && !(cctx.drvflags & MDSO_DRIVER_VERSION))
		return mdso_driver_usage(
			fdctx->fderr,
			program,0,
			optv,meta);

	if (pretty && !strcmp(pretty,"yaml"))
		cctx.fmtflags |= MDSO_PRETTY_YAML;

	if (!cctx.libname)
		cctx.libname = "win32any";

	if (cctx.implib && !(implib = strdup(cctx.implib)))
		return mdso_get_driver_ctx_fail(meta,0,0,AT_FDCWD);

	if (!(asmbase = strdup(cctx.libname)))
		return mdso_get_driver_ctx_fail(meta,implib,0,AT_FDCWD);

	if ((dot = strchr(asmbase,'.')))
		*dot = '\0';


	if (!cctx.dstdir && !(cctx.drvflags & MDSO_DRIVER_GENERATE_OBJECTS))
		fddst = AT_FDCWD;

	else if ((fddst = mdso_dstdir_open(fdctx->fdcwd,cctx.dstdir,asmbase)) < 0)
		return mdso_get_driver_ctx_fail(meta,implib,asmbase,AT_FDCWD);


	if (!(ctx = mdso_driver_ctx_alloc(meta,fdctx,&cctx,nunits)))
		return mdso_get_driver_ctx_fail(meta,implib,asmbase,fddst);

	ctx->implib		= implib;
	ctx->asmbase		= asmbase;
	ctx->cctx.asmbase	= asmbase;
	ctx->fddst		= fddst;

	ctx->ctx.program	= program;
	ctx->ctx.cctx		= &ctx->cctx;
	ctx->fdctx.fddst	= fddst;

	*pctx = &ctx->ctx;

	return MDSO_OK;
}

static void mdso_free_driver_ctx_impl(struct mdso_driver_ctx_alloc * ictx)
{
	if (ictx->ctx.fddst != AT_FDCWD)
		close(ictx->ctx.fddst);

	if (ictx->ctx.fdtmpin >= 0)
		close(ictx->ctx.fdtmpin);

	if (ictx->ctx.asmbase)
		free(ictx->ctx.asmbase);

	if (ictx->ctx.implib)
		free(ictx->ctx.implib);

	argv_free(ictx->meta);
	free(ictx);
}

void mdso_free_driver_ctx(struct mdso_driver_ctx * ctx)
{
	struct mdso_driver_ctx_alloc *	ictx;
	uintptr_t			addr;

	if (ctx) {
		addr = (uintptr_t)ctx - offsetof(struct mdso_driver_ctx_impl,ctx);
		addr = addr - offsetof(struct mdso_driver_ctx_alloc,ctx);
		ictx = (struct mdso_driver_ctx_alloc *)addr;
		mdso_free_driver_ctx_impl(ictx);
	}
}

const struct mdso_source_version * mdso_source_version(void)
{
	return &mdso_src_version;
}


int mdso_get_driver_fdctx(
	const struct mdso_driver_ctx *	dctx,
	struct mdso_fd_ctx *		fdctx)
{
	struct mdso_driver_ctx_impl *	ictx;

	ictx = mdso_get_driver_ictx(dctx);

	fdctx->fdin  = ictx->fdctx.fdin;
	fdctx->fdout = ictx->fdctx.fdout;
	fdctx->fderr = ictx->fdctx.fderr;
	fdctx->fdlog = ictx->fdctx.fdlog;
	fdctx->fdcwd = ictx->fdctx.fdcwd;
	fdctx->fddst = ictx->fdctx.fddst;

	return 0;
}

int mdso_set_driver_fdctx(
	struct mdso_driver_ctx *	dctx,
	const struct mdso_fd_ctx *	fdctx)
{
	struct mdso_driver_ctx_impl *	ictx;

	ictx = mdso_get_driver_ictx(dctx);

	ictx->fdctx.fdin  = fdctx->fdin;
	ictx->fdctx.fdout = fdctx->fdout;
	ictx->fdctx.fderr = fdctx->fderr;
	ictx->fdctx.fdlog = fdctx->fdlog;
	ictx->fdctx.fdcwd = fdctx->fdcwd;

	if (ictx->fdctx.fddst == fdctx->fddst) {
		(void)0;

	} else if (ictx->fddst == AT_FDCWD) {
		ictx->fdctx.fddst = fdctx->fddst;

	} else {
		close(ictx->fddst);
		ictx->fddst       = AT_FDCWD;
		ictx->fdctx.fddst = fdctx->fddst;
	}

	return 0;
}
