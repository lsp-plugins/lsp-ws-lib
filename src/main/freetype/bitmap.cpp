/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 29 апр. 2023 г.
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

#ifdef USE_LIBFREETYPE

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>

#include <private/freetype/types.h>
#include <private/freetype/bitmap.h>

#ifdef USE_LIBCAIRO

#include <cairo/cairo.h>

#endif /* USE_LIBCAIRO */

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            LSP_HIDDEN_MODIFIER
            size_t compute_bitmap_stride(size_t width)
            {
            #if defined(USE_LIBCAIRO)
                return cairo_format_stride_for_width(CAIRO_FORMAT_A8, width);
            #else
                return sizeof(size_t);
            #endif /* USE_LIBCAIRO */
            }

            LSP_HIDDEN_MODIFIER
            dsp::bitmap_t *create_bitmap(size_t width, size_t height)
            {
                size_t stride       = compute_bitmap_stride(width);
                size_t szof_bitmap  = sizeof(dsp::bitmap_t) + DEFAULT_ALIGN;
                size_t buf_size     = stride * height;

                uint8_t *ptr        = static_cast<uint8_t *>(malloc(buf_size + szof_bitmap));
                if (ptr == NULL)
                    return NULL;

                dsp::bitmap_t *b    = reinterpret_cast<dsp::bitmap_t *>(ptr);
                b->width            = width;
                b->height           = height;
                b->stride           = stride;
                b->data             = align_ptr(&ptr[sizeof(dsp::bitmap_t)], DEFAULT_ALIGN);

                bzero(b->data, buf_size);

                return b;
            }

            LSP_HIDDEN_MODIFIER
            void free_bitmap(dsp::bitmap_t *bitmap)
            {
                if (bitmap != NULL)
                    free(bitmap);
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */


