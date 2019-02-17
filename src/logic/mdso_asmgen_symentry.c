/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>

#include <mdso/mdso.h>
#include <mdso/mdso_specs.h>
#include "mdso_dprintf_impl.h"
#include "mdso_errinfo_impl.h"

static const char * const asm_lines[] = {
	"\t.section  " MDSO_STRS_SECTION "$%s,\"r0\"\n\n",
	".symstr_%s:\n",
	"\t.ascii\t\"%s\\0\"\n"
	"\t.linkonce discard\n\n",
	"\t.section  " MDSO_SYMS_SECTION "$%s,\"r\"\n",
	0
};

int mdso_asmgen_symentry(
	const struct mdso_driver_ctx *	dctx,
	const char *			sym,
	int				fdout)
{
	const char * const *	line;
	const char *		alignstr;
	const char *		ptrsize;
	const char *		uscore;

	if (mdso_dprintf(fdout,"\t.file     \".%s_symentry.s\"\n",sym) < 0)
		return MDSO_FILE_ERROR(dctx);

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		alignstr = "\t.balign   16\n\n";
		ptrsize  = ".quad";
		uscore   = "";
	} else {
		alignstr = "\t.balign   8\n\n";
		ptrsize  = ".long";
		uscore   = "_";
	}

	for (line=asm_lines; *line; line++)
		if ((mdso_dprintf(fdout,*line,sym)) < 0)
			return MDSO_FILE_ERROR(dctx);

	if (mdso_dprintf(fdout,"\t.globl    __imp_%s%s\n",uscore,sym) < 0)
		return MDSO_FILE_ERROR(dctx);

	if (mdso_dprintf(fdout,alignstr) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,"__imp_%s%s:\n",uscore,sym)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,"\t%s\t.symstr_%s\n",ptrsize,sym)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,"\t%s\t.dsometa_%s\n",ptrsize,dctx->cctx->libname)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,"\t.linkonce discard\n")) < 0)
		return MDSO_FILE_ERROR(dctx);

	return 0;
}
