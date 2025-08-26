/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 2 июн. 2025 г.
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

#ifndef PRIVATE_GL_STATS_H_
#define PRIVATE_GL_STATS_H_

#include <private/gl/defs.h>

#ifdef TRACE_OPENGL_STATS

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/runtime/system.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            typedef struct gl_stats_t
            {
                size_t surface_alloc;
                size_t surface_free;
                size_t batch_alloc;
                size_t batch_free;
                size_t draw_alloc;
                size_t draw_free;
                size_t draw_acquire;
                size_t draw_release;
                size_t cmd_alloc;
                size_t cmd_realloc;
                size_t vertex_alloc;
                size_t vertex_realloc;
                size_t index_alloc;
                size_t index_realloc;

                gl_stats_t();
            } gl_stats_t;

            LSP_HIDDEN_MODIFIER
            extern gl_stats_t gl_stats;

            LSP_HIDDEN_MODIFIER
            void output_stats(bool immediate);

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#define OPENGL_INC_STATS(name) \
    ++::lsp::ws::gl::gl_stats.name

#define OPENGL_OUTPUT_STATS(immediate) \
    ::lsp::ws::gl::output_stats(immediate)

#else

    #define OPENGL_INC_STATS(...)
    #define OPENGL_OUTPUT_STATS(...)

#endif /* TRACE_OPENGL_STATS */

#endif /* PRIVATE_GL_STATS_H_ */
