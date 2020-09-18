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

#include <lsp-plug.in/ws/factory.h>

#ifdef USE_XLIB
    #include <private/x11/X11Display.h>
#endif /* USE_XLIB */

namespace lsp
{
    namespace ws
    {
        LSP_WS_LIB_CEXPORT
        IDisplay *lsp_ws_create_display(int argc, const char **argv)
        {
        #ifdef USE_XLIB
            // Create X11 display
            {
                x11::X11Display *dpy = new x11::X11Display();
                if (dpy != NULL)
                {
                    status_t res = dpy->init(argc, argv);
                    if (res == STATUS_OK)
                        return dpy;
                    lsp_ws_free_display(dpy);
                }
            }
        #endif /* USE_XLIB */
            return NULL;
        }

        LSP_WS_LIB_CEXPORT
        void lsp_ws_free_display(IDisplay *dpy)
        {
            if (dpy == NULL)
                return;
            dpy->destroy();
            delete dpy;
        }
    }
}

