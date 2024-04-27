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

struct mdso_unified_record {
	unsigned char 	data[80];
};

struct mdso_dsometa_object {
	struct pe_raw_coff_object_hdr	hdr;
	struct pe_raw_sec_hdr		sec[2];
	struct mdso_unified_record	rec[1];
	struct pe_raw_coff_reloc	rel[1];
	struct pe_raw_coff_symbol	sym[8];
	struct pe_raw_coff_strtbl	cst;
};

int mdso_objgen_dsometa(
	const struct mdso_driver_ctx *	dctx,
	struct mdso_object *		vobj)
{
	struct mdso_dsometa_object *	dsometa;
	struct pe_raw_coff_symbol *	symrec;
	void *				addr;
	unsigned char *			mark;
	struct pe_raw_aux_rec_section *	aux;
	size_t				buflen;
	uint32_t			liblen;
	uint32_t			cstlen;
	uint32_t			objlen;
	uint32_t			aattr;
	uint32_t			sattr;
	uint32_t			rattr;
	uint16_t			oattr;
	uint16_t			machine;
	uint16_t			reltype;
	uint32_t			relrva;
	uint32_t			reclen;
	uint32_t			recoff;
	uint32_t			reloff;
	uint32_t			symoff;
	uint32_t			cstoff;
	uint32_t			datoff;
	uint32_t			stroff;

	if ((buflen = strlen(dctx->cctx->libname)) > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_DATA);

	liblen = (uint32_t)buflen;
	cstlen = (3 * liblen) + 48;
	objlen = sizeof(*dsometa) + cstlen;

	if (vobj->addr && (vobj->size < objlen))
		return MDSO_BUFFER_ERROR(dctx);

	if ((addr = vobj->addr)) {
		(void)0;

	} else {
		vobj->size       = objlen;
		vobj->mapstrsnum = 1;
		vobj->mapstrslen = 10 + liblen;

		if (!vobj->name)
			return 0;

		else if (mdso_create_object(dctx,vobj) < 0)
			return MDSO_NESTED_ERROR(dctx);
	}

	dsometa = (struct mdso_dsometa_object *)vobj->addr;

	if (dctx->cctx->drvflags & MDSO_DRIVER_QUAD_PTR) {
		reclen  = sizeof(struct mdso_meta_record_m64);
		aattr   = PE_IMAGE_SCN_ALIGN_16BYTES;
		machine = PE_IMAGE_FILE_MACHINE_AMD64;
		reltype = PE_IMAGE_REL_AMD64_ADDR64;
		relrva  = 8;
	} else {
		reclen  = sizeof(struct mdso_meta_record_m32);
		aattr   = PE_IMAGE_SCN_ALIGN_8BYTES;
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
	recoff = offsetof(struct mdso_dsometa_object,rec);
	reloff = offsetof(struct mdso_dsometa_object,rel);
	symoff = offsetof(struct mdso_dsometa_object,sym);
	cstoff = offsetof(struct pe_raw_coff_strtbl,cst_data);
	datoff = 0;

	stroff  = objlen - liblen - 16;
	stroff += 0xf;
	stroff |= 0xf;
	stroff ^= 0xf;

	/* coff object header */
	mdso_obj_write_short(dsometa->hdr.cfh_machine,machine);
	mdso_obj_write_short(dsometa->hdr.cfh_num_of_sections,2);
	mdso_obj_write_long(dsometa->hdr.cfh_ptr_to_sym_tbl,symoff);
	mdso_obj_write_long(dsometa->hdr.cfh_num_of_syms,8);
	mdso_obj_write_short(dsometa->hdr.cfh_characteristics,oattr);

	/* .dsostrs section header */
	memcpy(dsometa->sec[0].sh_name,".dsostrs",8);
	mdso_obj_write_long(dsometa->sec[0].sh_size_of_raw_data,liblen+1);
	mdso_obj_write_long(dsometa->sec[0].sh_ptr_to_raw_data,stroff);
	mdso_obj_write_long(dsometa->sec[0].sh_characteristics,sattr);

	/* .dsometa section header */
	memcpy(dsometa->sec[1].sh_name,".dsometa",8);
	mdso_obj_write_long(dsometa->sec[1].sh_size_of_raw_data,reclen);
	mdso_obj_write_long(dsometa->sec[1].sh_ptr_to_raw_data,recoff);
	mdso_obj_write_long(dsometa->sec[1].sh_ptr_to_relocs,reloff);
	mdso_obj_write_short(dsometa->sec[1].sh_num_of_relocs,1);
	mdso_obj_write_long(dsometa->sec[1].sh_characteristics,rattr);

	/* .dsometa section data: flags */
	mark = dsometa->rec[0].data;
	mdso_obj_write_long(&mark[2*relrva],dctx->cctx->dsoflags);

	/* .dsometa relocation record: .name */
	mdso_obj_write_long(dsometa->rel[0].rel_sym,6);
	mdso_obj_write_long(dsometa->rel[0].rel_rva,relrva);
	mdso_obj_write_short(dsometa->rel[0].rel_type,reltype);

	/* coff string table */
	mdso_obj_write_long(dsometa->cst.cst_size,cstlen);

	/* coff symbol table */
	symrec = dsometa->sym;
	mark   = dsometa->cst.cst_data;

	/* coff symbol: .file */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_FILE;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(&symrec[0].cs_section_number[0],PE_IMAGE_SYM_DEBUG);
	mdso_obj_write_long(&symrec[1].cs_name[4],cstoff+datoff);

	memcpy(symrec[0].cs_name,".file",5);
	memcpy(&mark[0],".dsometa_",9);
	memcpy(&mark[9],dctx->cctx->libname,liblen);
	memcpy(&mark[9+liblen],".s",2);

	datoff += 12 + liblen;
	mark   += 12 + liblen;
	symrec += 2;

	/* coff symbol: .dsostrs */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(symrec[0].cs_section_number,1);
	memcpy(symrec[0].cs_name,".dsostrs",8);

	aux = (struct pe_raw_aux_rec_section *)&symrec[1];
	mdso_obj_write_long(aux->aux_size,liblen+1);
	mdso_obj_write_short(aux->aux_num_of_relocs,0);

	datoff += 0;
	mark   += 0;
	symrec += 2;

	/* coff symbol: .dsometa */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_symbols[0] = 1;

	mdso_obj_write_short(symrec[0].cs_section_number,2);
	memcpy(symrec[0].cs_name,".dsometa",8);

	aux = (struct pe_raw_aux_rec_section *)&symrec[1];
	mdso_obj_write_long(aux->aux_size,reclen);
	mdso_obj_write_short(aux->aux_num_of_relocs,1);

	datoff += 0;
	mark   += 0;
	symrec += 2;

	/* coff symbol: .libname */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_STATIC;
	symrec[0].cs_num_of_aux_symbols[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,1);
	memcpy(symrec[0].cs_name,".libname",8);

	datoff += 0;
	mark   += 0;
	symrec += 1;

	/* coff symbol: .dsometa_libname */
	symrec[0].cs_storage_class[0] = PE_IMAGE_SYM_CLASS_EXTERNAL;
	symrec[0].cs_num_of_aux_symbols[0] = 0;

	mdso_obj_write_short(symrec[0].cs_section_number,2);
	mdso_obj_write_long(&symrec[0].cs_name[4],cstoff+datoff);

	memcpy(&mark[0],".dsometa_",9);
	memcpy(&mark[9],dctx->cctx->libname,liblen);

	/* archive symbol map */
	if (vobj->mapstrs)
		memcpy(vobj->mapstrs,mark,9+liblen);

	/* .libname */
	mark = dsometa->hdr.cfh_machine;
	memcpy(&mark[stroff],dctx->cctx->libname,liblen);

	/* fs object unmap */
	if (!addr)
		munmap(vobj->addr,vobj->size);

	/* tada */
	return 0;
}
