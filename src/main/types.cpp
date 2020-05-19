/*
 * types.cpp
 *
 *  Created on: 20 мая 2020 г.
 *      Author: sadko
 */

#include <lsp-plug.in/ws/types.h>

namespace lsp
{
    namespace ws
    {
        void init_event(event_t *ev)
        {
            ev->nType       = 0;
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


