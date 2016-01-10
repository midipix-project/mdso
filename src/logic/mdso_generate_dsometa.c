/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <mdso/mdso.h>
#include <mdso/mdso_specs.h>

static const char * const asm_hdr_lines[] = {
	"\t.file     \"__%s_dso_meta.s\"\n",

	"\t.section  " MDSO_META_SECTION ",\"r\"\n",
	"\t.globl    .__dsometa_%s\n",
	0
};

static const char * const asm_meta_lines[] = {
	"\t.long\t0	# priority\n",
	"\t.long\t0	# nsyms\n",
	"\t.long\t0	# padding\n",
	"\t%s\t0	# hashtbl\n",
	"\t%s\t0	# symtbl\n",
	"\t%s\t0	# fncarg\n",
	"\t%s\t0	# fncargarr\n",
	"\t%s\t0	# fnr1\n",
	"\t%s\t0	# fnr2\n",
	0
};

int mdso_generate_dsometa(
	const struct mdso_common_ctx *	cctx,
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

	for (line=asm_hdr_lines; *line; line++)
		if ((fprintf(fout,*line,cctx->libname)) < 0)
			return -1;

	if ((fputs(alignstr,fout)) < 0)
		return -1;

	if ((fprintf(fout,"\t%s\t%d\t# base\n",ptrsize,0)) < 0)
		return -1;

	if ((fprintf(fout,"\t%s\t%u\t# flags\n",".long",cctx->dsoflags)) < 0)
		return -1;

	for (line=asm_meta_lines; *line; line++)
		if ((fprintf(fout,*line,ptrsize)) < 0)
			return -1;

	return 0;
}
