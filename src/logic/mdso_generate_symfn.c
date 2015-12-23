/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015  Z. Gilboa                               */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <mdso/mdso.h>

static const char * const asm_lines[] = {
	"\t.file     \"__%s_sym_fn.s\"\n",
	"\t.section  .text\n",
	"\t.globl    %s\n",
	"\t.def      %s; .scl 2; .type 32; .endef\n\n",

	"%s:\n",
	"\tjmp *__imp_%s\n\n",
	0
};

int mdso_generate_symfn(const char * sym, FILE * fout)
{
	const char * const * line;

	for (line=asm_lines; *line; line++)
		if ((fprintf(fout,*line,sym)) < 0)
			return -1;

	return 0;
}
