/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>

#include <mdso/mdso.h>
#include "mdso_errinfo_impl.h"

static const char * const asm_lines[] = {
	"\t.section  .text\n",
	"\t.globl    %s%s\n",
	"\t.def      %s%s; .scl 2; .type 32; .endef\n\n",

	"%s%s:\n",
	"\tjmp *__imp_%s%s\n\n",
	0
};

int mdso_asmgen_symfn(
	const struct mdso_driver_ctx *	dctx,
	const char *			sym,
	FILE *				fout)
{
	const char * const * line;
	const char *         uscore;

	if (fprintf(fout,"\t.file     \".%s_symfn.s\"\n",sym) < 0)
		return MDSO_FILE_ERROR(dctx);

	uscore = (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR)
		? "" : "_";

	for (line=asm_lines; *line; line++)
		if (fprintf(fout,*line,uscore,sym) < 0)
			return MDSO_FILE_ERROR(dctx);

	return 0;
}
