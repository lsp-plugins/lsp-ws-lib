/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 3 февр. 2025 г.
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

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <private/gl/TextAllocator.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {

            TextAllocator::TextAllocator(gl::IContext *ctx)
            {
                pContext        = safe_acquire(ctx);
                nReferences     = 1;
                pTexture        = NULL;
                nTop            = 0;
            }

            TextAllocator::~TextAllocator()
            {
                clear();

                safe_release(pTexture);
                safe_release(pContext);
            }

            uatomic_t TextAllocator::reference_up()
            {
                return atomic_add(&nReferences, 1) + 1;
            }

            uatomic_t TextAllocator::reference_down()
            {
                uatomic_t result = atomic_add(&nReferences, -1) - 1;
                if (result == 0)
                    delete this;
                return result;
            }

            void TextAllocator::clear()
            {
                // Release textures associated with rows
                for (size_t i=0, n=vRows.size(); i<n; ++i)
                {
                    row_t *row = vRows.uget(i);
                    if (row == NULL)
                        continue;

                    safe_release(row->pTexture);
                }

                // Cleanup row allocation
                vRows.clear();
                nTop            = 0;
            }

            size_t TextAllocator::first_row_id(size_t height)
            {
                row_t *r;
                const ssize_t end = vRows.size() - 1;
                ssize_t first = 0, last = end, middle;
                if (last < 0)
                    return 0;

                while (first <= last)
                {
                    middle          = (first + last) >> 1;
                    r               = vRows.uget(middle);
                    if (r->nHeight >= height)
                        last            = middle - 1;
                    else
                        first           = middle + 1;
                }

                // 'first' points to the row with maximum height which is less than 'height'
                if (first > end)
                    return first;
                r               = vRows.uget(first);
                if (r->nHeight >= height)
                    return first;

                // Perform heuristics
                if (++first > end)
                    return first;

                r               = vRows.uget(first);
                return (r->nHeight >= height) ? first : first - 1;
            }

            gl::Texture *TextAllocator::fill_texture(ws::rectangle_t *rect, row_t *row, const void *data, size_t width, size_t stride)
            {
                status_t res;

                // Set allocation area and update row information
                rect->nLeft     = row->nWidth;
                rect->nTop      = row->nTop;
                rect->nWidth    = width;
                rect->nHeight   = row->nHeight;

                row->nWidth    += width;

                // Check that we already have a texture
                if (row->pTexture != NULL)
                {
                    res = row->pTexture->set_subimage(
                        data,
                        rect->nLeft, rect->nTop, rect->nWidth, rect->nHeight,
                        stride);

                    return (res == STATUS_OK) ? safe_acquire(row->pTexture) : NULL;
                }

                // Allocate new current texture if needed
                if (pTexture == NULL)
                {
                    pTexture        = new gl::Texture(pContext);
                    if (pTexture == NULL)
                        return NULL;

                    res = pTexture->set_image(
                        NULL,
                        TEXT_ATLAS_SIZE, TEXT_ATLAS_SIZE,
                        0, gl::TEXTURE_ALPHA8);
                    if (res != STATUS_OK)
                        return NULL;
                }

                res = pTexture->set_subimage(
                    data,
                    rect->nLeft, rect->nTop, rect->nWidth, rect->nHeight,
                    stride);
                if (res != STATUS_OK)
                    return NULL;

                // Update the row
                row->pTexture       = safe_acquire(pTexture);

                return safe_acquire(row->pTexture);
            }

            gl::Texture *TextAllocator::allocate(ws::rectangle_t *rect, const void *data, size_t width, size_t height, size_t stride)
            {
                row_t *row;

                pContext->activate();

                // Try to find matching row and allocate space there
                size_t index    = first_row_id(height);
                for ( ; index < vRows.size(); ++index)
                {
                    row             = vRows.uget(index);
                    if ((row->nHeight != height) || (row->pTexture == NULL))
                        break;

                    // Try to allocate space
                    if ((row->nWidth + width) <= row->pTexture->width())
                        return fill_texture(rect, row, data, width, stride);
                }

                // Create new row
                row     = vRows.insert(index);
                if (row == NULL)
                    return NULL;

                // Fill row parameters
                if ((nTop + height) <= TEXT_ATLAS_SIZE)
                {
                    row->nTop       = nTop;
                    row->pTexture   = safe_acquire(pTexture);
                    nTop           += height;
                }
                else
                {
                    row->nTop       = 0;
                    row->pTexture   = NULL;
                    nTop            = height;

                    // Reset texture identifier as we went out of possible texture bounds
                    safe_release(pTexture);
                }

                row->nWidth     = 0;
                row->nHeight    = height;

                return fill_texture(rect, row, data, width, stride);
            }

            gl::Texture *TextAllocator::current()
            {
                return pTexture;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */
