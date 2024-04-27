/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>

#include <mdso/mdso.h>
#include <mdso/mdso_specs.h>
#include "mdso_dprintf_impl.h"
#include "mdso_errinfo_impl.h"

static const char * const asm_hdr_lines[] = {
	"\t.file     \".dsometa_%s.s\"\n",

	"\t.section  " MDSO_META_SECTION ",\"r\"\n",
	"\t.globl    .dsometa_%s\n",
	0
};

static const char * const asm_meta_lines[] = {
	"\t.long\t0\t\t# priority\n",
	"\t.long\t0\t\t# nsyms\n",
	"\t.long\t0\t\t# padding\n",
	"\t%s\t0\t\t# hashtbl\n",
	"\t%s\t0\t\t# symtbl\n",
	"\t%s\t0\t\t# fncarg\n",
	"\t%s\t0\t\t# fncargarr\n",
	"\t%s\t0\t\t# fnr1\n",
	"\t%s\t0\t\t# fnr2\n",
	0
};

static const char * const asm_libname_fmt =
	"\n\n"
	"\t.section  " MDSO_STRS_SECTION ",\"r0\"\n\n"
	".libname:\n"
	"\t.ascii\t\"%s\\0\"\n\n";

int mdso_asmgen_dsometa(
	const struct mdso_driver_ctx *	dctx,
	int				fdout)
{
	const char * const *	line;
	const char *		alignstr;
	const char *		ptrsize;

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		alignstr = "\t.balign   16\n\n";
		ptrsize  = ".quad";
	} else {
		alignstr = "\t.balign   8\n\n";
		ptrsize  = ".long";
	}

	for (line=asm_hdr_lines; *line; line++)
		if ((mdso_dprintf(fdout,*line,dctx->cctx->libname)) < 0)
			return MDSO_FILE_ERROR(dctx);

	if (mdso_dprintf(fdout,alignstr) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,".dsometa_%s:\n",dctx->cctx->libname)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,"\t%s\t%d\t\t# base\n",ptrsize,0)) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,"\t%s\t%s\t# name\n",ptrsize,".libname")) < 0)
		return MDSO_FILE_ERROR(dctx);

	if ((mdso_dprintf(fdout,"\t%s\t%u\t\t# flags\n",".long",dctx->cctx->dsoflags)) < 0)
		return MDSO_FILE_ERROR(dctx);

	for (line=asm_meta_lines; *line; line++)
		if ((mdso_dprintf(fdout,*line,ptrsize)) < 0)
			return MDSO_FILE_ERROR(dctx);

	if (mdso_dprintf(fdout,asm_libname_fmt,dctx->cctx->libname) < 0)
		return MDSO_FILE_ERROR(dctx);

	return 0;
}
