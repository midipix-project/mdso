/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <mdso/mdso.h>
#include "mdso_errinfo_impl.h"
#include "perk_structs.h"

static void mdso_argen_common_hdr(
	struct pe_raw_archive_common_hdr *	arhdr,
	char *					file_id,
	size_t					size)
{
	size_t	slen;
	char	sbuf[10];

	memset(arhdr,0x20,sizeof(*arhdr));

	slen = strlen(file_id);
	memcpy(arhdr->ar_file_id,file_id,slen);
	arhdr->ar_file_id[slen] = '/';

	arhdr->ar_uid[0] = '0';
	arhdr->ar_gid[0] = '0';
	arhdr->ar_file_mode[0] = '0';
	arhdr->ar_time_date_stamp[0] = '0';

	slen = sprintf(sbuf,"%zu",size);
	memcpy(arhdr->ar_file_size,sbuf,slen);

	arhdr->ar_end_tag[0] = 0x60;
	arhdr->ar_end_tag[1] = 0x0a;
}

static void mdso_write_big_endian_long(unsigned char * ch, uint32_t val)
{
	ch[3] = val;
	ch[2] = val >> 8;
	ch[1] = val >> 16;
	ch[0] = val >> 24;
}

int  mdso_argen_common(
	const struct mdso_driver_ctx *	dctx,
	const char **			symv,
	struct mdso_object *		vobj)
{
	int					ret;
	const char **				psym;
	intptr_t				nobj;
	uint32_t				objlen;
	uint32_t				hdrlen;
	uint32_t				mapstrsnum;
	uint32_t				mapstrslen;
	uint32_t				symidx;
	void *					addr;
	unsigned char *				ar;
	unsigned char *				mark;
	unsigned char *				idx;
	char *					mapstrs;
	struct pe_raw_archive_common_hdr *	arhdr;
	struct mdso_object *			pobj;
	struct mdso_object *			aobj;
	struct mdso_object			sobj[256];
	char					objname[16];

	/* init */
	memset(sobj,0,sizeof(sobj));

	for (psym=symv; *psym; psym++)
		(void)0;

	if ((nobj = psym - symv) < 255)
		aobj = sobj;

	else if (nobj > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_VECTOR);

	else if (!(aobj = calloc(1,++nobj*sizeof(*aobj))))
		return MDSO_SYSTEM_ERROR(dctx);

	/* objlen: archive signature, index header */
	objlen  = 8;
	objlen += sizeof(struct pe_raw_archive_common_hdr);

	/* objlen: member headers */
	ret = mdso_objgen_dsometa(dctx,aobj);

	aobj->size += 1;
	aobj->size |= 1;
	aobj->size ^= 1;

	objlen += aobj->size;
	objlen += sizeof(struct pe_raw_archive_common_hdr);

	mapstrslen = aobj->mapstrslen;
	mapstrsnum = aobj->mapstrsnum;

	/* objlen: symentry */
	for (psym=symv,pobj=&aobj[1]; *psym && !ret; psym++) {
		ret = mdso_objgen_symentry(dctx,*psym,pobj);

		pobj->size += 1;
		pobj->size |= 1;
		pobj->size ^= 1;

		objlen += pobj->size;
		objlen += sizeof(struct pe_raw_archive_common_hdr);

		mapstrslen += pobj->mapstrslen;
		mapstrsnum += pobj->mapstrsnum;
		pobj++;
	}

	/* verify logic */
	if (ret && (aobj == sobj))
		return MDSO_NESTED_ERROR(dctx);

	if (ret) {
		free(aobj);
		return MDSO_NESTED_ERROR(dctx);
	}

	/* index: string block alignment */
	mapstrslen += 1;
	mapstrslen |= 1;
	mapstrslen ^= 1;

	/* objlen: index size, padding */
	objlen += sizeof(uint32_t) * (1 + mapstrsnum);
	objlen += mapstrslen;

	objlen += 15;
	objlen |= 15;
	objlen ^= 15;

	/* archive meta info, in-memory mapping */
	if (vobj->addr && (vobj->size < objlen))
		return MDSO_BUFFER_ERROR(dctx);

	if ((addr = vobj->addr)) {
		(void)0;

	} else {
		vobj->size       = objlen;
		vobj->mapstrslen = mapstrslen;
		vobj->mapstrsnum = mapstrsnum;

		if (!vobj->name)
			return 0;

		else if (mdso_create_archive(dctx,vobj) < 0)
			return MDSO_NESTED_ERROR(dctx);
	}

	ar = (unsigned char *)vobj->addr;

	/* archive signature */
	memcpy(ar,"!<arch>\n",8);
	mark = &ar[8];

	/* archive header */
	arhdr  = (struct pe_raw_archive_common_hdr *)mark;
	hdrlen = sizeof(uint32_t) * (1 + mapstrsnum) + mapstrslen;
	mdso_argen_common_hdr(arhdr,"",hdrlen);
	mark += sizeof(*arhdr);

	/* archive index initialization */
	mdso_write_big_endian_long(mark,mapstrsnum);
	mark += sizeof(uint32_t);

	idx   = mark;
	mark += sizeof(uint32_t) * mapstrsnum;

	mapstrs = (char *)mark;
	mark   += mapstrslen;

	/* .dsometa object */
	aobj->mapstrs  = mapstrs;
	aobj->arhdrpos = (uint32_t)(mark - ar);
	aobj->arhdrlen = sizeof(struct pe_raw_archive_common_hdr);
	aobj->addr     = &mark[aobj->arhdrlen];

	for (symidx=0; symidx<aobj->mapstrsnum; symidx++) {
		mdso_write_big_endian_long(idx,aobj->arhdrpos);
		idx += sizeof(uint32_t);
	}

	ret = mdso_objgen_dsometa(dctx,aobj);

	mdso_argen_common_hdr(
		(struct pe_raw_archive_common_hdr *)mark,
		".dsometa.o",aobj->size);

	mark    += aobj->arhdrlen + aobj->size;
	mapstrs += aobj->mapstrslen;

	/* archive symentry objects */
	for (psym=symv,pobj=&aobj[1]; *psym && !ret; psym++) {
		/* symentry object */
		pobj->mapstrs  = mapstrs;
		pobj->arhdrpos = (uint32_t)(mark - ar);
		pobj->arhdrlen = sizeof(struct pe_raw_archive_common_hdr);
		pobj->addr     = &mark[pobj->arhdrlen];

		for (symidx=0; symidx<pobj->mapstrsnum; symidx++) {
			mdso_write_big_endian_long(idx,pobj->arhdrpos);
			idx += sizeof(uint32_t);
		}

		sprintf(
			objname,"s%06zu.o",
			psym - symv);

		ret = mdso_objgen_symentry(dctx,*psym,pobj);

		mdso_argen_common_hdr(
			(struct pe_raw_archive_common_hdr *)mark,
			objname,pobj->size);

		mark    += pobj->arhdrlen + pobj->size;
		mapstrs += pobj->mapstrslen;
		pobj++;
	}

	/* aobj */
	if (aobj != sobj)
		free(aobj);

	/* fs object unmap */
	if (!addr)
		munmap(vobj->addr,vobj->size);

	/* verify */
	if (ret)
		return MDSO_NESTED_ERROR(dctx);

	/* tada */
	return 0;

}
