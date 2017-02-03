/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>

#include <mdso/mdso.h>
#include <mdso/mdso_specs.h>
#include "mdso_errinfo_impl.h"

static const char * const asm_lines[] = {
	"\t.section  " MDSO_STRS_SECTION ",\"r\"\n",
	"\t.globl    .__dsostr_%s\n",
	"\t.balign   2\n\n",
	".__dsostr_%s:\n",
	"\t.ascii\t\"%s\\0\"\n\n"
	"\t.section  " MDSO_SYMS_SECTION ",\"r\"\n",
	0
};

int mdso_generate_symentry(
	const struct mdso_driver_ctx *	dctx,
	const char *			sym,
	FILE *				fout)
{
	const char * const *	line;
	const char *		alignstr;
	const char *		ptrsize;
	const char *		uscore;

	if (fprintf(fout,"\t.file     \"__%s_sym_entry.s\"\n",sym) < 0)
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
		if ((fprintf(fout,*line,sym)) < 0)
			return MDSO_FILE_ERROR(dctx);

	if (fprintf(fout,"\t.globl    __imp_%s%s\n",uscore,sym) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((fputs(alignstr,fout)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((fprintf(fout,"__imp_%s%s:\n",uscore,sym)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((fprintf(fout,"\t%s\t.__dsostr_%s\n",ptrsize,sym)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((fprintf(fout,"\t%s\t.__dsometa_%s\n",ptrsize,dctx->cctx->libname)) < 0)
		return MDSO_FILE_ERROR(dctx);

	return 0;
}
