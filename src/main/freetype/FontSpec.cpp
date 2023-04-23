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

#include <private/freetype/FontSpec.h>
#include <private/freetype/face.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            size_t font_hash_iface::hash_func(const void *ptr, size_t /* size */)
            {
                const Font *f       = static_cast<const Font *>(ptr);
                size_t hash         = (f->name() != NULL) ? lltl::char_hash_func(f->name(), 0) : 0;
                size_t flags        = make_face_flags(f);
                hash                = ((hash << 8) | (hash >> ((sizeof(hash) - 1) * 8))) ^ flags;
                ssize_t size        = f->size() * 0x64;
                return hash ^ size;
            }

            ssize_t font_compare_iface::cmp_func(const void *a, const void *b, size_t /* size */)
            {
                const Font *fa  = static_cast<const Font *>(a);
                const Font *fb  = static_cast<const Font *>(b);

                if (fa->name() != fb->name())
                {
                    if (fa->name() == NULL)
                        return -1;
                    else if (fb->name() == NULL)
                        return 1;
                    ssize_t res         = strcmp(fa->name(), fb->name());
                    if (res != 0)
                        return res;
                }

                if (fa->size() < fb->size())
                    return -1;
                else if (fa->size() > fb->size())
                    return 1;

                ssize_t a_flags     = make_face_flags(fa);
                ssize_t b_flags     = make_face_flags(fb);
                return a_flags - b_flags;
            }

            void *font_allocator_iface::clone_func(const void *src, size_t size)
            {
                return new Font(static_cast<const Font *>(src));
            }

            void font_allocator_iface::free_func(void *ptr)
            {
                delete static_cast<Font *>(ptr);
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */
