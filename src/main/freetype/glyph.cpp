/*

 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 8 апр. 2023 г.
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/stdlib.h>

#include <private/freetype/glyph.h>

#ifdef USE_LIBFREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H

namespace lsp
{
    namespace ws
    {
        static constexpr int32_t face_slant  = 180; // sinf(M_PI * 9.0f / 180.0f) * 0x10000

        namespace ft
        {
            glyph_t *make_glyph(face_t *face, lsp_wchar_t ch)
            {
                int error;

                // Set transformation matrix
                FT_Matrix mt;
                mt.xx   = 1 * 0x10000;
                mt.xy   = (face->flags & FACE_SLANT) ? face_slant : 0;
                mt.yx   = 0;
                mt.yy   = 1 * 0x10000;

                FT_Set_Transform(face->ft_face, &mt, NULL);

                // Obtain the glyph index
                FT_UInt glyph_index = FT_Get_Char_Index(face->ft_face, ch);

                // Load glyph
                size_t load_flags   = (face->flags & FACE_ANTIALIAS) ? FT_LOAD_DEFAULT : FT_LOAD_MONOCHROME;
                if ((error = FT_Load_Glyph(face->ft_face, glyph_index, load_flags )) != FT_Err_Ok)
                    return NULL;

                // Render the glyph
                FT_GlyphSlot glyph  = face->ft_face->glyph;
                FT_Render_Mode render_mode  = (face->flags & FACE_ANTIALIAS) ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO;
                if ((error = FT_Render_Glyph(glyph, render_mode )) != FT_Err_Ok)
                    return NULL;

                uint32_t format = FMT_1_BPP;
                switch (glyph->bitmap.pixel_mode)
                {
                    case FT_PIXEL_MODE_MONO:
                        format = FMT_1_BPP;
                        break;
                    case FT_PIXEL_MODE_GRAY2:
                        format = FMT_2_BPP;
                        break;
                    case FT_PIXEL_MODE_GRAY4:
                        format = FMT_4_BPP;
                        break;
                    case FT_PIXEL_MODE_GRAY:
                        format = FMT_8_BPP;
                        break;
                    default:
                        return NULL;
                }

                // Copy the glyph data
                FT_Bitmap *bitmap   = &glyph->bitmap;
                size_t szof_glyph   = align_size(sizeof(glyph_t), sizeof(size_t));
                size_t stride       = lsp_abs(bitmap->pitch);
                size_t bytes        = bitmap->rows * stride;
                size_t to_alloc     = szof_glyph + bytes;

                uint8_t *buf        = static_cast<uint8_t *>(malloc(to_alloc));
                if (buf == NULL)
                    return NULL;
                glyph_t *res        = reinterpret_cast<glyph_t *>(buf);

                res->face           = face;
                res->codepoint      = ch;
                res->szof           = to_alloc;
                res->x_advance      = glyph->advance.x;
                res->y_advance      = glyph->advance.y;
                res->x_bearing      = glyph->bitmap_left;
                res->y_bearing      = glyph->bitmap_top;

                res->bitmap.width   = bitmap->width;
                res->bitmap.height  = bitmap->rows;
                res->bitmap.stride  = stride;
                res->bitmap.data    = &buf[szof_glyph];
                res->format         = format;

                // Copy the bitmap data of the glyph
                if (bitmap->pitch < 0)
                {
                    const uint8_t *src  = bitmap->buffer;
                    uint8_t *dst        = res->bitmap.data;
                    for (ssize_t y=0; y<res->bitmap.height; ++y)
                    {
                        memcpy(dst, src, stride);
                        dst            += stride;
                        src            += bitmap->pitch;
                    }
                }
                else
                    memcpy(res->bitmap.data, bitmap->buffer, bytes);

                // Return result
                return res;
            }

            void free_glyph(glyph_t *glyph)
            {
                if (glyph->face != NULL)
                    --glyph->face->references;
                free(glyph);
            }
        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */




