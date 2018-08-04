/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <mdso/mdso.h>

#include "mdso_driver_impl.h"
#include "mdso_dprintf_impl.h"

static const char aclr_reset[]   = "\x1b[0m";
static const char aclr_bold[]    = "\x1b[1m";

static const char aclr_red[]     = "\x1b[31m";
static const char aclr_green[]   = "\x1b[32m";
static const char aclr_blue[]    = "\x1b[34m";
static const char aclr_magenta[] = "\x1b[35m";

static char const * const mdso_error_strings[MDSO_ERR_CAP] = {
	[MDSO_ERR_FLOW_ERROR]       = "flow error: unexpected condition or other",
	[MDSO_ERR_FLEE_ERROR]       = "flees and bugs and cats and mice",
	[MDSO_ERR_NULL_CONTEXT]     = "null driver or unit context",
	[MDSO_ERR_NULL_SOURCE]      = "source file does not define any symbols",
	[MDSO_ERR_INVALID_NAME]     = "invalid or missing import library archive name",
	[MDSO_ERR_INVALID_DATA]     = "invalid data (symbol name too long)",
	[MDSO_ERR_INVALID_DSTDIR]   = "invalid destination directory",
	[MDSO_ERR_INVALID_CONTEXT]  = "invalid driver or unit context",
	[MDSO_ERR_INVALID_SOURCE]   = "invalid symbol definition source file",
	[MDSO_ERR_INVALID_VECTOR]   = "invalid symbol vector, or vector too long",
	[MDSO_ERR_SOURCE_SIZE_ZERO] = "cannot map an empty symbol definition source file",
};

static const char * mdso_output_error_header(const struct mdso_error_info * erri)
{
	if (erri->eflags & MDSO_ERROR_CHILD)
		return "exec error upon";

	else if (erri->eflags & MDSO_ERROR_TOP_LEVEL)
		return "error logged in";

	else if (erri->eflags & MDSO_ERROR_NESTED)
		return "< returned to >";

	else
		return "distorted state";
}

static const char * mdso_output_unit_header(const struct mdso_error_info * erri)
{
	if (!(erri->eflags & MDSO_ERROR_CUSTOM))
		return "while opening";

	else if (erri->elibcode == MDSO_ERR_SOURCE_SIZE_ZERO)
		return "while mapping";

	else
		return "while parsing";
}

static const char * mdso_output_strerror(const struct mdso_error_info * erri)
{
	if (erri->eflags & MDSO_ERROR_CUSTOM)
		return ((erri->elibcode < 0) || (erri->elibcode >= MDSO_ERR_CAP))
			? "internal error: please report to the maintainer"
			: mdso_error_strings[erri->elibcode];

	else if (erri->eflags & MDSO_ERROR_NESTED)
		return "";

	else if (erri->eflags & MDSO_ERROR_CHILD)
		return "(see child process error messages)";

	else if (erri->esyscode == ENOBUFS)
		return "input error: string length exceeds buffer size.";

	else
		return strerror(erri->esyscode);
}

static int mdso_output_error_record_plain(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_error_info *	erri)
{
	const char * epath;
	const char * errdesc = mdso_output_strerror(erri);
	int          fderr   = mdso_driver_fderr(dctx);

	epath = erri->euctx
		? *erri->euctx->path
		: erri->eunit;

	if (epath && !(erri->eflags & MDSO_ERROR_NESTED))
		if (mdso_dprintf(
				fderr,
				"%s: [%s] '%s':\n",
				dctx->program,
				mdso_output_unit_header(erri),
				epath) < 0)
			return -1;

	if (mdso_dprintf(
			fderr,
			"%s: %s %s(), line %d%s%s.\n",
			dctx->program,
			mdso_output_error_header(erri),
			erri->efunction,
			erri->eline,
			strlen(errdesc) ? ": " : "",
			errdesc) < 0)
		return -1;

	return 0;
}

static int mdso_output_error_record_annotated(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_error_info *	erri)
{
	const char * epath;
	const char * errdesc = mdso_output_strerror(erri);
	int          fderr   = mdso_driver_fderr(dctx);

	epath = erri->euctx
		? *erri->euctx->path
		: erri->eunit;

	if (epath && !(erri->eflags & MDSO_ERROR_NESTED))
		if (mdso_dprintf(
				fderr,
				"%s%s%s:%s %s[%s]%s %s%s'%s'%s:\n",

				aclr_bold,aclr_magenta,
				dctx->program,
				aclr_reset,

				aclr_bold,
				mdso_output_unit_header(erri),
				aclr_reset,

				aclr_bold,aclr_red,
				epath,
				aclr_reset) < 0)
			return -1;

	if (mdso_dprintf(
			fderr,
			"%s%s%s:%s %s%s%s %s%s%s()%s, %s%sline %d%s%s%s%s%s.\n",

			aclr_bold,aclr_magenta,
			dctx->program,
			aclr_reset,

			aclr_bold,
			mdso_output_error_header(erri),
			aclr_reset,

			aclr_bold,aclr_blue,
			erri->efunction,
			aclr_reset,

			aclr_bold,aclr_green,
			erri->eline,
			aclr_reset,
			strlen(errdesc) ? ": " : "",

			aclr_bold,
			mdso_output_strerror(erri),
			aclr_reset) < 0)
		return -1;

	return 0;
}

int mdso_output_error_record(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_error_info *	erri)
{
	if (dctx->cctx->drvflags & MDSO_DRIVER_ANNOTATE_NEVER)
		return mdso_output_error_record_plain(dctx,erri);

	else if (dctx->cctx->drvflags & MDSO_DRIVER_ANNOTATE_ALWAYS)
		return mdso_output_error_record_annotated(dctx,erri);

	else if (isatty(mdso_driver_fderr(dctx)))
		return mdso_output_error_record_annotated(dctx,erri);

	else
		return mdso_output_error_record_plain(dctx,erri);
}

static int mdso_output_error_vector_plain(const struct mdso_driver_ctx * dctx)
{
	struct mdso_error_info ** perr;

	for (perr=dctx->errv; *perr; perr++)
		if (mdso_output_error_record_plain(dctx,*perr))
			return -1;

	return 0;
}

static int mdso_output_error_vector_annotated(const struct mdso_driver_ctx * dctx)
{
	struct mdso_error_info ** perr;

	for (perr=dctx->errv; *perr; perr++)
		if (mdso_output_error_record_annotated(dctx,*perr))
			return -1;

	return 0;
}

int mdso_output_error_vector(const struct mdso_driver_ctx * dctx)
{
	if (dctx->cctx->drvflags & MDSO_DRIVER_ANNOTATE_NEVER)
		return mdso_output_error_vector_plain(dctx);

	else if (dctx->cctx->drvflags & MDSO_DRIVER_ANNOTATE_ALWAYS)
		return mdso_output_error_vector_annotated(dctx);

	else if (isatty(mdso_driver_fderr(dctx)))
		return mdso_output_error_vector_annotated(dctx);

	else
		return mdso_output_error_vector_plain(dctx);
}
