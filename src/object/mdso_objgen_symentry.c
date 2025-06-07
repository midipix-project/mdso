/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <mdso/mdso.h>
#include <mdso/mdso_specs.h>
#include "mdso_object_impl.h"
#include "mdso_errinfo_impl.h"
#include "perk_consts.h"
#include "perk_structs.h"

struct pe_raw_coff_strtbl_impl {
	unsigned char	cst_size	[0x04];
	unsigned char	cst_data	[0x10];
};

struct mdso_symfn_refs {
	unsigned char refs[16];
};

struct mdso_symentry_object {
	struct pe_raw_coff_object_hdr	hdr;
	struct pe_raw_sec_hdr		sec[2];
	struct mdso_symfn_refs		ref[1];
	struct pe_raw_coff_reloc	rel[2];
	struct pe_raw_coff_symbol	sym[9];
	struct pe_raw_coff_strtbl_impl  cst;
};

static void mdso_obj_write_secoff(unsigned char * ch, uint64_t secoff)
{
	*ch++ = '/';
	mdso_obj_write_dec(ch,secoff);
}

static void mdso_obj_write_sym_symentry(char * ch, const char * sym)
{
	*ch++ = '.';

	for (; *sym; )
		*ch++ = *sym++;

	memcpy(ch,"_symentry.s",11);
}

static void mdso_obj_write_sym_symstr(char * ch, const char * sym)
{
	memcpy(ch,".symstr_",8);
	ch = &ch[8];

	for (; *sym; )
		*ch++ = *sym++;
}

static void mdso_obj_write_sym_impstr(char * ch, uint32_t uscore, const char * sym)
{
	memcpy(ch,"__imp_",6);
	ch = &ch[6];

	if (uscore)
		*ch++ = '_';

	for (; *sym; )
		*ch++ = *sym++;
}

static void mdso_obj_write_sym_secstr(char * ch, const char * secname, const char * sym)
{
	for (; *secname; )
		*ch++ = *secname++;

	*ch++ = '$';

	for (; *sym; )
		*ch++ = *sym++;
}

static void mdso_obj_write_libname(char * ch, const char * secname, const char * sym)
{
	for (; *secname; )
		*ch++ = *secname++;

	*ch++ = '_';

	for (; *sym; )
		*ch++ = *sym++;
}

int mdso_objgen_symentry(
	const struct mdso_driver_ctx *	dctx,
	const char *			sym,
	struct mdso_object *		vobj)
{
	struct mdso_symentry_object *	syment;
	struct pe_raw_coff_symbol *	symrec;
	void *				addr;
	void *				mark;
	char *				ch;
	char *				strtbl;
	const char *			src;
	struct pe_raw_aux_rec_section *	aux;
	size_t				liblen;
	uint32_t			symlen;
	size_t				cstlen;
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
	uint32_t			uscore;
	uint32_t			stroff;
	uint32_t			stroff_cstdata;
	uint32_t			stroff_file;
	uint32_t			stroff_dsosyms;
	uint32_t			stroff_dsostrs;
	uint32_t			stroff_symstr;
	uint32_t			stroff_impsym;
	uint32_t			stroff_libname;
	uint32_t			stroff_null;

	if ((symlen = (uint32_t)strlen(sym)) > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_DATA);

	if ((liblen = strlen(dctx->cctx->libname)) > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_DATA);

	stroff_cstdata = offsetof(struct pe_raw_coff_strtbl,cst_data);
	stroff_file    = stroff_cstdata;
	stroff_dsosyms = stroff_file    + 13 + symlen;   /* .foo_symentry.s  */
	stroff_dsostrs = stroff_dsosyms + 10 + symlen;   /* .dsosyms$foo     */
	stroff_symstr  = stroff_dsostrs + 10 + symlen;   /* .dsostrs$foo     */
	stroff_impsym  = stroff_symstr  + 9  + symlen;   /* .symstr_foo      */
	stroff_libname = stroff_impsym  + 8  + symlen;   /* __imp__foo       */
	stroff_null    = stroff_libname + 10 + liblen;   /* .dsometa_libname */

	stroff  = offsetof(struct mdso_symentry_object,cst) + stroff_null;
	stroff += 0xf;
	stroff |= 0xf;
	stroff ^= 0xf;

	cstlen  = stroff_null;
	objlen  = stroff + symlen + 1;
	uscore  = !(dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR);

	if (vobj->addr && (vobj->size < objlen))
		return MDSO_BUFFER_ERROR(dctx);

	if ((addr = vobj->addr)) {
		(void)0;

	} else {
		vobj->size       = objlen;
		vobj->mapstrsnum = 1;
		vobj->mapstrslen = 7 + uscore + symlen;

		if (!vobj->name)
			return 0;

		else if (mdso_create_object(dctx,vobj) < 0)
			return MDSO_NESTED_ERROR(dctx);
	}

	syment = (struct mdso_symentry_object *)vobj->addr;

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		aattr   = PE_IMAGE_SCN_ALIGN_16BYTES;
		machine = PE_IMAGE_FILE_MACHINE_AMD64;
		reltype = PE_IMAGE_REL_AMD64_ADDR32NB;
		relrva  = 8;
	} else {
		aattr   = PE_IMAGE_SCN_ALIGN_4BYTES;
		machine = PE_IMAGE_FILE_MACHINE_I386;
		reltype = PE_IMAGE_REL_I386_DIR32NB;
		relrva  = 4;
	}

	sattr  = PE_IMAGE_SCN_ALIGN_1BYTES;
	sattr |= PE_IMAGE_SCN_MEM_READ;
	sattr |= PE_IMAGE_SCN_CNT_INITIALIZED_DATA;
	sattr |= PE_IMAGE_SCN_LNK_COMDAT;

	rattr  = aattr;
	rattr |= PE_IMAGE_SCN_MEM_READ;
	rattr |= PE_IMAGE_SCN_CNT_INITIALIZED_DATA;
	rattr |= PE_IMAGE_SCN_LNK_COMDAT;

	oattr  = PE_IMAGE_FILE_LINE_NUMS_STRIPPED;
	refoff = offsetof(struct mdso_symentry_object,ref);
	reloff = offsetof(struct mdso_symentry_object,rel);
	symoff = offsetof(struct mdso_symentry_object,sym);

	/* coff object header */
	mdso_obj_write_short(syment->hdr.cfh_machine,machine);
	mdso_obj_write_short(syment->hdr.cfh_num_of_sections,2);
	mdso_obj_write_long(syment->hdr.cfh_ptr_to_sym_tbl,symoff);
	mdso_obj_write_long(syment->hdr.cfh_num_of_sym_recs,9);
	mdso_obj_write_short(syment->hdr.cfh_characteristics,oattr);

	/* .dsostrs section header */
	mdso_obj_write_secoff(syment->sec[0].sh_name,stroff_dsostrs);
	mdso_obj_write_long(syment->sec[0].sh_size_of_raw_data,symlen+1);
	mdso_obj_write_long(syment->sec[0].sh_ptr_to_raw_data,stroff);
	mdso_obj_write_long(syment->sec[0].sh_characteristics,sattr);

	/* .dsosyms section header */
	mdso_obj_write_secoff(syment->sec[1].sh_name,stroff_dsosyms);
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
	mark   = &syment->cst;
	strtbl = mark;

	/* coff symbol table */
	symrec = syment->sym;

	/* coff symbol: .file */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_FILE;
	symrec[0].cs_num_of_aux_recs[0] = 1;

	mdso_obj_write_short(&symrec[0].cs_section_number[0],PE_IMAGE_SYM_DEBUG);
	mdso_obj_write_long(&symrec[1].cs_name[4],stroff_file);

	memcpy(symrec[0].cs_name,".file",5);
	mdso_obj_write_sym_symentry(&strtbl[stroff_file],sym);

	symrec += 2;

	/* coff symbol: .dsostrs */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_recs[0] = 1;

	mdso_obj_write_short(symrec[0].cs_section_number,1);
	mdso_obj_write_long(&symrec[0].cs_name[4],stroff_dsostrs);
	mdso_obj_write_sym_secstr(&strtbl[stroff_dsostrs],MDSO_STRS_SECTION,sym);

	aux = (struct pe_raw_aux_rec_section *)&symrec[1];
	mdso_obj_write_long(aux->aux_size,symlen+1);
	mdso_obj_write_short(aux->aux_num_of_relocs,0);

	symrec += 2;

	/* coff symbol: .dsosyms */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_recs[0] = 1;

	mdso_obj_write_short(symrec[0].cs_section_number,2);
	mdso_obj_write_long(&symrec[0].cs_name[4],stroff_dsosyms);
	mdso_obj_write_sym_secstr(&strtbl[stroff_dsosyms],MDSO_SYMS_SECTION,sym);

	aux = (struct pe_raw_aux_rec_section *)&symrec[1];
	mdso_obj_write_long(aux->aux_size,2*relrva);
	mdso_obj_write_short(aux->aux_num_of_relocs,2);

	symrec += 2;

	/* coff symbol: .symstr */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_recs[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,1);
	mdso_obj_write_long(&symrec[0].cs_name[4],stroff_symstr);
	mdso_obj_write_sym_symstr(&strtbl[stroff_symstr],sym);

	symrec += 1;

	/* coff symbol: __imp_sym */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_EXTERNAL;
	symrec[0].cs_num_of_aux_recs[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,2);
	mdso_obj_write_long(&symrec[0].cs_name[4],stroff_impsym);
	mdso_obj_write_sym_impstr(&strtbl[stroff_impsym],uscore,sym);

	symrec += 1;

	/* archive symbol map */
	if (vobj->mapstrs) {
		ch  = vobj->mapstrs;
		src = &strtbl[stroff_impsym];

		for (; *src; )
			*ch++ = *src++;
	}

	/* coff symbol: .dsometa_libname */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_EXTERNAL;
	symrec[0].cs_num_of_aux_recs[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,0);
	mdso_obj_write_long(&symrec[0].cs_name[4],stroff_libname);

	mdso_obj_write_libname(&strtbl[stroff_libname],
		MDSO_META_SECTION,
		dctx->cctx->libname);

	/* .symstr */
	mark = syment;
	ch   = mark;
	memcpy(&ch[stroff],sym,symlen);

	/* fs object unmap */
	if (!addr)
		munmap(vobj->addr,vobj->size);

	/* tada */
	return 0;
}
