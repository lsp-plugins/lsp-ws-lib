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

#------------------------------------------------------------------------------
# Variables that describe source code dependencies
LSP_COMMON_LIB_VERSION     := 1.0.36
LSP_COMMON_LIB_NAME        := lsp-common-lib
LSP_COMMON_LIB_TYPE        := src
LSP_COMMON_LIB_URL_RO      := https://github.com/lsp-plugins/$(LSP_COMMON_LIB_NAME).git
LSP_COMMON_LIB_URL_RW      := git@github.com:lsp-plugins/$(LSP_COMMON_LIB_NAME).git

LSP_DSP_LIB_VERSION        := 1.0.24
LSP_DSP_LIB_NAME           := lsp-dsp-lib
LSP_DSP_LIB_TYPE           := src
LSP_DSP_LIB_URL_RO         := https://github.com/lsp-plugins/$(LSP_DSP_LIB_NAME).git
LSP_DSP_LIB_URL_RW         := git@github.com:lsp-plugins/$(LSP_DSP_LIB_NAME).git

LSP_LLTL_LIB_VERSION       := 1.0.19
LSP_LLTL_LIB_NAME          := lsp-lltl-lib
LSP_LLTL_LIB_TYPE          := src
LSP_LLTL_LIB_URL_RO        := https://github.com/lsp-plugins/$(LSP_LLTL_LIB_NAME).git
LSP_LLTL_LIB_URL_RW        := git@github.com:lsp-plugins/$(LSP_LLTL_LIB_NAME).git

LSP_R3D_BASE_LIB_VERSION   := 1.0.18
LSP_R3D_BASE_LIB_NAME      := lsp-r3d-base-lib
LSP_R3D_BASE_LIB_TYPE      := src
LSP_R3D_BASE_LIB_URL_RO    := https://github.com/lsp-plugins/$(LSP_R3D_BASE_LIB_NAME).git
LSP_R3D_BASE_LIB_URL_RW    := git@github.com:lsp-plugins/$(LSP_R3D_BASE_LIB_NAME).git

LSP_R3D_GLX_LIB_VERSION    := 1.0.18
LSP_R3D_GLX_LIB_NAME       := lsp-r3d-glx-lib
LSP_R3D_GLX_LIB_TYPE       := src
LSP_R3D_GLX_LIB_URL_RO     := https://github.com/lsp-plugins/$(LSP_R3D_GLX_LIB_NAME).git
LSP_R3D_GLX_LIB_URL_RW     := git@github.com:lsp-plugins/$(LSP_R3D_GLX_LIB_NAME).git

LSP_R3D_IFACE_VERSION      := 1.0.18
LSP_R3D_IFACE_NAME         := lsp-r3d-iface
LSP_R3D_IFACE_TYPE         := src
LSP_R3D_IFACE_URL_RO       := https://github.com/lsp-plugins/$(LSP_R3D_IFACE_NAME).git
LSP_R3D_IFACE_URL_RW       := git@github.com:lsp-plugins/$(LSP_R3D_IFACE_NAME).git

LSP_R3D_WGL_LIB_VERSION    := 1.0.13
LSP_R3D_WGL_LIB_NAME       := lsp-r3d-wgl-lib
LSP_R3D_WGL_LIB_TYPE       := src
LSP_R3D_WGL_LIB_URL_RO     := https://github.com/lsp-plugins/$(LSP_R3D_WGL_LIB_NAME).git
LSP_R3D_WGL_LIB_URL_RW     := git@github.com:lsp-plugins/$(LSP_R3D_WGL_LIB_NAME).git

LSP_RUNTIME_LIB_VERSION    := 1.0.22
LSP_RUNTIME_LIB_NAME       := lsp-runtime-lib
LSP_RUNTIME_LIB_TYPE       := src
LSP_RUNTIME_LIB_URL_RO     := https://github.com/lsp-plugins/$(LSP_RUNTIME_LIB_NAME).git
LSP_RUNTIME_LIB_URL_RW     := git@github.com:lsp-plugins/$(LSP_RUNTIME_LIB_NAME).git

LSP_TEST_FW_VERSION        := 1.0.25
LSP_TEST_FW_NAME           := lsp-test-fw
LSP_TEST_FW_TYPE           := src
LSP_TEST_FW_URL_RO         := https://github.com/lsp-plugins/$(LSP_TEST_FW_NAME).git
LSP_TEST_FW_URL_RW         := git@github.com:lsp-plugins/$(LSP_TEST_FW_NAME).git

#------------------------------------------------------------------------------
# Variables that describe system dependencies
LIBCAIRO_VERSION           := system
LIBCAIRO_NAME              := cairo
LIBCAIRO_TYPE              := pkg

LIBDL_VERSION              := system
LIBDL_NAME                 := libdl
LIBDL_TYPE                 := opt
LIBDL_LDFLAGS              := -ldl

LIBD2D1_VERSION            := system
LIBD2D1_NAME               := libd2d1
LIBD2D1_TYPE               := opt
LIBD2D1_LDFLAGS            := -ld2d1

LIBDWRITE_VERSION          := system
LIBDWRITE_NAME             := libdwrite
LIBDWRITE_TYPE             := opt
LIBDWRITE_LDFLAGS          := -ldwrite

LIBFREETYPE_VERSION        := system
LIBFREETYPE_NAME           := freetype2
LIBFREETYPE_TYPE           := pkg

LIBGL_VERSION              := system
LIBGL_NAME                 := gl
LIBGL_TYPE                 := pkg

LIBGDI32_VERSION           := system
LIBGDI32_NAME              := libgid32
LIBGDI32_TYPE              := opt
LIBGDI32_LDFLAGS           := -lgdi32

LIBICONV_VERSION           := system
LIBICONV_NAME              := libiconv
LIBICONV_TYPE              := opt
LIBICONV_LDFLAGS           := -liconv

LIBMSACM_VERSION           := system
LIBMSACM_NAME              := libmsacm
LIBMSACM_TYPE              := opt
LIBMSACM_LDFLAGS           := -lmsacm32

LIBOLE_VERSION             := system
LIBOLE_NAME                := libole
LIBOLE_TYPE                := opt
LIBOLE_LDFLAGS             := -lole32

LIBOPENGL32_VERSION        := system
LIBOPENGL32_NAME           := libole
LIBOPENGL32_TYPE           := opt
LIBOPENGL32_LDFLAGS        := -lopengl32

LIBPTHREAD_VERSION         := system
LIBPTHREAD_NAME            := libpthread
LIBPTHREAD_TYPE            := opt
LIBPTHREAD_LDFLAGS         := -lpthread

LIBRT_VERSION              := system
LIBRT_NAME                 := librt
LIBRT_TYPE                 := opt
LIBRT_LDFLAGS              := -lrt

LIBSNDFILE_VERSION         := system
LIBSNDFILE_NAME            := sndfile
LIBSNDFILE_TYPE            := pkg

LIBSHLWAPI_VERSION         := system
LIBSHLWAPI_NAME            := libshlwapi
LIBSHLWAPI_TYPE            := opt
LIBSHLWAPI_LDFLAGS         := -lshlwapi

LIBUUID_VERSION            := system
LIBUUID_NAME               := libuuid
LIBUUID_TYPE               := opt
LIBUUID_LDFLAGS            := -luuid

LIBWINCODEC_VERSION        := system
LIBWINCODEC_NAME           := libwincodec
LIBWINCODEC_TYPE           := opt
LIBWINCODEC_LDFLAGS        := -lwindowscodecs

LIBWINMM_VERSION           := system
LIBWINMM_NAME              := libwinmm
LIBWINMM_TYPE              := opt
LIBWINMM_LDFLAGS           := -lwinmm

LIBX11_VERSION             := system
LIBX11_NAME                := x11
LIBX11_TYPE                := pkg

LIBXRANDR_VERSION          := system
LIBXRANDR_NAME             := xrandr
LIBXRANDR_TYPE             := pkg

LIBMPR_VERSION             := system
LIBMPR_NAME                := libmpr
LIBMPR_TYPE                := opt
LIBMPR_LDFLAGS             := -lmpr

