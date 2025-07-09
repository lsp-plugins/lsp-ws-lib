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

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_MACOSX

#include <lsp-plug.in/ws/cocoa/decode.h>
#include <Cocoa/Cocoa.h>

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            mcb_t decode_mcb(const NSEvent* event) 
            {
                const NSUInteger mouseButton = [event buttonNumber]; // Get mouse button index
                switch (mouseButton)
                {
                    case 0: return MCB_LEFT;
                    case 1: return MCB_RIGHT;
                    case 2: return MCB_MIDDLE;
                    default: break;
                }
                return MCB_NONE;
            }

            // TODO: do we need to map scroll direction set by user? macos scrolls inverted by default.
            mcd_t decode_mcd(const NSEvent* event) 
            {
                if ([event type] != NSEventTypeScrollWheel)
                    return MCD_NONE;
            
                const CGFloat dx   = [event scrollingDeltaX];
                const CGFloat dy   = [event scrollingDeltaY];
                const CGFloat adx   = fabs(dx);
            
                if (fabs(dy) > adx)
                    return (dy > 0) ? MCD_UP : MCD_DOWN;
                else if (adx > 0)
                    return (dx > 0) ? MCD_LEFT : MCD_RIGHT;
            
                return MCD_NONE;
            }

            size_t decode_modifier(const NSEvent* event)
            {
                const NSEventModifierFlags code = [event modifierFlags];
                const NSInteger mouseButton = [event buttonNumber];
                const NSEventType type = [event type];

                size_t result = 0;
                #define DC(mask, flag)  \
                    if (code & mask) \
                        result |= flag; \

                DC(NSEventModifierFlagShift,    MCF_SHIFT);
                DC(NSEventModifierFlagControl,  MCF_CONTROL);
                DC(NSEventModifierFlagOption,   MCF_ALT);
                DC(NSEventModifierFlagCommand,  MCF_META);
                DC(NSEventModifierFlagCapsLock, MCF_LOCK);
                //TODO: implement more flags?

                #undef DC

                switch (type)
                {
                    case NSEventTypeLeftMouseDown:
                    case NSEventTypeRightMouseDown:
                    case NSEventTypeOtherMouseDown:
                    {
                        switch (mouseButton)
                        {
                            case 0: result     |= MCF_LEFT; break;
                            case 1: result     |= MCF_RIGHT; break;
                            case 2: result     |= MCF_MIDDLE; break;
                            default: break;
                        }
                        break;
                    }
                    default:
                        break;
                }
                
                return result;
            }

            code_t decode_keycode(unsigned long code)
            {
                return code;
            }

        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_MACOSX */

