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

#ifdef USE_LIBFREETYPE

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/stdlib.h>

#include <private/freetype/bitmap.h>
#include <private/freetype/glyph.h>
#include <private/freetype/face.h>

#include <ft2build.h>
#include FT_BITMAP_H
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            static glyph_t *make_glyph_data(face_t *face, FT_GlyphSlot glyph, lsp_wchar_t ch)
            {
                // Obtain bitmap and pixel format
                FT_Bitmap *bitmap   = &glyph->bitmap;
                uint32_t format = FMT_1_BPP;
                switch (bitmap->pixel_mode)
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
                size_t szof_glyph   = sizeof(glyph_t) + DEFAULT_ALIGN;
                size_t stride       = lsp_abs(bitmap->pitch);
                size_t bytes        = bitmap->rows * stride;
                size_t to_alloc     = szof_glyph + bytes;

                uint8_t *buf        = static_cast<uint8_t *>(malloc(to_alloc));
                if (buf == NULL)
                    return NULL;
                glyph_t *res        = reinterpret_cast<glyph_t *>(buf);

                res->lru_prev       = NULL;
                res->lru_next       = NULL;
                res->cache_next     = NULL;
                res->face           = face;
                res->codepoint      = ch;
                res->szof           = to_alloc;
                res->width          = glyph->metrics.width;
                res->height         = glyph->metrics.height;
                res->x_advance      = glyph->advance.x;
                res->y_advance      = glyph->advance.y;
                res->x_bearing      = glyph->bitmap_left;
                res->y_bearing      = glyph->bitmap_top;

                res->bitmap.width   = bitmap->width;
                res->bitmap.height  = bitmap->rows;
                res->bitmap.stride  = stride;
                res->bitmap.data    = align_ptr(&buf[sizeof(glyph_t)], DEFAULT_ALIGN);
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

            LSP_HIDDEN_MODIFIER
            glyph_t *render_regular_glyph(face_t *face, FT_UInt glyph_index, lsp_wchar_t ch)
            {
                // Load glyph
                size_t load_flags   = (face->flags & FID_ANTIALIAS) ? FT_LOAD_DEFAULT : FT_LOAD_MONOCHROME;
                load_flags         |= FT_LOAD_FORCE_AUTOHINT;
                if (FT_Load_Glyph(face->ft_face, glyph_index, load_flags) != FT_Err_Ok)
                    return NULL;

                // Render glyph
                FT_GlyphSlot glyph  = face->ft_face->glyph;
                FT_Render_Mode render_mode  = (face->flags & FID_ANTIALIAS) ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO;
                if (FT_Render_Glyph(glyph, render_mode ) != FT_Err_Ok)
                    return NULL;

                return make_glyph_data(face, glyph, ch);
            }

            LSP_HIDDEN_MODIFIER
            glyph_t *render_bold_glyph(FT_Library library, face_t *face, FT_UInt glyph_index, lsp_wchar_t ch)
            {
                // Load glyph
                size_t load_flags   = (face->flags & FID_ANTIALIAS) ? FT_LOAD_DEFAULT : FT_LOAD_MONOCHROME;
                load_flags         |= FT_LOAD_FORCE_AUTOHINT;
                if (FT_Load_Glyph(face->ft_face, glyph_index, load_flags) != FT_Err_Ok)
                    return NULL;

                // Get the glyph
                FT_GlyphSlot glyph  = face->ft_face->glyph;
                const bool is_outline = glyph->format == FT_GLYPH_FORMAT_OUTLINE;
                size_t embolden     = size_t(lsp_max(face->h_size, face->v_size)) >> 5;
                if (is_outline)
                    FT_Outline_Embolden(&glyph->outline, embolden);

                // Render glyph
                FT_Render_Mode render_mode  = (face->flags & FID_ANTIALIAS) ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO;
                if (FT_Render_Glyph(glyph, render_mode ) != FT_Err_Ok)
                    return NULL;

                if (!is_outline)
                {
                    if (FT_Bitmap_Embolden(library, &glyph->bitmap, embolden >> 1, 0) != FT_Err_Ok)
                        return NULL;
                }

                return make_glyph_data(face, glyph, ch);
            }

            LSP_HIDDEN_MODIFIER
            glyph_t *render_glyph(FT_Library library, face_t *face, lsp_wchar_t ch)
            {
                // Obtain the glyph index
                FT_UInt glyph_index = FT_Get_Char_Index(face->ft_face, ch);

                // Render the glyph
                if ((face->flags & FID_BOLD) && (!(face->ft_face->style_flags & FT_STYLE_FLAG_BOLD)))
                    return render_bold_glyph(library, face, glyph_index, ch);

                return render_regular_glyph(face, glyph_index, ch);
            }

            LSP_HIDDEN_MODIFIER
            void free_glyph(glyph_t *glyph)
            {
                if (glyph == NULL)
                    return;

                free(glyph);
            }
        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */




