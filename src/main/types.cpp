/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 20 мая 2020 г.
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

#include <lsp-plug.in/ws/types.h>

namespace lsp
{
    namespace ws
    {
        LSP_WS_LIB_PUBLIC
        void init_event(event_t *ev)
        {
            ev->nType       = UIE_UNKNOWN;
            ev->nLeft       = 0;
            ev->nTop        = 0;
            ev->nWidth      = 0;
            ev->nHeight     = 0;
            ev->nCode       = 0;
            ev->nState      = 0;
            ev->nTime       = 0;
        }
    }
}


