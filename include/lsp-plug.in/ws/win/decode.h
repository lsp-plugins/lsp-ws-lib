/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 3 июл. 2022 г.
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

#ifndef LSP_PLUG_IN_WS_WIN_DECODE_H_
#define LSP_PLUG_IN_WS_WIN_DECODE_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/types.h>

#ifdef PLATFORM_WINDOWS

#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            /**
             * Decode the key state from the mouse event
             * @param code the key state from the mouse event
             * @return mask that indicates the pressure of some specific keys
             */
            size_t      decode_mouse_keystate(size_t code);

            /**
             * Obtain the current state of keys for the keyboard
             * @param kState current keyboard state
             * @return mask that indicates the pressure of some specific keys
             */
            size_t      decode_kb_keystate(const BYTE *kState);

            /**
             * Decode keyboard state as it is a mouse event keyboard state
             * @param kState keyboard state
             * @return encoded keyboard state
             */
            WPARAM      encode_mouse_keystate(const BYTE *kState);
        }
    }
}

#endif /* PLATFORM_WINDOWS */

#endif /* LSP_PLUG_IN_WS_WIN_DECODE_H_ */
