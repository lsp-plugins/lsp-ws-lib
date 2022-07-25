#
# Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
#
# This file is part of lsp-ws-lib
#
# lsp-ws-lib is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# lsp-ws-lib is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with lsp-ws-lib.  If not, see <https://www.gnu.org/licenses/>.
#

# List of dependencies
DEPENDENCIES = \
  LIBPTHREAD \
  LSP_COMMON_LIB \
  LSP_LLTL_LIB \
  LSP_R3D_IFACE \
  LSP_RUNTIME_LIB

TEST_DEPENDENCIES = \
  LSP_TEST_FW

#------------------------------------------------------------------------------
# Linux dependencies
LINUX_DEPENDENCIES = \
  LIBDL \
  LIBSNDFILE \
  LIBCAIRO \
  LIBFREETYPE \
  LIBX11 \
  LIBXRANDR

LINUX_TEST_DEPENDENCIES = \
  LSP_R3D_BASE_LIB \
  LSP_R3D_GLX_LIB \
  LIBGL

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES             += $(LINUX_DEPENDENCIES)
  TEST_DEPENDENCIES        += $(LINUX_TEST_DEPENDENCIES)
endif

#------------------------------------------------------------------------------
# BSD dependencies
BSD_DEPENDENCIES = \
  LIBDL \
  LIBSNDFILE \
  LIBICONV \
  LIBCAIRO \
  LIBFREETYPE \
  LIBX11 \
  LIBXRANDR

BSD_TEST_DEPENDENCIES = \
  LSP_R3D_BASE_LIB \
  LSP_R3D_GLX_LIB \
  LIBGL

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES             += $(BSD_DEPENDENCIES)
  TEST_DEPENDENCIES        += $(BSD_TEST_DEPENDENCIES)
endif

#------------------------------------------------------------------------------
# Windows dependencies
WINDOWS_DEPENDENCIES = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBD2D1 \
  LIBOLE \
  LIBWINCODEC \
  LIBDWRITE \
  LIBGDI32 \
  LIBUUID

WINDOWS_TEST_DEPENDENCIES = \
  LSP_R3D_BASE_LIB \
  LSP_R3D_WGL_LIB \
  LIBOPENGL32

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES             += $(WINDOWS_DEPENDENCIES)
  TEST_DEPENDENCIES        += $(WINDOWS_TEST_DEPENDENCIES)
endif

ALL_DEPENDENCIES = \
  $(DEPENDENCIES) \
  $(LINUX_DEPENDENCIES) \
  $(BSD_DEPENDENCIES) \
  $(WINDOWS_DEPENDENCIES) \
  $(TEST_DEPENDENCIES) \
  $(TEST_LINUX_DEPENDENCIES) \
  $(TEST_WINDOWS_DEPENDENCIES) \
  $(TEST_BSD_DEPENDENCIES)
