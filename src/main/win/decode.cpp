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
                size_t result = 0;
                #define DC(mask, flag)  \
                    if (code & mask) \
                        result |= flag; \

                DC(MK_CONTROL, MCF_CONTROL);
                DC(MK_SHIFT, MCF_SHIFT);
                DC(MK_LBUTTON, MCF_LEFT);
                DC(MK_MBUTTON, MCF_MIDDLE);
                DC(MK_RBUTTON, MCF_RIGHT);
                DC(MK_XBUTTON1, MCF_BUTTON4);
                DC(MK_XBUTTON2, MCF_BUTTON5);

                return result;
            }
        }
    }
}

#endif /* PLATFORM_WINDOWS */
