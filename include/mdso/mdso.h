#ifndef MDSO_H
#define MDSO_H

#include <stdint.h>
#include <stdio.h>

#include "mdso_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pre-alpha */
#ifndef MDSO_APP
#ifndef MDSO_PRE_ALPHA
#warning libmdso: pre-alpha: ABI is not final!
#warning pass -DMDSO_PRE_ALPHA to suppress this warning.
#endif
#endif

/* status codes */
#define MDSO_OK				0x00
#define MDSO_USAGE			0x01
#define MDSO_ERROR			0x02

/* driver flags */
#define MDSO_DRIVER_VERBOSITY_NONE	0x0000
#define MDSO_DRIVER_VERBOSITY_ERRORS	0x0001
#define MDSO_DRIVER_VERBOSITY_STATUS	0x0002
#define MDSO_DRIVER_VERBOSITY_USAGE	0x0004
#define MDSO_DRIVER_CLONE_VECTOR	0x0008

#define MDSO_DRIVER_VERSION		0x0010
#define MDSO_DRIVER_DRY_RUN		0x0020
#define MDSO_DRIVER_QUAD_PTR		0x0040
#define MDSO_DRIVER_GENERATE_ASM	0x0100
#define MDSO_DRIVER_GENERATE_OBJECTS	0x0200
#define MDSO_DRIVER_COMPUTE_CRC32	0x0400
#define MDSO_DRIVER_COMPUTE_CRC64	0x0800

#define MDSO_DRIVER_ANNOTATE_ALWAYS	0x1000
#define MDSO_DRIVER_ANNOTATE_NEVER	0x2000
#define MDSO_DRIVER_ANNOTATE_FULL	0x4000

/* error flags */
#define MDSO_ERROR_TOP_LEVEL		0x0001
#define MDSO_ERROR_NESTED		0x0002
#define MDSO_ERROR_CHILD		0x0004
#define MDSO_ERROR_CUSTOM		0x0008

enum mdso_custom_error {
	MDSO_ERR_FLOW_ERROR,
	MDSO_ERR_FLEE_ERROR,
	MDSO_ERR_NULL_CONTEXT,
	MDSO_ERR_NULL_SOURCE,
	MDSO_ERR_INVALID_NAME,
	MDSO_ERR_INVALID_DATA,
	MDSO_ERR_INVALID_DSTDIR,
	MDSO_ERR_INVALID_CONTEXT,
	MDSO_ERR_INVALID_SOURCE,
	MDSO_ERR_INVALID_VECTOR,
	MDSO_ERR_SOURCE_SIZE_ZERO,
	MDSO_ERR_CAP,
};

struct mdso_input {
	void *		addr;
	size_t		size;
};

struct mdso_source_version {
	int		major;
	int		minor;
	int		revision;
	const char *	commit;
};

struct mdso_fd_ctx {
	int		fdin;
	int		fdout;
	int		fderr;
	int		fdlog;
	int		fdcwd;
	int		fddst;
};

struct mdso_object {
	const char *	name;
	void *		addr;
	size_t		size;
	char *		mapstrs;
	uint32_t	mapstrsnum;
	uint32_t	mapstrslen;
	uint32_t	arhdrpos;
	uint32_t	arhdrlen;
};

struct mdso_error_info {
	const struct mdso_driver_ctx *	edctx;
	const struct mdso_unit_ctx *	euctx;
	const char *			eunit;
	int				esyscode;
	int				elibcode;
	const char *			efunction;
	int				eline;
	unsigned			eflags;
	void *				eany;
};

struct mdso_common_ctx {
	uint64_t			drvflags;
	uint64_t			actflags;
	uint64_t			fmtflags;
	uint32_t			dsoflags;
	const char *			implib;
	const char *			libname;
	const char *			asmbase;
	const char *			dstdir;
};

struct mdso_driver_ctx {
	const char **			units;
	const char *			program;
	const char *			module;
	const struct mdso_common_ctx *	cctx;
	struct mdso_error_info **	errv;
	void *				any;
};

struct mdso_unit_ctx {
	const char * const *		path;
	const struct mdso_input *	map;
	const struct mdso_common_ctx *	cctx;
	const char * const *		syms;
	void *				any;
};

/* driver api */
mdso_api int  mdso_get_driver_ctx       (char **, char **, uint32_t,
                                         const struct mdso_fd_ctx *,
                                         struct mdso_driver_ctx **);

mdso_api void mdso_free_driver_ctx      (struct mdso_driver_ctx *);

mdso_api int  mdso_get_unit_ctx         (const struct mdso_driver_ctx *, const char *,
                                         struct mdso_unit_ctx **);

mdso_api void mdso_free_unit_ctx        (struct mdso_unit_ctx *);

mdso_api int  mdso_get_driver_fdctx     (const struct mdso_driver_ctx *, struct mdso_fd_ctx *);
mdso_api int  mdso_set_driver_fdctx     (struct mdso_driver_ctx *, const struct mdso_fd_ctx *);

/* helper api */
mdso_api int  mdso_create_asmsrc        (const struct mdso_driver_ctx *, const char *);
mdso_api int  mdso_create_object        (const struct mdso_driver_ctx *, struct mdso_object *);
mdso_api int  mdso_create_archive       (const struct mdso_driver_ctx *, struct mdso_object *);

/* utility api */
mdso_api int  mdso_main                 (char **, char **, const struct mdso_fd_ctx *);
mdso_api int  mdso_create_implib_archive(const struct mdso_driver_ctx *);
mdso_api int  mdso_create_implib_sources(const struct mdso_driver_ctx *);
mdso_api int  mdso_create_implib_objects(const struct mdso_driver_ctx *);
mdso_api int  mdso_output_export_symbols(const struct mdso_driver_ctx *, const struct mdso_unit_ctx *);
mdso_api int  mdso_output_expsyms_crc32 (const struct mdso_driver_ctx *, const struct mdso_unit_ctx *);
mdso_api int  mdso_output_expsyms_crc64 (const struct mdso_driver_ctx *, const struct mdso_unit_ctx *);
mdso_api int  mdso_output_error_record  (const struct mdso_driver_ctx *, const struct mdso_error_info *);
mdso_api int  mdso_output_error_vector  (const struct mdso_driver_ctx *);

/* raw input api */
mdso_api int  mdso_map_input            (const struct mdso_driver_ctx *,
                                         int, const char *, int,
                                         struct mdso_input *);

mdso_api int  mdso_unmap_input          (struct mdso_input *);

/* low-level api */
mdso_api uint32_t mdso_crc32_mbstr      (const unsigned char *, size_t *);
mdso_api uint64_t mdso_crc64_mbstr      (const unsigned char *, size_t *);

mdso_api int      mdso_asmgen_dsometa   (const struct mdso_driver_ctx *, int);
mdso_api int      mdso_asmgen_symentry  (const struct mdso_driver_ctx *, const char *, int);

mdso_api int      mdso_objgen_dsometa   (const struct mdso_driver_ctx *, struct mdso_object *);
mdso_api int      mdso_objgen_symentry  (const struct mdso_driver_ctx *, const char *, struct mdso_object *);

mdso_api int      mdso_argen_common     (const struct mdso_driver_ctx *,
                                         const char **,
                                         struct mdso_object *);

/* package info */
mdso_api const struct mdso_source_version * mdso_source_version(void);


#ifdef __cplusplus
}
#endif

#endif
