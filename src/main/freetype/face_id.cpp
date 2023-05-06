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

#include <private/freetype/face_id.h>
#include <private/freetype/face.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            LSP_HIDDEN_MODIFIER
            size_t face_id_hash(const face_id_t *face_id)
            {
                size_t hash = (face_id->name != NULL) ? lltl::char_hash_func(face_id->name, 0) : 0;
                size_t size = face_id->size;
                size_t extra= (size << FID_SHIFT) + (size >> 6)+ (size >> 1) + face_id->flags;
                return hash ^ extra;
            }

            LSP_HIDDEN_MODIFIER
            face_id_t *make_face_id(const char *name, f26p6_t size, size_t flags)
            {
                size_t len      = strlen(name) + 1;
                size_t szof     = align_size(sizeof(face_id_t), DEFAULT_ALIGN);
                size_t str_size = align_size(len, DEFAULT_ALIGN);

                uint8_t *ptr    = static_cast<uint8_t *>(malloc(szof + str_size));
                if (ptr == NULL)
                    return NULL;

                face_id_t *id   = reinterpret_cast<face_id_t *>(ptr);
                id->name        = reinterpret_cast<const char *>(&ptr[szof]);
                id->size        = size;
                id->flags       = flags;
                memcpy(&ptr[szof], name, len);

                return id;
            }

            LSP_HIDDEN_MODIFIER
            void free_face_id(face_id_t *id)
            {
                if (id != NULL)
                    free(id);
            }

            LSP_HIDDEN_MODIFIER
            size_t make_face_id_flags(const Font *f)
            {
                size_t flags    = (f->bold()) ? FID_BOLD : 0;
                if (f->italic())
                    flags          |= FID_ITALIC;
                if (f->antialias() != FA_DISABLED)
                    flags          |= FID_ANTIALIAS;
                return flags;
            }

        } /* namespace ft */
    } /* namespace ws */

    namespace lltl
    {
        using namespace ::lsp::ws::ft;

        size_t hash_spec<ws::ft::face_id_t>::hash_func(const void *ptr, size_t /* size */)
        {
            const face_id_t *f  = static_cast<const face_id_t *>(ptr);
            return face_id_hash(f);
        }

        ssize_t compare_spec<ws::ft::face_id_t>::cmp_func(const void *a, const void *b, size_t /* size */)
        {
            const face_id_t *fa  = static_cast<const face_id_t *>(a);
            const face_id_t *fb  = static_cast<const face_id_t *>(b);

            if (fa->name != fb->name)
            {
                if (fa->name == NULL)
                    return -1;
                else if (fb->name == NULL)
                    return 1;
                ssize_t res         = strcmp(fa->name, fb->name);
                if (res != 0)
                    return res;
            }

            ssize_t delta = ssize_t(fa->size) - ssize_t(fb->size);
            return (delta != 0) ? delta : ssize_t(fa->flags) - ssize_t(fb->flags);
        }

        void *allocator_spec<ws::ft::face_id_t>::clone_func(const void *src, size_t size)
        {
            const face_id_t *f  = static_cast<const face_id_t *>(src);
            return make_face_id(f->name, f->size, f->flags);
        }

        void allocator_spec<ws::ft::face_id_t>::free_func(void *ptr)
        {
            free_face_id(static_cast<face_id_t *>(ptr));
        }
    }

} /* namespace lsp */

#endif /* USE_LIBFREETYPE */
