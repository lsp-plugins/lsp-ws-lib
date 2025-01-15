/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 16 янв. 2025 г.
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

#include <private/gl/GLXContext.h>

#if defined(USE_LIBX11)

namespace lsp
{
    namespace ws
    {
        namespace glx
        {
            Context::Context(GLXContext ctx)
                : IContext()
            {
                hContext        = ctx;
            }

            Context::~Context()
            {
            }

            status_t Context::do_activate()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t Context::do_deactivate()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            const char *Context::shader(gl::shader_t shader) const
            {
                return NULL;
            }

        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */


