/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	FILE *				fout,
	struct mdso_object *		vobj)
{
	int					ret;
	const char **				psym;
	int					nsym;
	int					nobj;
	uint32_t				objlen;
	uint32_t				hdrlen;
	uint32_t				mapstrsnum;
	uint32_t				mapstrslen;
	uint32_t				symidx;
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
	memset (sobj,0,sizeof(sobj));

	for (nsym=0,psym=symv; *psym; psym++)
		nsym++;

	if ((nobj = 2*nsym + 1) < 256)
		aobj = sobj;

	else if (nobj > 1024*1024)
		return MDSO_CUSTOM_ERROR(dctx,MDSO_ERR_INVALID_VECTOR);

	else if (!(aobj = calloc(1,nobj*sizeof(*aobj))))
		return MDSO_SYSTEM_ERROR(dctx);

	/* archive signature, archive header */
	objlen  = 8;
	objlen += sizeof(struct pe_raw_archive_common_hdr);

	/* archive meta */
	ret = mdso_objgen_dsometa(dctx,0,aobj);

	aobj->size += 1;
	aobj->size |= 1;
	aobj->size ^= 1;

	objlen += aobj->size;
	objlen += aobj->mapstrslen;
	objlen += sizeof(uint32_t) * aobj->mapstrsnum;
	objlen += sizeof(struct pe_raw_archive_common_hdr);

	mapstrslen = aobj->mapstrslen;
	mapstrsnum = aobj->mapstrsnum;

	for (psym=symv,pobj=&aobj[1]; *psym && !ret; psym++) {
		ret  = mdso_objgen_symfn(dctx,*psym,0,pobj);

		pobj->size += 1;
		pobj->size |= 1;
		pobj->size ^= 1;

		objlen += pobj->size;
		objlen += pobj->mapstrslen;
		objlen += sizeof(uint32_t) * pobj->mapstrsnum;
		objlen += sizeof(struct pe_raw_archive_common_hdr);

		mapstrslen += pobj->mapstrslen;
		mapstrsnum += pobj->mapstrsnum;
		pobj++;

		ret |= mdso_objgen_symentry(dctx,*psym,0,pobj);

		pobj->size += 1;
		pobj->size |= 1;
		pobj->size ^= 1;

		objlen += pobj->size;
		objlen += pobj->mapstrslen;
		objlen += sizeof(uint32_t) * pobj->mapstrsnum;
		objlen += sizeof(struct pe_raw_archive_common_hdr);

		mapstrslen += pobj->mapstrslen;
		mapstrsnum += pobj->mapstrsnum;
		pobj++;
	}

	if (ret && (aobj != sobj))
		free(aobj);

	if (ret)
		return ret;

	/* archive alignment */
	mapstrslen += 1;
	mapstrslen |= 1;
	mapstrslen ^= 1;

	objlen += 15;
	objlen |= 15;
	objlen ^= 15;

	if (vobj && vobj->addr && (vobj->size < objlen))
		return MDSO_BUFFER_ERROR(dctx);

	if (vobj && !vobj->addr) {
		vobj->size = objlen;
		vobj->mapstrslen = mapstrslen;
		vobj->mapstrsnum = mapstrsnum;
		return 0;
	}

	if (vobj)
		ar = (unsigned char *)vobj->addr;

	else if (!(ar = calloc(1,objlen))) {
		if (aobj != sobj)
			free(aobj);

		return MDSO_SYSTEM_ERROR(dctx);
	}

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

	ret = mdso_objgen_dsometa(dctx,0,aobj);

	mdso_argen_common_hdr(
		(struct pe_raw_archive_common_hdr *)mark,
		".dsometa.o",aobj->size);

	mark    += aobj->arhdrlen + aobj->size;
	mapstrs += aobj->mapstrslen;

	/* archive symfn and symentry objects */
	for (psym=symv,pobj=&aobj[1]; *psym && !ret; psym++) {
		/* symfn object */
		pobj->mapstrs  = mapstrs;
		pobj->arhdrpos = (uint32_t)(mark - ar);
		pobj->arhdrlen = sizeof(struct pe_raw_archive_common_hdr);
		pobj->addr     = &mark[pobj->arhdrlen];

		for (symidx=0; symidx<pobj->mapstrsnum; symidx++) {
			mdso_write_big_endian_long(idx,pobj->arhdrpos);
			idx += sizeof(uint32_t);
		}

		ret = mdso_objgen_symfn(dctx,*psym,0,pobj);

		sprintf(
			objname,"f%06zu.o",
			psym - symv);

		mdso_argen_common_hdr(
			(struct pe_raw_archive_common_hdr *)mark,
			objname,pobj->size);

		mark    += pobj->arhdrlen + pobj->size;
		mapstrs += pobj->mapstrslen;
		pobj++;


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

		ret = mdso_objgen_symentry(dctx,*psym,0,pobj);

		mdso_argen_common_hdr(
			(struct pe_raw_archive_common_hdr *)mark,
			objname,pobj->size);

		mark    += pobj->arhdrlen + pobj->size;
		mapstrs += pobj->mapstrslen;
		pobj++;
	}

	if (ret) {
		if (aobj != sobj)
			free(aobj);

		if (!vobj)
			free(ar);

		return ret;
	}

	/* tada */
	if (fout)
		ret = fwrite(ar,objlen,1,fout);

	if (aobj != sobj)
		free(aobj);

	if (!vobj)
		free(ar);

	return (ret == 0)
		? MDSO_FILE_ERROR(dctx)
		: 0;
}
