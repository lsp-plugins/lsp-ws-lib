/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 8 янв. 2026 г.
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

#include <lsp-plug.in/ws/IDrawable.h>

namespace lsp
{
    namespace ws
    {
        IDrawable::IDrawable()
        {
            atomic_store(&nReferences, 1);
        }

        IDrawable::~IDrawable()
        {
        }

        uatomic_t IDrawable::reference_up()
        {
            return atomic_add(&nReferences, 1) + 1;
        }

        uatomic_t IDrawable::reference_down()
        {
            uatomic_t result = atomic_add(&nReferences, -1) - 1;
            if (result == 0)
                delete this;
            return result;
        }

        bool IDrawable::valid() const
        {
            return false;
        }

        void IDrawable::invalidate()
        {
        }

        void *IDrawable::handle() const
        {
            return NULL;
        }

    } /* namespace ws */
} /* namespace lsp */


