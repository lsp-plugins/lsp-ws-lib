/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 31 янв. 2025 г.
 *
 * lsp-ws-lib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-ws-lib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-ws-lib. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_GL_DEFS_H_
#define PRIVATE_GL_DEFS_H_

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/debug.h>

// Detect what type of OpenGL do we use
#if defined(USE_LIBGL) && defined(USE_LIBX11) && defined(USE_LIBFREETYPE) && defined(USE_LIBFONTCONFIG)
    #define LSP_PLUGINS_USE_OPENGL_GLX
#endif /* USE_LIBGL && USE_LIBX11 && USE_LIBFREETYPE && USE_LIBFONTCONFIG */

#ifdef LSP_PLUGINS_USE_OPENGL_GLX
    #if defined(PLATFORM_LINUX)
        #include <GL/gl.h>
        #include <GL/glext.h>
    #elif defined(PLATFORM_FREEBSD)
        #include <GL/gl.h>
        #include <GL/glext.h>
    #endif
#endif /* LSP_PLUGINS_USE_OPENGL_GLX */

// Enable overall OpenGL
#if defined(LSP_PLUGINS_USE_OPENGL_GLX)
    #define LSP_PLUGINS_USE_OPENGL
#endif /* LSP_PLUGINS_USE_OPENGL_GLX */

// Uncomment this to log all OpenGL object allocations and deletions
//#define TRACE_OPENGL_ALLOCATIONS

// Uncomment this to log all OpenGL statistics
//#define TRACE_OPENGL_STATS

#ifdef TRACE_OPENGL_ALLOCATIONS
    #define IF_TRACE_OPENGL_ALLOCATIONS(...) __VA_ARGS__
    #define lsp_gl_trace(...) lsp_trace(__VA_ARGS__)
#else
    #define IF_TRACE_OPENGL_ALLOCATIONS(...)
    #define lsp_gl_trace(...) LSP_DEBUG_STUB_CALL
#endif

#endif /* PRIVATE_GL_DEFS_H_ */
