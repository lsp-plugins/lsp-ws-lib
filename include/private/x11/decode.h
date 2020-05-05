/*
 * decode.h
 *
 *  Created on: 8 сент. 2017 г.
 *      Author: sadko
 */

#ifndef LSP_PLUG_IN_WS_X11_DECODE_H_
#define LSP_PLUG_IN_WS_X11_DECODE_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/common/types.h>

#ifdef USE_XLIB

#include <lsp-plug.in/ws/types.h>
#include <X11/Xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            mcb_t       decode_mcb(size_t code);

            mcd_t       decode_mcd(size_t code);

            size_t      decode_state(size_t code);

            ws_code_t   decode_keycode(KeySym code);
        }
    }
}

#endif /* USE_XLIB */

#endif /* LSP_PLUG_IN_WS_X11_DECODE_H_ */
