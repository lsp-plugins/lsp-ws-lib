/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 23 апр. 2023 г.
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
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/lltl/phashset.h>

#include <private/freetype/face.h>
#include <private/freetype/glyph.h>
#include <private/freetype/types.h>

inline void *operator new(size_t size, void *ptr, const lsp::ws::ft::allocator_tag_t & tag)
{
    return ptr;
}

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            size_t make_face_flags(const Font *f)
            {
                size_t flags    = (f->italic()) ? FACE_SLANT : 0;
                if (f->bold())
                    flags          |= FACE_BOLD;
                if (f->antialias() != FA_DISABLED)
                    flags          |= FACE_ANTIALIAS;
                return flags;
            }

            face_t *create_face(FT_Face ft_face, float size, uint32_t flags)
            {
                FT_Error error;

                // Reference the freetype face
                if ((error = FT_Reference_Face(ft_face)) != FT_Err_Ok)
                    return NULL;
                lsp_finally {
                    if (ft_face != NULL)
                        FT_Done_Face(ft_face);
                };

                // Select the font size
                f24p6_t h_size      = (ft_face->face_flags & FT_FACE_FLAG_HORIZONTAL) ? float_to_f24p6(size) : 0;
                f24p6_t v_size      = (ft_face->face_flags & FT_FACE_FLAG_HORIZONTAL) ? 0 : float_to_f24p6(size);
                if ((error = FT_Set_Char_Size(ft_face, h_size, v_size, 0, 0)) != FT_Err_Ok)
                    return NULL;

                // Allocate the font face object
                face_t *face    = reinterpret_cast<face_t *>(malloc(sizeof(face_t)));
                if (face == NULL)
                    return NULL;

                // Initialize font face object
                face->references    = 0;
                face->cache_size    = 0;
                face->ft_face       = ft_face;

                face->flags         = flags;
                face->h_size        = h_size;
                face->v_size        = v_size;
                face->height        = ft_face->size->metrics.height;
                face->ascend        = ft_face->size->metrics.ascender;
                face->descend       = ft_face->size->metrics.descender;

                allocator_tag_t tag;
                new (&face->cache, tag) lltl::phashset<glyph_t>(glyph_hash_iface(), glyph_compare_iface());

                // Cleanup the pointer to avoid face destruction
                ft_face             = NULL;

                return face;
            }

            void destroy_face(face_t *face)
            {
                if (face == NULL)
                    return;

                // Remove freetype face reference
                if (face->ft_face != NULL)
                {
                    FT_Done_Face(face->ft_face);
                    face->ft_face   = NULL;
                }

                // Drop all glyphs in the cache
                lltl::parray<glyph_t> vv;
                if (face->cache.values(&vv))
                {
                    for (size_t i=0, n=vv.size(); i<n; ++i)
                        free_glyph(vv.uget(i));
                }
                face->cache.flush();
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */



