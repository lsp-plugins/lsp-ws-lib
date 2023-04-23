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

#ifndef PRIVATE_FREETYPE_FONTSPEC_H_
#define PRIVATE_FREETYPE_FONTSPEC_H_

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
            struct font_hash_iface: public lltl::hash_iface
            {
                static size_t hash_func(const void *ptr, size_t size);

                explicit inline font_hash_iface()
                {
                    hash        = hash_func;
                }
            };

            struct font_compare_iface: public lltl::compare_iface
            {
                static ssize_t cmp_func(const void *a, const void *b, size_t size);

                explicit inline font_compare_iface()
                {
                    compare     = cmp_func;
                }
            };

            struct font_allocator_iface: public lltl::allocator_iface
            {
                static void *clone_func(const void *src, size_t size);
                static void free_func(void *ptr);

                explicit inline font_allocator_iface()
                {
                    clone       = clone_func;
                    free        = free_func;
                }
            };

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_FONTSPEC_H_ */
