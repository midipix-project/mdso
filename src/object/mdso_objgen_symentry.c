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

struct mdso_symfn_references {
	unsigned char 	refs[16];
};

struct mdso_symentry_object {
	struct pe_raw_coff_object_hdr	hdr;
	struct pe_raw_sec_hdr		sec[2];
	struct mdso_symfn_references	ref[1];
	struct pe_raw_coff_reloc	rel[2];
	struct pe_raw_coff_symbol	sym[9];
	struct pe_raw_coff_strtbl	cst;
};

int mdso_objgen_symentry(
	const struct mdso_driver_ctx *	dctx,
	const char *			sym,
	FILE *				fout,
	struct mdso_object *		vobj)
{
	struct mdso_symentry_object *	syment;
	struct pe_raw_coff_symbol *	symrec;
	unsigned char *			mark;
	struct pe_raw_aux_rec_section *	aux;
	size_t				buflen;
	size_t				liblen;
	uint32_t			symlen;
	uint32_t			cstlen;
	uint32_t			objlen;
	uint32_t			aattr;
	uint32_t			sattr;
	uint32_t			rattr;
	uint16_t			oattr;
	uint16_t			machine;
	uint16_t			reltype;
	uint32_t			relrva;
	uint32_t			refoff;
	uint32_t			reloff;
	uint32_t			symoff;
	uint32_t			cstoff;
	uint32_t			datoff;
	uint32_t			stroff;

	if ((buflen = strlen(sym)) > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_DATA);

	if ((liblen = strlen(dctx->cctx->libname)) > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_DATA);

	symlen = (uint32_t)buflen;
	cstlen = (uint32_t)liblen + (3 * symlen) + 64;
	objlen = sizeof(*syment) + cstlen;

	if (vobj && vobj->addr && (vobj->size < objlen))
		return MDSO_BUFFER_ERROR(dctx);

	if (vobj && !vobj->addr) {
		vobj->size = objlen;
		return 0;
	}

	if (vobj)
		syment = (struct mdso_symentry_object *)vobj->addr;

	else if (!(syment = calloc(1,objlen)))
		return MDSO_SYSTEM_ERROR(dctx);

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		aattr   = PE_IMAGE_SCN_ALIGN_16BYTES;
		machine = PE_IMAGE_FILE_MACHINE_AMD64;
		reltype = PE_IMAGE_REL_AMD64_ADDR64;
		relrva  = 8;
	} else {
		aattr   = PE_IMAGE_SCN_ALIGN_4BYTES;
		machine = PE_IMAGE_FILE_MACHINE_I386;
		reltype = PE_IMAGE_REL_I386_DIR32;
		relrva  = 4;
	}

	sattr  = PE_IMAGE_SCN_ALIGN_1BYTES;
	sattr |= PE_IMAGE_SCN_MEM_READ;
	sattr |= PE_IMAGE_SCN_CNT_INITIALIZED_DATA;

	rattr  = aattr;
	rattr |= PE_IMAGE_SCN_MEM_READ;
	rattr |= PE_IMAGE_SCN_CNT_INITIALIZED_DATA;

	oattr  = PE_IMAGE_FILE_LINE_NUMS_STRIPPED;
	refoff = offsetof(struct mdso_symentry_object,ref);
	reloff = offsetof(struct mdso_symentry_object,rel);
	symoff = offsetof(struct mdso_symentry_object,sym);
	cstoff = offsetof(struct pe_raw_coff_strtbl,cst_data);
	datoff = 0;

	stroff  = objlen - symlen - 16;
	stroff += 0xf;
	stroff |= 0xf;
	stroff ^= 0xf;

	/* coff object header */
	mdso_obj_write_short(syment->hdr.cfh_machine,machine);
	mdso_obj_write_short(syment->hdr.cfh_num_of_sections,2);
	mdso_obj_write_long(syment->hdr.cfh_ptr_to_sym_tbl,symoff);
	mdso_obj_write_long(syment->hdr.cfh_num_of_syms,9);
	mdso_obj_write_short(syment->hdr.cfh_characteristics,oattr);

	/* .dsostrs section header */
	memcpy(syment->sec[0].sh_name,".dsostrs",8);
	mdso_obj_write_long(syment->sec[0].sh_size_of_raw_data,symlen+1);
	mdso_obj_write_long(syment->sec[0].sh_ptr_to_raw_data,stroff);
	mdso_obj_write_long(syment->sec[0].sh_characteristics,sattr);

	/* .dsosyms section header */
	memcpy(syment->sec[1].sh_name,".dsosyms",8);
	mdso_obj_write_long(syment->sec[1].sh_size_of_raw_data,2*relrva);
	mdso_obj_write_long(syment->sec[1].sh_ptr_to_raw_data,refoff);
	mdso_obj_write_long(syment->sec[1].sh_ptr_to_relocs,reloff);
	mdso_obj_write_short(syment->sec[1].sh_num_of_relocs,2);
	mdso_obj_write_long(syment->sec[1].sh_characteristics,rattr);

	/* .dsosyms relocation record: .symstr */
	mdso_obj_write_long(syment->rel[0].rel_sym,6);
	mdso_obj_write_long(syment->rel[0].rel_rva,0);
	mdso_obj_write_short(syment->rel[0].rel_type,reltype);

	/* .dsosyms relocation record: .dsometa_libname */
	mdso_obj_write_long(syment->rel[1].rel_sym,8);
	mdso_obj_write_long(syment->rel[1].rel_rva,relrva);
	mdso_obj_write_short(syment->rel[1].rel_type,reltype);

	/* coff string table */
	mdso_obj_write_long(syment->cst.cst_size,cstlen);

	/* coff symbol table */
	symrec = syment->sym;
	mark   = syment->cst.cst_data;

	/* coff symbol: .file */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_FILE;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(&symrec[0].cs_section_number[0],PE_IMAGE_SYM_DEBUG);
	mdso_obj_write_long(&symrec[1].cs_name[4],cstoff+datoff);

	memcpy(symrec[0].cs_name,".file",5);
	memcpy(&mark[0],".",1);
	memcpy(&mark[1],sym,symlen);
	memcpy(&mark[1+symlen],"_symentry.s",11);

	datoff += 13 + symlen;
	mark   += 13 + symlen;
	symrec += 2;

	/* coff symbol: .dsostrs */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(symrec[0].cs_section_number,1);
	memcpy(symrec[0].cs_name,".dsostrs",8);

	aux = (struct pe_raw_aux_rec_section *)&symrec[1];
	mdso_obj_write_long(aux->aux_size,symlen+1);
	mdso_obj_write_short(aux->aux_num_of_relocs,0);

	datoff += 0;
	mark   += 0;
	symrec += 2;

	/* coff symbol: .dsosyms */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(symrec[0].cs_section_number,2);
	memcpy(symrec[0].cs_name,".dsosyms",8);

	aux = (struct pe_raw_aux_rec_section *)&symrec[1];
	mdso_obj_write_long(aux->aux_size,2*relrva);
	mdso_obj_write_short(aux->aux_num_of_relocs,2);

	datoff += 0;
	mark   += 0;
	symrec += 2;

	/* coff symbol: .symstr */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_symbols[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,1);
	memcpy(symrec[0].cs_name,".symstr",7);

	datoff += 0;
	mark   += 0;
	symrec += 1;

	/* coff symbol: __imp_sym */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_EXTERNAL;
	symrec[0].cs_num_of_aux_symbols[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,2);
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

	datoff += 7 + symlen;
	mark   += 7 + symlen;
	symrec += 1;

	/* coff symbol: .dsometa_libname */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_EXTERNAL;
	symrec[0].cs_num_of_aux_symbols[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,0);
	mdso_obj_write_long(&symrec[0].cs_name[4],cstoff+datoff);

	memcpy(&mark[0],".dsometa_",9);
	memcpy(&mark[9],dctx->cctx->libname,liblen);

	/* .symstr */
	mark = syment->hdr.cfh_machine;
	memcpy(&mark[stroff],sym,symlen);

	/* tada */
	if (fout)
		if (fwrite(syment,objlen,1,fout) == 0)
			return MDSO_FILE_ERROR(dctx);

	if (!vobj)
		free(syment);

	return 0;
}
