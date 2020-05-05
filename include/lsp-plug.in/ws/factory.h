/*
 * factory.h
 *
 *  Created on: 5 мая 2020 г.
 *      Author: sadko
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
        LSP_CSYMBOL_EXTERN IDisplay *lsp_create_display(int argc, const char **argv);

        /**
         * Display finalization routine
         */
        LSP_CSYMBOL_EXTERN void lsp_free_display(IDisplay *dpy);
    }
}

#endif /* LSP_PLUG_IN_WS_FACTORY_H_ */
