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

#include <private/freetype/library.h>
#include <lsp-plug.in/common/debug.h>

#ifdef USE_LIBFREETYPE

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            #define FETCH_SYMBOL(name)      reinterpret_cast<decltype(&name)>(sLibrary.import(LSP_STRINGIFY(name)))
            #define COPY_SYMBOL(name)       name

            static constexpr const char * LIBRARY_NAME = "libfreetype" FILE_LIBRARY_EXT_S;

            static const char * const paths[] =
            {
            #if defined(ARCH_64BIT)
                "/usr/lib64",
                "/usr/local/lib64",
                "/lib64",
            #elif defined(ARCH_32BIT)
                "/usr/lib32",
                "/usr/local/lib32",
                "/lib32",
            #endif /* ARCH_64BIT */
                "/usr/lib",
                "/usr/local/lib",
                NULL
            };

            library_t::library_t()
            {
                hLibrary        = NULL;
                init_functions();
            }

            library_t::~library_t()
            {
                destroy();
            }

            void library_t::init_functions()
            {
                #define INIT(func) \
                    pInit_FreeType      = func(FT_Init_FreeType); \
                    pDone_FreeType      = func(FT_Done_FreeType); \
                    pLibrary_Version    = func(FT_Library_Version); \
                    \
                    pOpen_Face          = func(FT_Open_Face); \
                    pDone_Face          = func(FT_Done_Face); \
                    pReference_Face     = func(FT_Reference_Face); \
                    pSet_Transform      = func(FT_Set_Transform); \
                    pLoad_Glyph         = func(FT_Load_Glyph); \
                    pBitmap_Embolden    = func(FT_Bitmap_Embolden); \
                    pOutline_Embolden   = func(FT_Outline_Embolden); \
                    pRender_Glyph       = func(FT_Render_Glyph); \
                    pGet_Char_Index     = func(FT_Get_Char_Index);

                if (sLibrary.is_opened())
                {
                    INIT(FETCH_SYMBOL)
                }
                else
                {
                    INIT(COPY_SYMBOL)
                }
            }

            status_t library_t::init()
            {
                if (hLibrary != NULL)
                    return STATUS_OK;

                init_functions();

                // Try to load FreeType library
                FT_Error error = FT_Err_Ok;
                FT_Int major = 0, minor = 0, patch = 0;
                FT_Library library = NULL;
                if ((error = pInit_FreeType(&library)) == FT_Err_Ok)
                    pLibrary_Version(library, &major, &minor, &patch);
                lsp_finally {
                    if (library != NULL)
                        pDone_FreeType(library);
                };

                if ((error != FT_Err_Ok) || (library == NULL) ||
                    ((major <= 2) && (minor < 11)))
                {
                    LSPString tmp;
                    for (const char * const * path = paths; (path != NULL) && (*path != NULL); ++path)
                    {
                        // Form the path to the library
                        const ssize_t count = tmp.fmt_native("%s" FILE_SEPARATOR_S "%s", *path, LIBRARY_NAME);
                        if (count < 0)
                            continue;

                        status_t res = sLibrary.open(&tmp, true);
                        if (res != STATUS_OK)
                            continue;
                        bool close = true;
                        lsp_finally {
                            if (close)
                                sLibrary.close();
                        };

                        // Obtain the library information
                        decltype(&FT_Init_FreeType) init = FETCH_SYMBOL(FT_Init_FreeType);
                        if (init == NULL)
                            continue;

                        decltype(&FT_Done_FreeType) done = FETCH_SYMBOL(FT_Done_FreeType);
                        if (done == NULL)
                            continue;

                        decltype(&FT_Library_Version) version = FETCH_SYMBOL(FT_Library_Version);
                        if (version == NULL)
                            continue;

                        // Try to load library
                        FT_Library alt_lib;
                        if ((error = init(&alt_lib)) != FT_Err_Ok)
                            continue;

                        // Obtain the version
                        FT_Int alt_major = 0, alt_minor = 0, alt_patch = 0;
                        version(alt_lib, &alt_major, &alt_minor, &alt_patch);
                        if ((alt_major > major) ||
                            ((alt_major == major) && (alt_minor > minor)) ||
                            ((alt_major == major) && (alt_minor == minor) && (alt_patch > patch)))
                        {
                            lsp_trace("Found alternative FreeType library of version %d.%d.%d at %s",
                                int(alt_major), int(alt_minor), int(alt_patch), tmp.get_native());
                            close = false;

                            if (library != NULL)
                                pDone_FreeType(library);

                            library     = alt_lib;
                            close       = false;
                            break;
                        }
                    }
                }

                // Ensure that library is loaded
                if (library == NULL)
                    return STATUS_UNKNOWN_ERR;

                // Initialize state
                init_functions();
                hLibrary            = release_ptr(library);

                return STATUS_OK;
            }

            void library_t::destroy()
            {
                if (hLibrary == NULL)
                    return;

                if (pDone_FreeType)
                    pDone_FreeType(hLibrary);
                hLibrary        = NULL;

                if (sLibrary.is_opened())
                    sLibrary.close();

                init_functions();
            }

            void library_t::version(FT_Int *major, FT_Int *minor, FT_Int *patch)
            {
                pLibrary_Version(hLibrary, major, minor, patch);
            }

            FT_Error library_t::open_face(const FT_Open_Args *args, FT_Long face_index, FT_Face *aface)
            {
                return pOpen_Face(hLibrary, args, face_index, aface);
            }

            FT_Error library_t::done_face(FT_Face face)
            {
                return (face != NULL) ? pDone_Face(face) : FT_Err_Ok;
            }

            FT_Error library_t::reference_face(FT_Face face)
            {
                return pReference_Face(face);
            }

            void library_t::set_transform(FT_Face face, FT_Matrix *matrix, FT_Vector*delta)
            {
                return pSet_Transform(face, matrix, delta);
            }

            FT_Error library_t::load_glyph(FT_Face face, FT_UInt glyph_index, FT_Int32 load_flags)
            {
                return pLoad_Glyph(face, glyph_index, load_flags);
            }

            FT_Error library_t::outline_embolden(FT_Outline *outline, FT_Pos strength)
            {
                return pOutline_Embolden(outline, strength);
            }

            FT_Error library_t::bitmap_embolden(FT_Bitmap *bitmap, FT_Pos xStrength, FT_Pos yStrength)
            {
                return pBitmap_Embolden(hLibrary, bitmap, xStrength, yStrength);
            }

            FT_Error library_t::render_glyph(FT_GlyphSlot slot, FT_Render_Mode render_mode)
            {
                return pRender_Glyph(slot, render_mode);
            }

            FT_UInt library_t::get_char_index(FT_Face face, FT_ULong charcode)
            {
                return pGet_Char_Index(face, charcode);
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */
