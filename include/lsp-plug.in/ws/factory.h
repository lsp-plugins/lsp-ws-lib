/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 5 мая 2020 г.
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

#ifndef LSP_PLUG_IN_WS_FACTORY_H_
#define LSP_PLUG_IN_WS_FACTORY_H_

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/IDisplay.h>

namespace lsp
{
    namespace ws
    {
        /**
         * Factory function prototype for creating display
         * @param argc number of arguments
         * @param argv list of arguments
         * @return pointer to the display or NULL
         */
        typedef IDisplay *(* display_factory_t)(int argc, const char **argv);

        /**
         * Display finalization routine
         */
        typedef void *(* display_finalizer_t)(IDisplay *dpy);

        /**
         * Display factory
         * @param argc number of arguments
         * @param argv list of arguments
         * @return pointer to the display or NULL, returned object must be destructed by free_display call
         */
        LSP_WS_LIB_CIMPORT
        IDisplay *lsp_ws_create_display(int argc, const char **argv);

        /**
         * Display finalization routine
         */
        LSP_WS_LIB_CIMPORT
        void lsp_ws_free_display(IDisplay *dpy);
    }
}

#endif /* LSP_PLUG_IN_WS_FACTORY_H_ */
