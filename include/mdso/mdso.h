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
#error  libmdso: pre-alpha: ABI is not final!
#error  to use the library, please pass -DMDSO_PRE_ALPHA to the compiler.
#endif
#endif

/* status codes */
#define MDSO_OK				0x00
#define MDSO_USAGE			0x01
#define MDSO_BAD_OPT			0x02
#define MDSO_BAD_OPT_VAL		0x03
#define MDSO_IO_ERROR			0xA0
#define MDSO_MAP_ERROR			0xA1

/* driver flags */
#define MDSO_DRIVER_VERBOSITY_NONE	0x0000
#define MDSO_DRIVER_VERBOSITY_ERRORS	0x0001
#define MDSO_DRIVER_VERBOSITY_STATUS	0x0002
#define MDSO_DRIVER_VERBOSITY_USAGE	0x0004
#define MDSO_DRIVER_CLONE_VECTOR	0x0008

#define MDSO_DRIVER_VERSION		0x0010
#define MDSO_DRIVER_DRY_RUN		0x0020
#define MDSO_DRIVER_QUAD_PTR		0x0040

/* unit action flags */

struct mdso_input {
	void *	addr;
	size_t	size;
};

struct mdso_common_ctx {
	uint64_t			drvflags;
	uint64_t			actflags;
	uint64_t			fmtflags;
	uint32_t			dsoflags;
	const char *			libname;
	const char *			asmbase;
	const char *			dstdir;
};

struct mdso_driver_ctx {
	const char **			units;
	const char *			program;
	const char *			module;
	const struct mdso_common_ctx *	cctx;
	void *				any;
	int				status;
	int				nerrors;
};

struct mdso_unit_ctx {
	const char * const *		path;
	const struct mdso_input *	map;
	const struct mdso_common_ctx *	cctx;
	const char * const *		syms;
	void *				any;
	int				status;
	int				nerrors;
};

/* driver api */
mdso_api int  mdso_get_driver_ctx	(char ** argv, char ** envp, uint32_t flags, struct mdso_driver_ctx **);
mdso_api int  mdso_create_driver_ctx	(const struct mdso_common_ctx *, struct mdso_driver_ctx **);
mdso_api void mdso_free_driver_ctx	(struct mdso_driver_ctx *);

mdso_api int  mdso_get_unit_ctx		(const struct mdso_driver_ctx *, const char * path, struct mdso_unit_ctx **);
mdso_api void mdso_free_unit_ctx	(struct mdso_unit_ctx *);

mdso_api int  mdso_map_input		(int fd, const char * path, int prot, struct mdso_input *);
mdso_api int  mdso_unmap_input		(struct mdso_input *);

/* utility api */
mdso_api int  mdso_output_export_symbols(const struct mdso_unit_ctx *, const struct mdso_common_ctx *, FILE *);
mdso_api int  mdso_create_implib_sources(const struct mdso_driver_ctx *);

/* low-level api */
mdso_api uint32_t mdso_crc32_mbstr	(const unsigned char * str, size_t * symlen);
mdso_api uint64_t mdso_crc64_mbstr	(const unsigned char * str, size_t * symlen);
mdso_api FILE *   mdso_create_output	(const struct mdso_driver_ctx *, const char * asmname);
mdso_api int      mdso_generate_dsometa	(const struct mdso_common_ctx *, FILE * fout);
mdso_api int      mdso_generate_symentry(const struct mdso_common_ctx *, const char * sym, FILE * fout);
mdso_api int      mdso_generate_symfn	(const char * sym, FILE * fout);

#ifdef __cplusplus
}
#endif

#endif
