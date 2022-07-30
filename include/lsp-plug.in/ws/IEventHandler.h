/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 16 июн. 2017 г.
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

#ifndef LSP_PLUG_IN_WS_IEVENTHANDLER_H_
#define LSP_PLUG_IN_WS_IEVENTHANDLER_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/ws/types.h>

namespace lsp
{
    namespace ws
    {
        /** Windowing system's event handler
         *
         */
        class LSP_WS_LIB_PUBLIC IEventHandler
        {
            private:
                IEventHandler & operator = (const IEventHandler &);
                IEventHandler(const IEventHandler &);

            public:
                explicit IEventHandler();
                virtual ~IEventHandler();

            public:
                /**
                 * Handle event
                 * @param ev event
                 * @return status of operation
                 */
                virtual status_t handle_event(const event_t *ev);
        };
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_IEVENTHANDLER_H_ */
