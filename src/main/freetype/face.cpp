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
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/stdlib/stdlib.h>

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

            static void release_font_data(font_t *font)
            {
                if ((--font->references) > 0)
                    return;

                lsp_trace("Dealocated font data %p, size=%d, content=%p", font, int(font->size), font->data);

                free(font->data);

                font->size          = 0;
                font->data          = NULL;

                free(font);
            }

            static font_t *create_font_data(io::IInStream *is)
            {
                status_t res;

                // Make memory chunk from the input stream
                io::OutMemoryStream os;
                if (is->avail() > 0)
                    os.reserve(is->avail());
                if ((res = is->sink(&os)) != STATUS_OK)
                    return NULL;

                // Create font data object
                font_t *font = static_cast<font_t *>(malloc(sizeof(font_t)));
                if (font == NULL)
                    return NULL;

                font->references    = 1;
                font->size          = os.size();
                font->data          = os.release();

                lsp_trace("Allocated font data %p, size=%d, content=%p", font, int(font->size), font->data);

                return font;
            }

            status_t load_face(lltl::parray<face_t> *faces, FT_Library ft, io::IInStream *is)
            {
                // Create the font data
                font_t *data        = create_font_data(is);
                if (data == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { release_font_data(data); };

                // Estimate the number of faces
                FT_Open_Args args;
                FT_Face ft_face;
                FT_Error error;

                // Estimate number of faces
                ssize_t num_faces   = 0;
                {
                    args.flags          = FT_OPEN_MEMORY;
                    args.memory_base    = data->data;
                    args.memory_size    = data->size;
                    args.pathname       = NULL;
                    args.stream         = NULL;
                    args.driver         = NULL;
                    args.num_params     = 0;
                    args.params         = NULL;

                    if ((error = FT_Open_Face( ft, &args, -1, &ft_face )) != FT_Err_Ok)
                        return STATUS_UNKNOWN_ERR;
                    lsp_finally { FT_Done_Face(ft_face ); };

                    num_faces       = ft_face->num_faces;
                }

                // Load each face
                lltl::parray<face_t> list;
                lsp_finally { destroy_faces(&list); };

                for (ssize_t i=0; i<num_faces; ++i)
                {
                    args.flags          = FT_OPEN_MEMORY;
                    args.memory_base    = data->data;
                    args.memory_size    = data->size;
                    args.pathname       = NULL;
                    args.stream         = NULL;
                    args.driver         = NULL;
                    args.num_params     = 0;
                    args.params         = NULL;

                    if ((error = FT_Open_Face( ft, &args, i, &ft_face )) != FT_Err_Ok)
                        return STATUS_UNKNOWN_ERR;
                    lsp_finally {
                        if (ft_face != NULL)
                            FT_Done_Face( ft_face );
                    };

                    // Allocate the font face object
                    face_t *face    = reinterpret_cast<face_t *>(malloc(sizeof(face_t)));
                    if (face == NULL)
                        return STATUS_NO_MEM;
                    lsp_finally { destroy_face(face); };

                    // Initialize font face object
                    face->references    = 0;
                    face->cache_size    = 0;
                    face->ft_face       = ft_face;
                    face->font          = data;

                    face->flags         = 0;
                    if (ft_face->style_flags & FT_STYLE_FLAG_ITALIC)
                        face->flags        |= FACE_SLANT;
                    if (ft_face->style_flags & FT_STYLE_FLAG_BOLD)
                        face->flags        |= FACE_BOLD;

                    face->h_size        = 0;
                    face->v_size        = 0;
                    face->height        = 0;
                    face->ascend        = 0;
                    face->descend       = 0;

                    allocator_tag_t tag;
                    new (&face->cache, tag) lltl::phashset<glyph_t>(glyph_hash_iface(), glyph_compare_iface());

                    // Cleanup the pointer to avoid face destruction
                    ++face->font->references;
                    ft_face             = NULL;

                    // Add the created face to list
                    if (!list.add(face))
                        return STATUS_NO_MEM;
                    face                = NULL;
                }

                // Commit the result and return success
                list.swap(faces);

                return STATUS_OK;
            }

            face_t *clone_face(face_t *src, uint32_t flags)
            {
                FT_Error error;

                // Reference the freetype face
                if ((error = FT_Reference_Face(src->ft_face)) != FT_Err_Ok)
                    return NULL;
                lsp_finally {
                    if (src != NULL)
                        FT_Done_Face(src->ft_face);
                };

                // Allocate the font face object
                face_t *face    = reinterpret_cast<face_t *>(malloc(sizeof(face_t)));
                if (face == NULL)
                    return NULL;

                // Initialize font face object
                face->references    = 0;
                face->cache_size    = 0;
                face->ft_face       = src->ft_face;
                face->font          = src->font;

                face->flags         = flags;
                face->h_size        = 0;
                face->v_size        = 0;
                face->height        = 0;
                face->ascend        = 0;
                face->descend       = 0;

                allocator_tag_t tag;
                new (&face->cache, tag) lltl::phashset<glyph_t>(glyph_hash_iface(), glyph_compare_iface());

                // Cleanup the pointer to avoid face destruction
                ++face->font->references;
                src                 = NULL;

                return face;
            }

            void destroy_faces(lltl::parray<face_t> *faces)
            {
                for (size_t i=0, n=faces->size(); i<n; ++i)
                    destroy_face(faces->uget(i));
                faces->flush();
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

                // Release font data if present
                if (face->font != NULL)
                {
                    release_font_data(face->font);
                    face->font      = NULL;
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

            status_t select_face(face_t *face, float size)
            {
                FT_Error error;
                FT_Face ft_face     = face->ft_face;

                // Select the font size
                f24p6_t h_size      = (ft_face->face_flags & FT_FACE_FLAG_HORIZONTAL) ? float_to_f24p6(size) : 0;
                f24p6_t v_size      = (ft_face->face_flags & FT_FACE_FLAG_HORIZONTAL) ? 0 : float_to_f24p6(size);
                if ((error = FT_Set_Char_Size(ft_face, h_size, v_size, 0, 0)) != FT_Err_Ok)
                    return STATUS_UNKNOWN_ERR;

                // Update the font metrics for the face
                face->h_size        = h_size;
                face->v_size        = v_size;
                face->height        = ft_face->size->metrics.height;
                face->ascend        = ft_face->size->metrics.ascender;
                face->descend       = ft_face->size->metrics.descender;

                return STATUS_OK;
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */



