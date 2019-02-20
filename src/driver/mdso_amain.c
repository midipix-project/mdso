/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdio.h>
#include <unistd.h>

#include <mdso/mdso.h>
#include <mdso/mdso_output.h>
#include "mdso_driver_impl.h"
#include "mdso_dprintf_impl.h"

#ifndef MDSO_DRIVER_FLAGS
#define MDSO_DRIVER_FLAGS	MDSO_DRIVER_VERBOSITY_ERRORS \
				| MDSO_DRIVER_VERBOSITY_USAGE
#endif

static const char vermsg[] = "%s%s%s (git://midipix.org/mdso): "
			     "version %s%d.%d.%d%s.\n"
			     "[commit reference: %s%s%s]\n";

static const char * const mdso_ver_color[6] = {
		"\x1b[1m\x1b[35m","\x1b[0m",
		"\x1b[1m\x1b[32m","\x1b[0m",
		"\x1b[1m\x1b[34m","\x1b[0m"
};

static const char * const mdso_ver_plain[6] = {
		"","",
		"","",
		"",""
};

static ssize_t mdso_version(struct mdso_driver_ctx * dctx, int fdout)
{
	const struct mdso_source_version * verinfo;
	const char * const * verclr;

	verinfo = mdso_source_version();
	verclr  = isatty(fdout) ? mdso_ver_color : mdso_ver_plain;

	return mdso_dprintf(
			fdout,vermsg,
			verclr[0],dctx->program,verclr[1],
			verclr[2],verinfo->major,verinfo->minor,
			verinfo->revision,verclr[3],
			verclr[4],verinfo->commit,verclr[5]);
}

static void mdso_perform_unit_actions(
	struct mdso_driver_ctx * dctx,
	struct mdso_unit_ctx *   uctx)
{
	if (uctx->cctx->fmtflags & MDSO_OUTPUT_EXPORT_SYMS)
		mdso_output_export_symbols(dctx,uctx);

	if (uctx->cctx->drvflags & MDSO_DRIVER_COMPUTE_CRC32)
		mdso_output_expsyms_crc32(dctx,uctx);

	if (uctx->cctx->drvflags & MDSO_DRIVER_COMPUTE_CRC64)
		mdso_output_expsyms_crc64(dctx,uctx);
}

static int mdso_exit(struct mdso_driver_ctx * dctx, int ret)
{
	mdso_output_error_vector(dctx);
	mdso_free_driver_ctx(dctx);
	return ret;
}

int mdso_main(char ** argv, char ** envp, const struct mdso_fd_ctx * fdctx)
{
	int				ret;
	int				fdout;
	uint64_t			flags;
	struct mdso_driver_ctx *	dctx;
	struct mdso_unit_ctx *		uctx;
	const char **			unit;

	flags = MDSO_DRIVER_FLAGS;
	fdout = fdctx ? fdctx->fdout : STDOUT_FILENO;

	if ((ret = mdso_get_driver_ctx(argv,envp,flags,fdctx,&dctx)))
		return (ret == MDSO_USAGE)
			? !argv || !argv[0] || !argv[1]
			: MDSO_ERROR;

	if (dctx->cctx->drvflags & MDSO_DRIVER_VERSION)
		if ((mdso_version(dctx,fdout)) < 0)
			return mdso_exit(dctx,MDSO_ERROR);

	if (dctx->cctx->implib)
		mdso_create_implib_archive(dctx);

	if (dctx->cctx->drvflags & MDSO_DRIVER_GENERATE_ASM)
		mdso_create_implib_sources(dctx);

	if (dctx->cctx->drvflags & MDSO_DRIVER_GENERATE_OBJECTS)
		mdso_create_implib_objects(dctx);

	for (unit=dctx->units; *unit && !dctx->errv[0]; unit++) {
		if (!(mdso_get_unit_ctx(dctx,*unit,&uctx))) {
			mdso_perform_unit_actions(dctx,uctx);
			mdso_free_unit_ctx(uctx);
		}
	}

	return mdso_exit(dctx,dctx->errv[0] ? MDSO_ERROR : MDSO_OK);
}
