/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <mdso/mdso.h>

static const char aclr_reset[]   = "\x1b[0m";
static const char aclr_bold[]    = "\x1b[1m";

static const char aclr_green[]   = "\x1b[32m";
static const char aclr_blue[]    = "\x1b[34m";
static const char aclr_magenta[] = "\x1b[35m";

static const char * mdso_output_error_header(const struct mdso_error_info * erri)
{
	if (erri->flags & MDSO_ERROR_CHILD)
		return "exec error upon";

	else if (erri->flags & MDSO_ERROR_TOP_LEVEL)
		return "error logged in";

	else if (erri->flags & MDSO_ERROR_NESTED)
		return "< returned to >";

	else
		return "distorted state";
}

static const char * mdso_output_strerror(const struct mdso_error_info * erri)
{
	if (erri->flags & MDSO_ERROR_CUSTOM)
		return "flow error: unexpected condition or other";

	else if (erri->flags & MDSO_ERROR_NESTED)
		return "";

	else if (erri->flags & MDSO_ERROR_CHILD)
		return "(see child process error messages)";

	else if (erri->syserror == ENOBUFS)
		return "input error: string length exceeds buffer size.";

	else
		return strerror(erri->syserror);
}

static int mdso_output_error_record_plain(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_error_info *	erri)
{
	const char * errdesc = mdso_output_strerror(erri);

	if (fprintf(stderr,"%s: %s %s(), line %d%s%s.\n",
			dctx->program,
			mdso_output_error_header(erri),
			erri->function,
			erri->line,
			strlen(errdesc) ? ": " : "",
			errdesc) < 0)
		return -1;

	return fflush(stderr);
}

static int mdso_output_error_record_annotated(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_error_info *	erri)
{
	const char * errdesc = mdso_output_strerror(erri);

	if (fprintf(
			stderr,
			"%s%s%s:%s %s%s%s %s%s%s()%s, %s%sline %d%s%s%s%s%s.\n",

			aclr_bold,aclr_magenta,
			dctx->program,
			aclr_reset,

			aclr_bold,
			mdso_output_error_header(erri),
			aclr_reset,

			aclr_bold,aclr_blue,
			erri->function,
			aclr_reset,

			aclr_bold,aclr_green,
			erri->line,
			aclr_reset,
			strlen(errdesc) ? ": " : "",

			aclr_bold,
			mdso_output_strerror(erri),
			aclr_reset) < 0)
		return -1;

	return fflush(stderr);
}

int mdso_output_error_record(
	const struct mdso_driver_ctx *	dctx,
	const struct mdso_error_info *	erri)
{
	if (dctx->cctx->drvflags & MDSO_DRIVER_ANNOTATE_NEVER)
		return mdso_output_error_record_plain(dctx,erri);

	else if (dctx->cctx->drvflags & MDSO_DRIVER_ANNOTATE_ALWAYS)
		return mdso_output_error_record_annotated(dctx,erri);

	else if (isatty(STDERR_FILENO))
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

	else if (isatty(STDERR_FILENO))
		return mdso_output_error_vector_annotated(dctx);

	else
		return mdso_output_error_vector_plain(dctx);
}
