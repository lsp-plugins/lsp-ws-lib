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

#ifndef PRIVATE_FREETYPE_FACE_ID_H_
#define PRIVATE_FREETYPE_FACE_ID_H_

#ifdef USE_LIBFREETYPE

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/ws/Font.h>
#include <lsp-plug.in/lltl/spec.h>

#include <private/freetype/types.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            enum face_id_flags_t
            {
                FID_SYNTHETIC   = 1 << 0,
                FID_ANTIALIAS   = 1 << 1,
                FID_BOLD        = 1 << 2,
                FID_ITALIC      = 1 << 3,

                FID_SHIFT       = 4
            };

            typedef struct face_id_t
            {
                const char *name;           // The name of Face ID
                f26p6_t     size;           // The size of the font face
                size_t      flags;          // The flags of Face ID
            } face_id_t;


            size_t          face_id_hash(const face_id_t *hash);
            face_id_t      *make_face_id(const char *name, f26p6_t size, size_t flags);
            void            free_face_id(face_id_t *id);
            size_t          make_face_id_flags(const Font *f);

        } /* namespace ft */
    } /* namespace ws */

    namespace lltl
    {
        template <>
        struct LSP_HIDDEN_MODIFIER hash_spec<ws::ft::face_id_t>: public hash_iface
        {
            static size_t hash_func(const void *ptr, size_t size);

            explicit inline hash_spec()
            {
                hash        = hash_func;
            }
        };

        template <>
        struct LSP_HIDDEN_MODIFIER compare_spec<ws::ft::face_id_t>: public lltl::compare_iface
        {
            static ssize_t cmp_func(const void *a, const void *b, size_t size);

            explicit inline compare_spec()
            {
                compare     = cmp_func;
            }
        };

        template <>
        struct LSP_HIDDEN_MODIFIER allocator_spec<ws::ft::face_id_t>: public lltl::allocator_iface
        {
            static void *clone_func(const void *src, size_t size);
            static void free_func(void *ptr);

            explicit inline allocator_spec()
            {
                clone       = clone_func;
                free        = free_func;
            }
        };
    } /* namespace lltl */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_FACE_ID_H_ */
