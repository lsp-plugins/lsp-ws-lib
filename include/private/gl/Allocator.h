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

#ifndef PRIVATE_GL_ALLOCATOR_H_
#define PRIVATE_GL_ALLOCATOR_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>

#include <private/gl/Data.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class LSP_HIDDEN_MODIFIER Allocator
            {
                private:
                    batch_draw_t   *pFree;

                public:
                    Allocator();
                    Allocator(const Allocator &) = delete;
                    Allocator(Allocator &&) = delete;
                    ~Allocator();
                    Allocator & operator = (const Allocator &) = delete;
                    Allocator & operator = (Allocator &&) = delete;

                protected:
                    void            destroy_draw(batch_draw_t *draw);

                public:
                    void            clear();
                    void            perform_gc();

                public:
                    batch_draw_t   *alloc_draw(const batch_header_t & header);
                    void            release_draw(batch_draw_t *draw);
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_ALLOCATOR_H_ */
