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

#include <private/gl/Stats.h>

#ifdef TRACE_OPENGL_STATS

#include <lsp-plug.in/runtime/system.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            static system::time_millis_t stat_time = 0;
            gl_stats_t gl_stats;

            gl_stats_t::gl_stats_t()
            {
                surface_alloc   = 0;
                surface_free    = 0;
                batch_alloc     = 0;
                batch_free      = 0;
                draw_alloc      = 0;
                draw_free       = 0;
                draw_acquire    = 0;
                draw_release    = 0;
                cmd_alloc       = 0;
                cmd_realloc     = 0;
                vertex_alloc    = 0;
                vertex_realloc  = 0;
                index_alloc     = 0;
                index_realloc   = 0;
            }

            void output_stats(bool immediate)
            {
                const system::time_millis_t ctime = system::get_time_millis();
                if ((immediate) || ((ctime - stat_time) >= 1000))
                {
                    lsp_trace(
                        "Batch allocation statistics: "
                        "batches=[alloc=%d, free=%d], "
                        "draws=[alloc=%d, free=%d, acq=%d, rel=%d], "
                        "indices=[alloc=%d, realloc=%d], "
                        "vertices=[alloc=%d, realloc=%d], "
                        "commands=[alloc=%d, realloc=%d], "
                        "surface=[alloc=%d, free=%d]",
                        int(gl_stats.batch_alloc), int(gl_stats.batch_free),
                        int(gl_stats.draw_alloc), int(gl_stats.draw_free), int(gl_stats.draw_acquire), int(gl_stats.draw_release),
                        int(gl_stats.index_alloc), int(gl_stats.index_realloc),
                        int(gl_stats.vertex_alloc), int(gl_stats.vertex_realloc),
                        int(gl_stats.cmd_alloc), int(gl_stats.cmd_realloc),
                        int(gl_stats.surface_alloc), int(gl_stats.surface_free));
                    stat_time       = ctime;
                }
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* TRACE_OPENGL_STATS */
