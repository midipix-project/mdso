API_SRCS = \
	src/archive/mdso_argen_common.c \
	src/crc/mdso_crc64.c \
	src/crc/mdso_crc32.c \
	src/driver/mdso_amain.c \
	src/driver/mdso_driver_ctx.c \
	src/driver/mdso_unit_ctx.c \
	src/helper/mdso_create_output.c \
	src/helper/mdso_map_input.c \
	src/logic/mdso_asmgen_dsometa.c \
	src/logic/mdso_asmgen_symentry.c \
	src/logic/mdso_asmgen_symfn.c \
	src/object/mdso_objgen_dsometa.c \
	src/object/mdso_objgen_symentry.c \
	src/object/mdso_objgen_symfn.c \
	src/output/mdso_output_error.c \
	src/output/mdso_output_export_symbols.c \
	src/skin/mdso_skin_default.c \
	src/util/mdso_create_implib_objects.c \
	src/util/mdso_create_implib_sources.c \

INTERNAL_SRCS = \
	src/internal/$(PACKAGE)_errinfo_impl.c \

APP_SRCS = \
	src/mdso.c

COMMON_SRCS = $(API_SRCS) $(INTERNAL_SRCS)
