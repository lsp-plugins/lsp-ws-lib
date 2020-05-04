/*
 * IEventHandler.h
 *
 *  Created on: 16 июн. 2017 г.
 *      Author: sadko
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
        class IEventHandler
        {
            private:
                IEventHandler & operator = (const IEventHandler &);

            public:
                explicit IEventHandler();
                virtual ~IEventHandler();

            public:
                /**
                 * Handle event
                 * @param ev event
                 * @return status of operation
                 */
                virtual status_t handle_event(const ws_event_t *ev);
        };
    } /* namespace tk */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_IEVENTHANDLER_H_ */
