/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <mdso/mdso.h>
#include <mdso/mdso_output.h>
#include "mdso_driver_impl.h"

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

static ssize_t mdso_version(struct mdso_driver_ctx * dctx)
{
	const struct mdso_source_version * verinfo;
	const char * const * verclr;

	verinfo = mdso_source_version();
	verclr  = isatty(STDOUT_FILENO) ? mdso_ver_color : mdso_ver_plain;

	return fprintf(stdout,vermsg,
			verclr[0],dctx->program,verclr[1],
			verclr[2],verinfo->major,verinfo->minor,
			verinfo->revision,verclr[3],
			verclr[4],verinfo->commit,verclr[5]);
}

static void mdso_perform_unit_actions(
	struct mdso_driver_ctx * dctx,
	struct mdso_unit_ctx *   uctx)
{
        uint64_t flags = uctx->cctx->fmtflags;

        if (flags & MDSO_OUTPUT_EXPORT_SYMS)
		mdso_output_export_symbols(
			dctx,uctx,stdout);
}

static int mdso_exit(struct mdso_driver_ctx * dctx, int ret)
{
	mdso_output_error_vector(dctx);
	mdso_free_driver_ctx(dctx);
	return ret;
}

int mdso_main(int argc, char ** argv, char ** envp)
{
	int				ret;
	struct mdso_driver_ctx *	dctx;
	struct mdso_unit_ctx *		uctx;
	const char **			unit;

	if ((ret = mdso_get_driver_ctx(argv,envp,MDSO_DRIVER_FLAGS,&dctx)))
		return (ret == MDSO_USAGE)
			? !--argc
			: MDSO_ERROR;

	if (dctx->cctx->drvflags & MDSO_DRIVER_VERSION)
		if ((mdso_version(dctx)) < 0)
			return mdso_exit(dctx,MDSO_ERROR);

	for (unit=dctx->units; *unit && !dctx->errv[0]; unit++) {
		if (!(mdso_get_unit_ctx(dctx,*unit,&uctx))) {
			mdso_perform_unit_actions(dctx,uctx);
			mdso_free_unit_ctx(uctx);
		}
	}

	if (*dctx->units && !dctx->errv[0])
		mdso_create_implib_sources(dctx);

	return mdso_exit(dctx,dctx->errv[0] ? MDSO_ERROR : MDSO_OK);
}
