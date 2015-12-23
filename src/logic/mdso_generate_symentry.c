/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015  Z. Gilboa                               */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <mdso/mdso.h>
#include <mdso/mdso_specs.h>

static const char * const asm_lines[] = {
	"\t.file     \"__%s_sym_entry.s\"\n",

	"\t.section  " MDSO_STRS_SECTION ",\"r\"\n",
	"\t.globl    .__dsostr_%s\n",
	"\t.balign   2\n\n",
	".__dsostr_%s:\n",
	"\t.ascii\t\"%s\\0\"\n\n"

	"\t.section  " MDSO_SYMS_SECTION ",\"r\"\n",
	"\t.globl    __imp_%s\n",
	0
};

int mdso_generate_symentry(
	const struct mdso_common_ctx *	cctx,
	const char *			sym,
	FILE *				fout)
{
	const char * const *	line;
	const char *		alignstr;
	const char *		ptrsize;

	if (cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		alignstr = "\t.balign   16\n\n";
		ptrsize  = ".quad";
	} else {
		alignstr = "\t.balign   8\n\n";
		ptrsize  = ".long";
	}

	for (line=asm_lines; *line; line++)
		if ((fprintf(fout,*line,sym)) < 0)
			return -1;

	if ((fputs(alignstr,fout)) < 0)
		return -1;

	if ((fprintf(fout,"__imp_%s:\n",sym)) < 0)
		return -1;

	if ((fprintf(fout,"\t%s\t.__dsostr_%s\n",ptrsize,sym)) < 0)
		return -1;

	if ((fprintf(fout,"\t%s\t.__dsometa_%s\n",ptrsize,cctx->libname)) < 0)
		return -1;

	return 0;
}
