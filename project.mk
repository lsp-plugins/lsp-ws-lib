# Package version
ARTIFACT_NAME               = lsp-ws-lib
ARTIFACT_VARS               = LSP_WS_LIB
ARTIFACT_HEADERS            = lsp-plug.in
ARTIFACT_EXPORT_ALL         = 1
VERSION                     = 0.5.0

# List of dependencies
TEST_DEPENDENCIES = \
  LSP_TEST_FW

DEPENDENCIES = \
  STDLIB \
  LSP_COMMON_LIB \
  LSP_LLTL_LIB \
  LSP_R3D_BASE_LIB \
  LSP_RUNTIME_LIB

# For Linux-based systems, use libsndfile and xlib
ifeq ($(PLATFORM),Linux)
  DEPENDENCIES             += LIBSNDFILE XLIB CAIRO
endif

# For BSD-based systems, use libsndfile and xlib
ifeq ($(PLATFORM),BSD)
  DEPENDENCIES             += LIBSNDFILE XLIB CAIRO
endif
