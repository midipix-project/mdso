/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2024  SysDeer Technologies, LLC         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <errno.h>
#include <mdso/mdso.h>

int mdso_record_error(
	const struct mdso_driver_ctx *,
	int		esyscode,
	int		elibcode,
	const char *	efunction,
	int		eline,
	unsigned	eflags,
	void *		eany);

#define MDSO_SYSTEM_ERROR(dctx)           \
	mdso_record_error(                \
		dctx,                     \
		errno,                    \
		0,                        \
		__func__,                 \
		__LINE__,                 \
		MDSO_ERROR_TOP_LEVEL,     \
		0)

#define MDSO_BUFFER_ERROR(dctx)           \
	mdso_record_error(                \
		dctx,                     \
		ENOBUFS,                  \
		0,                        \
		__func__,                 \
		__LINE__,                 \
		MDSO_ERROR_TOP_LEVEL,     \
		0)

#define MDSO_SPAWN_ERROR(dctx)            \
	mdso_record_error(                \
		dctx,                     \
		errno,                    \
		0,                        \
		__func__,                 \
		__LINE__,                 \
		MDSO_ERROR_TOP_LEVEL      \
		| (errno ? 0              \
		   : MDSO_ERROR_CHILD),   \
		0)

#define MDSO_FILE_ERROR(dctx)             \
	mdso_record_error(                \
		dctx,                     \
		EIO,                      \
		0,                        \
		__func__,                 \
		__LINE__,                 \
		MDSO_ERROR_TOP_LEVEL,     \
		0)

#define MDSO_CUSTOM_ERROR(dctx,elibcode)  \
	mdso_record_error(                \
		dctx,                     \
		0,                        \
		elibcode,                 \
		__func__,                 \
		__LINE__,                 \
		MDSO_ERROR_TOP_LEVEL      \
		| MDSO_ERROR_CUSTOM,      \
		0)

#define MDSO_NESTED_ERROR(dctx)           \
	mdso_record_error(                \
		dctx,                     \
		0,                        \
		0,                        \
		__func__,                 \
		__LINE__,                 \
		MDSO_ERROR_NESTED,        \
		0)
