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

#include <lsp-plug.in/ws/win/decode.h>

#ifdef PLATFORM_WINDOWS

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            size_t decode_mouse_keystate(size_t code)
            {
                BYTE kState[256];
                size_t result = 0;

                #define DC(mask, flag)  \
                    if (code & (mask)) \
                        result |= flag; \

                DC(MK_CONTROL, MCF_CONTROL);
                DC(MK_SHIFT, MCF_SHIFT);
                DC(MK_LBUTTON, MCF_LEFT);
                DC(MK_MBUTTON, MCF_MIDDLE);
                DC(MK_RBUTTON, MCF_RIGHT);
                DC(MK_XBUTTON1, MCF_BUTTON4);
                DC(MK_XBUTTON2, MCF_BUTTON5);

                #undef DC

                // Obtain the keyboard state
                if (GetKeyboardState(kState))
                {
                    if ((kState[VK_MENU] | kState[VK_LMENU] | kState[VK_RMENU]) & 0x80)
                        result     |= MCF_ALT;
                    if (kState[VK_CAPITAL])
                        result     |= MCF_LOCK;
                }

                return result;
            }

            size_t decode_kb_keystate(const BYTE *kState)
            {
                size_t result = 0;

                if ((kState[VK_CONTROL] | kState[VK_LCONTROL] | kState[VK_RCONTROL]) & 0x80)
                    result     |= MCF_CONTROL;
                if ((kState[VK_SHIFT] | kState[VK_LSHIFT] | kState[VK_RSHIFT]) & 0x80)
                    result     |= MCF_SHIFT;
                if ((kState[VK_MENU] | kState[VK_LMENU] | kState[VK_RMENU]) & 0x80)
                    result     |= MCF_ALT;
                if (kState[VK_CAPITAL])
                    result     |= MCF_LOCK;

                if (kState[VK_LBUTTON] & 0x80)
                    result     |= MCF_LEFT;
                if (kState[VK_MBUTTON] & 0x80)
                    result     |= MCF_MIDDLE;
                if (kState[VK_RBUTTON] & 0x80)
                    result     |= MCF_RIGHT;
                if (kState[VK_XBUTTON1] & 0x80)
                    result     |= MCF_BUTTON4;
                if (kState[VK_XBUTTON2] & 0x80)
                    result     |= MCF_BUTTON5;

                return result;
            }

            WPARAM encode_mouse_keystate(const BYTE *kState)
            {
                WPARAM result = 0;

                if ((kState[VK_CONTROL] | kState[VK_LCONTROL] | kState[VK_RCONTROL]) & 0x80)
                    result     |= MK_CONTROL;
                if ((kState[VK_SHIFT] | kState[VK_LSHIFT] | kState[VK_RSHIFT]) & 0x80)
                    result     |= MK_SHIFT;

                if (kState[VK_LBUTTON] & 0x80)
                    result     |= MK_LBUTTON;
                if (kState[VK_MBUTTON] & 0x80)
                    result     |= MK_MBUTTON;
                if (kState[VK_RBUTTON] & 0x80)
                    result     |= MK_RBUTTON;
                if (kState[VK_XBUTTON1] & 0x80)
                    result     |= MK_XBUTTON1;
                if (kState[VK_XBUTTON2] & 0x80)
                    result     |= MK_XBUTTON2;

                return result;
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */
