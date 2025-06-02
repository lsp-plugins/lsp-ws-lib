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

#include <private/gl/Allocator.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/runtime/system.h>

#include <private/gl/Stats.h>
#include <private/gl/Texture.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            Allocator::Allocator()
            {
                pFree                   = NULL;
            }

            Allocator::~Allocator()
            {
                clear();
            }

            void Allocator::clear()
            {
                // Destroy all released draws
                for (batch_draw_t *draw = pFree; draw != NULL; )
                {
                    batch_draw_t *next  = draw->next;
                    destroy_draw(draw);
                    draw                = next;
                }

                // Cleanup pointer
                pFree           = NULL;

                OPENGL_OUTPUT_STATS(true);
            }

            batch_draw_t *Allocator::alloc_draw(const batch_header_t & header)
            {
                // Check if we can reuse some draw batch
                batch_draw_t *draw      = pFree;
                if (draw != NULL)
                {
                    OPENGL_INC_STATS(draw_acquire);
                    pFree                   = draw->next;

                    draw->header            = header;
                    draw->vertices.count    = 0;
                    draw->indices.count     = 0;
                    draw->next              = NULL;
                    draw->ttl               = 0;

                    safe_acquire(draw->header.pTexture);

                    return draw;
                }

                // Allocate new draw element
                draw                    = static_cast<batch_draw_t *>(malloc(sizeof(batch_draw_t)));
                if (draw == NULL)
                    return NULL;

                OPENGL_INC_STATS(draw_alloc);

                draw->header            = header;
                draw->vertices.v        = NULL;
                draw->vertices.count    = 0;
                draw->vertices.capacity = 0x40;

                draw->indices.data      = NULL;
                draw->indices.count     = 0;
                draw->indices.capacity  = 0x100;
                draw->indices.szof      = sizeof(uint8_t);

                draw->next              = NULL;
                draw->ttl               = 0;

                safe_acquire(draw->header.pTexture);

                lsp_finally { destroy_draw(draw); };

                // Initialize new draw batch
                draw->vertices.v        = static_cast<vertex_t *>(malloc(sizeof(vertex_t) * draw->vertices.capacity));
                if (draw->vertices.v == NULL)
                    return NULL;

                OPENGL_INC_STATS(vertex_alloc);

                draw->indices.data      = malloc(sizeof(uint8_t) * draw->indices.capacity);
                if (draw->indices.data == NULL)
                    return NULL;

                OPENGL_INC_STATS(index_alloc);

                return release_ptr(draw);
            }

            void Allocator::release_draw(batch_draw_t *draw)
            {
                if (draw == NULL)
                    return;

                OPENGL_INC_STATS(draw_release);

                safe_release(draw->header.pTexture);

                draw->vertices.count    = 0;
                draw->indices.count     = 0;
                draw->next              = pFree;
                pFree                   = draw;
            }

            void Allocator::destroy_draw(batch_draw_t *draw)
            {
                if (draw == NULL)
                    return;

                safe_release(draw->header.pTexture);

                if (draw->vertices.v != NULL)
                {
                    free(draw->vertices.v);
                    draw->vertices.v        = NULL;
                }
                if (draw->indices.data != NULL)
                {
                    free(draw->indices.data);
                    draw->indices.data      = NULL;
                }

                free(draw);

                OPENGL_INC_STATS(draw_free);
            }

            void Allocator::perform_gc()
            {
                // Destroy all outdated draws
                for (batch_draw_t **pdraw = &pFree; *pdraw != NULL; )
                {
                    batch_draw_t * const draw = *pdraw;
                    if ((draw->ttl++) >= 16)
                    {
                        *pdraw          = draw->next;
                        destroy_draw(draw);
                    }
                    else
                        pdraw           = &draw->next;
                }
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */
