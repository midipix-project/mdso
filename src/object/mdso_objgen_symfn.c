/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mdso/mdso.h>
#include "mdso_object_impl.h"
#include "mdso_errinfo_impl.h"
#include "perk_consts.h"
#include "perk_structs.h"

static const unsigned char jmp_code_i386[16] = {
	0xff,0x25,
	0x0,0x0,0x0,0x0,
	0x90,0x90,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0
};

static const unsigned char jmp_code_amd64[16] = {
	0xff,0x24,0x25,
	0x0,0x0,0x0,0x0,
	0x90,
	0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
};

struct mdso_symfn_code {
	unsigned char 	code[16];
};

struct mdso_symfn_object {
	struct pe_raw_coff_object_hdr	hdr;
	struct pe_raw_sec_hdr		sec[1];
	struct mdso_symfn_code		cod[1];
	struct pe_raw_coff_reloc	rel[1];
	struct pe_raw_coff_symbol	sym[7];
	struct pe_raw_coff_strtbl	cst;
};

int mdso_objgen_symfn(
	const struct mdso_driver_ctx *	dctx,
	const char *			sym,
	FILE *				fout,
	struct mdso_object *		vobj)
{
	struct mdso_symfn_object *	symfn;
	struct pe_raw_coff_symbol *	symrec;
	const unsigned char *		code;
	unsigned char *			mark;
	struct pe_aux_rec_section *	aux;
	size_t				buflen;
	uint32_t			symlen;
	uint32_t			cstlen;
	uint32_t			objlen;
	uint32_t			aattr;
	uint32_t			sattr;
	uint16_t			oattr;
	uint16_t			machine;
	uint16_t			reltype;
	uint32_t			relrva;
	uint32_t			reloff;
	uint32_t			symoff;
	uint32_t			cstoff;
	uint32_t			codoff;
	uint32_t			datoff;

	if ((buflen = strlen(sym)) > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_DATA);

	symlen = (uint32_t)buflen;
	cstlen = (3 * symlen) + 32;
	objlen = sizeof(*symfn) + cstlen;

	if (vobj && vobj->addr && (vobj->size < objlen))
		return MDSO_BUFFER_ERROR(dctx);

	if (vobj && !vobj->addr) {
		vobj->size = objlen;
		return 0;
	}

	if (vobj)
		symfn = (struct mdso_symfn_object *)vobj->addr;

	else if (!(symfn = calloc(1,objlen)))
		return MDSO_SYSTEM_ERROR(dctx);

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		code    = jmp_code_amd64;
		aattr   = PE_IMAGE_SCN_ALIGN_16BYTES;
		machine = PE_IMAGE_FILE_MACHINE_AMD64;
		reltype = PE_IMAGE_REL_AMD64_ADDR32;
		relrva  = 3;
	} else {
		code    = jmp_code_i386;
		aattr   = PE_IMAGE_SCN_ALIGN_4BYTES;
		machine = PE_IMAGE_FILE_MACHINE_I386;
		reltype = PE_IMAGE_REL_I386_DIR32;
		relrva  = 2;
	}

	sattr  = aattr;
	sattr |= PE_IMAGE_SCN_MEM_READ;
	sattr |= PE_IMAGE_SCN_MEM_EXECUTE;
	sattr |= PE_IMAGE_SCN_CNT_CODE;

	oattr  = PE_IMAGE_FILE_LINE_NUMS_STRIPPED;
	reloff = offsetof(struct mdso_symfn_object,rel);
	symoff = offsetof(struct mdso_symfn_object,sym);
	codoff = offsetof(struct mdso_symfn_object,cod);
	cstoff = offsetof(struct pe_raw_coff_strtbl,cst_data);
	datoff = 0;

	/* coff object header */
	mdso_obj_write_short(symfn->hdr.cfh_machine,machine);
	mdso_obj_write_short(symfn->hdr.cfh_num_of_sections,1);
	mdso_obj_write_long(symfn->hdr.cfh_ptr_to_sym_tbl,symoff);
	mdso_obj_write_long(symfn->hdr.cfh_num_of_syms,7);
	mdso_obj_write_short(symfn->hdr.cfh_characteristics,oattr);

	/* .text section header */
	memcpy(symfn->sec[0].sh_name,".text",5);
	mdso_obj_write_long(symfn->sec[0].sh_size_of_raw_data,16);
	mdso_obj_write_long(symfn->sec[0].sh_ptr_to_raw_data,codoff);
	mdso_obj_write_long(symfn->sec[0].sh_ptr_to_relocs,reloff);
	mdso_obj_write_short(symfn->sec[0].sh_num_of_relocs,1);
	mdso_obj_write_long(symfn->sec[0].sh_characteristics,sattr);

	/* .text section content */
	memcpy(symfn->cod,code,16);

	/* .text relocation record */
	mdso_obj_write_long(symfn->rel[0].rel_sym,6);
	mdso_obj_write_long(symfn->rel[0].rel_rva,relrva);
	mdso_obj_write_short(symfn->rel[0].rel_type,reltype);

	/* coff string table */
	mdso_obj_write_long(symfn->cst.cst_size,cstlen);

	/* coff symbol table */
	symrec = symfn->sym;
	mark   = symfn->cst.cst_data;

	/* coff symbol: .file */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_FILE;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(&symrec[0].cs_section_number[0],PE_IMAGE_SYM_DEBUG);
	mdso_obj_write_long(&symrec[1].cs_name[4],cstoff+datoff);

	memcpy(symrec[0].cs_name,".file",5);
	memcpy(&mark[0],"__",2);
	memcpy(&mark[2],sym,symlen);
	memcpy(&mark[2+symlen],"_sym_fn.s",9);

	datoff += 12 + symlen;
	mark   += 12 + symlen;
	symrec += 2;

	/* coff symbol: .text */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(symrec[0].cs_section_number,1);
	memcpy(symrec[0].cs_name,".text",5);

	aux = (struct pe_aux_rec_section *)&symrec[1];
	mdso_obj_write_long(aux->aux_size,relrva+4);
	mdso_obj_write_short(aux->aux_num_of_relocs,1);

	datoff += 0;
	mark   += 0;
	symrec += 2;

	/* coff symbol: sym */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_EXTERNAL;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(symrec[0].cs_type,PE_IMAGE_SYM_DTYPE_FUNCTION<<8);
	mdso_obj_write_short(symrec[0].cs_section_number,1);
	mdso_obj_write_long(&symrec[0].cs_name[4],cstoff+datoff);

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		memcpy(&mark[0],sym,symlen);
	} else {
		memcpy(&mark[1],sym,symlen);
		mark[0] = '_';
		datoff++;
		mark++;
	}

	datoff += 1 + symlen;
	mark   += 1 + symlen;
	symrec += 2;

	/* coff symbol: __imp_sym */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_EXTERNAL;
	symrec[0].cs_num_of_aux_symbols[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,0);
	mdso_obj_write_long(&symrec[0].cs_name[4],cstoff+datoff);

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		memcpy(&mark[0],"__imp_",6);
		memcpy(&mark[6],sym,symlen);
	} else {
		memcpy(&mark[0],"__imp_",6);
		memcpy(&mark[7],sym,symlen);
		mark[6] = '_';
		datoff++;
		mark++;
	}

	/* tada */
	if (fout)
		if (fwrite(symfn,objlen,1,fout) == 0)
			return MDSO_FILE_ERROR(dctx);

	return 0;
}
