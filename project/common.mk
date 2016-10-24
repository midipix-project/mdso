COMMON_SRCS = \
	src/crc/mdso_crc64.c \
	src/crc/mdso_crc32.c \
	src/driver/mdso_amain.c \
	src/driver/mdso_driver_ctx.c \
	src/driver/mdso_unit_ctx.c \
	src/internal/mdso_errinfo_impl.c \
	src/logic/mdso_create_implib_sources.c \
	src/logic/mdso_generate_dsometa.c \
	src/logic/mdso_generate_symentry.c \
	src/logic/mdso_generate_symfn.c \
	src/logic/mdso_map_input.c \
	src/output/mdso_create_output.c \
	src/output/mdso_output_error.c \
	src/output/mdso_output_export_symbols.c \
	src/skin/mdso_skin_default.c \

APP_SRCS = \
	src/mdso.c
