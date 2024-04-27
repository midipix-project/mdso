API_HEADERS = \
	$(PROJECT_DIR)/include/$(PACKAGE)/mdso.h \
	$(PROJECT_DIR)/include/$(PACKAGE)/mdso_api.h \
	$(PROJECT_DIR)/include/$(PACKAGE)/mdso_crc32.h \
	$(PROJECT_DIR)/include/$(PACKAGE)/mdso_crc64.h \
	$(PROJECT_DIR)/include/$(PACKAGE)/mdso_output.h \
	$(PROJECT_DIR)/include/$(PACKAGE)/mdso_specs.h \
	$(PROJECT_DIR)/include/$(PACKAGE)/mdso_structs.h \

INTERNAL_HEADERS = \
	$(PROJECT_DIR)/src/internal/argv/argv.h \
	$(PROJECT_DIR)/src/internal/perk/perk_consts.h \
	$(PROJECT_DIR)/src/internal/perk/perk_structs.h \
	$(PROJECT_DIR)/src/internal/$(PACKAGE)_dprintf_impl.h \
	$(PROJECT_DIR)/src/internal/$(PACKAGE)_driver_impl.h \
	$(PROJECT_DIR)/src/internal/$(PACKAGE)_errinfo_impl.h \
	$(PROJECT_DIR)/src/internal/$(PACKAGE)_hexfmt_impl.h \
	$(PROJECT_DIR)/src/internal/$(PACKAGE)_object_impl.h \

ALL_HEADERS = $(API_HEADERS) $(INTERNAL_HEADERS)
