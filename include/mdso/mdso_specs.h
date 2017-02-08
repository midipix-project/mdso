#ifndef MDSO_SPECS_H
#define MDSO_SPECS_H

#include <stdint.h>

#define MDSO_META_SECTION	".dsometa"
#define MDSO_SYMS_SECTION	".dsosyms"
#define MDSO_STRS_SECTION	".dsostrs"
#define MDSO_HASH_SECTION	".dsohash"

#define MDSO_FLAG_LOADER_PATH	0x0001
#define MDSO_FLAG_PEB_PATH	0x0002
#define MDSO_FLAG_SYSTEM32	0x0004

struct mdso_arg;
struct mdso_meta_table;
struct mdso_hash_table;
struct mdso_sym_table;

typedef int32_t mdso_arg_conv_fn(char *, uint16_t **);
typedef int32_t mdso_arg_arr_conv_fn(struct mdso_arg *);

struct mdso_arg {
	char *		utf8;
	uint16_t *	utf16;
};

struct mdso_sym_entry {
	char *				string;
	struct mdso_meta_table *	meta;
};

struct mdso_meta_record {
	void *				base;
	const char *			name;
	uint32_t			flags;
	uint32_t			priority;
	uint32_t			nsyms;
	uint32_t			padding;
	struct mdso_hash_table *	hashtbl;
	struct mdso_sym_table *		symtbl;
	mdso_arg_conv_fn *		fncarg;
	mdso_arg_arr_conv_fn *		fncargarr;
	void *				fnr1;
	void *				fnr2;
};


struct mdso_meta_record_m32 {
	uint32_t			base;
	uint32_t			name;
	uint32_t			flags;
	uint32_t			priority;
	uint32_t			nsyms;
	uint32_t			padding;
	uint32_t			hashtbl;
	uint32_t			symtbl;
	uint32_t			fncarg;
	uint32_t			fncargarr;
	uint32_t			fnr1;
	uint32_t			fnr2;
};

struct mdso_meta_record_m64 {
	uint64_t			base;
	uint64_t			name;
	uint32_t			flags;
	uint32_t			priority;
	uint32_t			nsyms;
	uint32_t			padding;
	uint64_t			hashtbl;
	uint64_t			symtbl;
	uint64_t			fncarg;
	uint64_t			fncargarr;
	uint64_t			fnr1;
	uint64_t			fnr2;
};

#endif
