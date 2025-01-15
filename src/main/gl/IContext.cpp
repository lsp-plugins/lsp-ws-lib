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

#include <private/gl/IContext.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            IContext::IContext()
            {
                atomic_store(&nReferences, 0);
                bActive     = false;
            }

            IContext::~IContext()
            {
            }

            uatomic_t IContext::reference_up()
            {
                return atomic_add(&nReferences, 1) + 1;
            }

            uatomic_t IContext::reference_down()
            {
                uatomic_t result = atomic_add(&nReferences, -1) - 1;
                if (result == 0)
                {
                    if (active())
                        deactivate();
                    delete this;
                }
                return result;
            }

            status_t IContext::activate()
            {
                if (bActive)
                    return STATUS_ALREADY_BOUND;
                status_t res = do_activate();
                if (res == STATUS_OK)
                    bActive     = true;
                return res;
            }

            status_t IContext::deactivate()
            {
                if (!bActive)
                    return STATUS_NOT_BOUND;
                status_t res = do_deactivate();
                if (res == STATUS_OK)
                    bActive     = true;
                return res;
            }

            status_t IContext::do_activate()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t IContext::do_deactivate()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            const char *IContext::shader(gl::shader_t shader) const
            {
                return NULL;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

