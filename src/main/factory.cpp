/*
 * factory.cpp
 *
 *  Created on: 5 мая 2020 г.
 *      Author: sadko
 */

#include <lsp-plug.in/ws/factory.h>

#ifdef USE_XLIB
    #include <private/x11/X11Display.h>
#endif /* USE_XLIB */

namespace lsp
{
    namespace ws
    {
        LSP_CSYMBOL_EXPORT IDisplay *lsp_create_display(int argc, const char **argv)
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
                    lsp_free_display(dpy);
                }
            }
        #endif /* USE_XLIB */
            return NULL;
        }

        LSP_CSYMBOL_EXPORT void lsp_free_display(IDisplay *dpy)
        {
            if (dpy == NULL)
                return;
            dpy->destroy();
            delete dpy;
        }
    }
}

