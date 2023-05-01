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

#ifndef PRIVATE_FREETYPE_BITMAP_H_
#define PRIVATE_FREETYPE_BITMAP_H_

#ifdef USE_LIBFREETYPE

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>

#include <private/freetype/types.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            /**
             * Compute optimal stride between rows in bytes for monochrome bitmap with 8 bits per pixel
             * @param width width of the bitmap
             * @return stride of the bitmap in bytes
             */
            size_t compute_bitmap_stride(size_t width);

            /**
             * Create bitmap, 1 byte per pixel
             * @param width width of the bitmap
             * @param height height of the bitmap
             * @return pointer to bitmap or NULL
             */
            dsp::bitmap_t *create_bitmap(size_t width, size_t height);

            void free_bitmap(dsp::bitmap_t *bitmap);

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */


#endif /* PRIVATE_FREETYPE_BITMAP_H_ */
