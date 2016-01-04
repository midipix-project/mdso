/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <mdso/mdso.h>
#include <mdso/mdso_output.h>
#include "mdso_version.h"

#ifndef MDSO_DRIVER_FLAGS
#define MDSO_DRIVER_FLAGS	MDSO_DRIVER_VERBOSITY_ERRORS \
				| MDSO_DRIVER_VERBOSITY_USAGE
#endif

static const char vermsg[] = "%s (git://midipix.org/mdso): commit %s.\n";

static ssize_t mdso_version(struct mdso_driver_ctx * dctx)
{
	return fprintf(stdout,vermsg,dctx->program,MDSO_GIT_VERSION);
}

static void mdso_perform_unit_actions(struct mdso_unit_ctx * uctx)
{
        uint64_t flags = uctx->cctx->fmtflags;

        if (flags & MDSO_OUTPUT_EXPORT_SYMS) {
                uctx->status = mdso_output_export_symbols(uctx,uctx->cctx,stdout);
                uctx->nerrors += !!uctx->status;
        }
}

static int mdso_exit(struct mdso_driver_ctx * dctx, int nerrors)
{
	mdso_free_driver_ctx(dctx);
	return nerrors ? 2 : 0;
}

int mdso_main(int argc, const char ** argv, const char ** envp)
{
	int				ret;
	struct mdso_driver_ctx *	dctx;
	struct mdso_unit_ctx *		uctx;
	const char **			unit;

	if ((ret = mdso_get_driver_ctx(argv,envp,MDSO_DRIVER_FLAGS,&dctx)))
		return (ret == MDSO_USAGE) ? !--argc : 2;

	if (dctx->cctx->drvflags & MDSO_DRIVER_VERSION)
		if ((mdso_version(dctx)) < 0)
			return mdso_exit(dctx,2);

	for (unit=dctx->units; *unit; unit++) {
		if (!(mdso_get_unit_ctx(dctx,*unit,&uctx))) {
			mdso_perform_unit_actions(uctx);
			ret += uctx->nerrors;
			mdso_free_unit_ctx(uctx);
		}
	}

	if (*dctx->units) {
		dctx->status  =  mdso_create_implib_sources(dctx);
		dctx->nerrors += !!dctx->status;
		ret += dctx->nerrors;
	}

	return mdso_exit(dctx,ret);
}

#ifndef MDSO_IN_A_BOX

int main(int argc, const char ** argv, const char ** envp)
{
	return mdso_main(argc,argv,envp);
}

#endif
