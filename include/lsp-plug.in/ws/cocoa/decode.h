/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *           (C) 2025 Marvin Edeler <marvin.edeler@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 16 June 2025
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

#ifndef LSP_PLUG_IN_WS_COCOA_DECODE_H_
#define LSP_PLUG_IN_WS_COCOA_DECODE_H_
 
#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/types.h>
 
#ifdef PLATFORM_MACOSX

#include <Cocoa/Cocoa.h>

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            mcb_t       decode_mcb(NSEvent *event);

            mcd_t       decode_mcd(NSEvent *event);

            size_t      decode_modifier(NSEvent *event);

            code_t      decode_keycode(unsigned long code);
        }
    }
}

#endif
#endif