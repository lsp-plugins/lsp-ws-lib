/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 1 июн. 2026 г.
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

#ifndef PRIVATE_FREETYPE_LIBRARY_H_
#define PRIVATE_FREETYPE_LIBRARY_H_

#include <lsp-plug.in/ws/ws.h>

#ifdef USE_LIBFREETYPE

#include <ft2build.h>
#include FT_BITMAP_H
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <lsp-plug.in/ipc/Library.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            typedef struct library_t
            {
                private:
                    FT_Library                      hLibrary;
                    ipc::Library                    sLibrary;

                    decltype(&FT_Init_FreeType)     pInit_FreeType;
                    decltype(&FT_Done_FreeType)     pDone_FreeType;
                    decltype(&FT_Library_Version)   pLibrary_Version;

                    decltype(&FT_Open_Face)         pOpen_Face;
                    decltype(&FT_Done_Face)         pDone_Face;
                    decltype(&FT_Reference_Face)    pReference_Face;
                    decltype(&FT_Set_Transform)     pSet_Transform;

                    decltype(&FT_Load_Glyph)        pLoad_Glyph;
                    decltype(&FT_Bitmap_Embolden)   pBitmap_Embolden;
                    decltype(&FT_Outline_Embolden)  pOutline_Embolden;
                    decltype(&FT_Render_Glyph)      pRender_Glyph;
                    decltype(&FT_Get_Char_Index)    pGet_Char_Index;

                private:
                    void                            init_functions();

                public:
                    library_t();
                    library_t(const library_t &) = delete;
                    library_t(library_t &&) = delete;
                    ~library_t();

                    library_t & operator = (const library_t &) = delete;
                    library_t & operator = (library_t &&) = delete;

                    status_t                        init();
                    void                            destroy();

                public:
                    inline bool                     initialized() const     { return hLibrary != NULL; }
                    void                            version(FT_Int *major, FT_Int *minor, FT_Int *patch);
                    FT_Error                        open_face(const FT_Open_Args *args, FT_Long face_index, FT_Face *aface);
                    FT_Error                        done_face(FT_Face face);
                    FT_Error                        reference_face(FT_Face face);
                    void                            set_transform(FT_Face face, FT_Matrix *matrix, FT_Vector*delta);
                    FT_Error                        load_glyph(FT_Face face, FT_UInt glyph_index, FT_Int32 load_flags);
                    FT_Error                        outline_embolden(FT_Outline *outline, FT_Pos strength);
                    FT_Error                        bitmap_embolden(FT_Bitmap *bitmap, FT_Pos xStrength, FT_Pos yStrength);
                    FT_Error                        render_glyph(FT_GlyphSlot slot, FT_Render_Mode render_mode);
                    FT_UInt                         get_char_index(FT_Face face, FT_ULong charcode);
            } library_t;

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_LIBRARY_H_ */
