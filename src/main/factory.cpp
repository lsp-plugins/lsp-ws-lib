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

#include <lsp-plug.in/common/types.h>

#if defined(PLATFORM_WINDOWS)
    #include <private/win/WinDisplay.h>
#elif defined(USE_LIBX11)
    #include <private/x11/X11Display.h>
#endif /* PLATFORM_WINDOWS, USE_LIBX11 */

namespace lsp
{
    namespace ws
    {
        LSP_WS_LIB_CEXPORT
        IDisplay *lsp_ws_create_display(int argc, const char **argv)
        {
        #if defined(PLATFORM_WINDOWS)
            // Create Windows display
            {
                win::WinDisplay *dpy = new win::WinDisplay();
                if (dpy != NULL)
                {
                    status_t res = dpy->init(argc, argv);
                    if (res == STATUS_OK)
                        return dpy;
                    lsp_ws_free_display(dpy);
                }
            }
        #elif defined(USE_LIBX11)
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
        #endif /* PLATFORM_WINDOWS, USE_LIBX11 */
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

