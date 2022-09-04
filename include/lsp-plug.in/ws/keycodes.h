/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 8 сент. 2017 г.
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

#ifndef LSP_PLUG_IN_WS_KEYCODES_H_
#define LSP_PLUG_IN_WS_KEYCODES_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/types.h>

namespace lsp
{
    namespace ws
    {
        enum keycode_t
        {
            WSK_KEYSETS_FIRST           = 0x80000000,

            // Function keys
            WSK_KEYSET_FUNCTION_FIRST   = WSK_KEYSETS_FIRST + 0,
            WSK_BACKSPACE               = WSK_KEYSET_FUNCTION_FIRST + 0,
            WSK_TAB                     = WSK_KEYSET_FUNCTION_FIRST + 1,
            WSK_LINEFEED                = WSK_KEYSET_FUNCTION_FIRST + 2,
            WSK_CLEAR                   = WSK_KEYSET_FUNCTION_FIRST + 3,
            WSK_RETURN                  = WSK_KEYSET_FUNCTION_FIRST + 4,
            WSK_PAUSE                   = WSK_KEYSET_FUNCTION_FIRST + 5,
            WSK_SCROLL_LOCK             = WSK_KEYSET_FUNCTION_FIRST + 6,
            WSK_SYS_REQ                 = WSK_KEYSET_FUNCTION_FIRST + 7,
            WSK_ESCAPE                  = WSK_KEYSET_FUNCTION_FIRST + 8,
            WSK_DELETE                  = WSK_KEYSET_FUNCTION_FIRST + 9,
            WSK_KEYSET_FUNCTION_LAST    = WSK_DELETE,

            // Cursor control & motion
            WSK_KEYSET_CONTROL_FIRST    = WSK_KEYSETS_FIRST + 10,
            WSK_HOME                    = WSK_KEYSET_CONTROL_FIRST + 0,
            WSK_END                     = WSK_KEYSET_CONTROL_FIRST + 1,
            WSK_LEFT                    = WSK_KEYSET_CONTROL_FIRST + 2,
            WSK_RIGHT                   = WSK_KEYSET_CONTROL_FIRST + 3,
            WSK_UP                      = WSK_KEYSET_CONTROL_FIRST + 4,
            WSK_DOWN                    = WSK_KEYSET_CONTROL_FIRST + 5,
            WSK_PAGE_UP                 = WSK_KEYSET_CONTROL_FIRST + 6,
            WSK_PAGE_DOWN               = WSK_KEYSET_CONTROL_FIRST + 7,
            WSK_BEGIN                   = WSK_KEYSET_CONTROL_FIRST + 8,
            WSK_SELECT                  = WSK_KEYSET_CONTROL_FIRST + 9,
            WSK_PRINT                   = WSK_KEYSET_CONTROL_FIRST + 10,
            WSK_EXECUTE                 = WSK_KEYSET_CONTROL_FIRST + 11,
            WSK_INSERT                  = WSK_KEYSET_CONTROL_FIRST + 12,
            WSK_UNDO                    = WSK_KEYSET_CONTROL_FIRST + 13,
            WSK_REDO                    = WSK_KEYSET_CONTROL_FIRST + 14,
            WSK_MENU                    = WSK_KEYSET_CONTROL_FIRST + 15,
            WSK_FIND                    = WSK_KEYSET_CONTROL_FIRST + 16,
            WSK_CANCEL                  = WSK_KEYSET_CONTROL_FIRST + 17,
            WSK_HELP                    = WSK_KEYSET_CONTROL_FIRST + 18,
            WSK_BREAK                   = WSK_KEYSET_CONTROL_FIRST + 19,
            WSK_MODE_SWITCH             = WSK_KEYSET_CONTROL_FIRST + 20,
            WSK_NUM_LOCK                = WSK_KEYSET_CONTROL_FIRST + 21,

            WSK_PRIOR                   = WSK_PAGE_UP,
            WSK_NEXT                    = WSK_PAGE_DOWN,
            WSK_SCRIPT_SWITCH           = WSK_MODE_SWITCH,
            WSK_KEYSET_CONTROL_LAST     = WSK_NUM_LOCK,

            // Keypad keys
            WSK_KEYSET_KEYPAD_FIRST     = WSK_KEYSETS_FIRST + 30,
            WSK_KEYPAD_SPACE            = WSK_KEYSET_KEYPAD_FIRST + 0,
            WSK_KEYPAD_TAB              = WSK_KEYSET_KEYPAD_FIRST + 1,
            WSK_KEYPAD_ENTER            = WSK_KEYSET_KEYPAD_FIRST + 2,
            WSK_KEYPAD_F1               = WSK_KEYSET_KEYPAD_FIRST + 3,
            WSK_KEYPAD_F2               = WSK_KEYSET_KEYPAD_FIRST + 4,
            WSK_KEYPAD_F3               = WSK_KEYSET_KEYPAD_FIRST + 5,
            WSK_KEYPAD_F4               = WSK_KEYSET_KEYPAD_FIRST + 6,
            WSK_KEYPAD_HOME             = WSK_KEYSET_KEYPAD_FIRST + 7,
            WSK_KEYPAD_LEFT             = WSK_KEYSET_KEYPAD_FIRST + 8,
            WSK_KEYPAD_UP               = WSK_KEYSET_KEYPAD_FIRST + 9,
            WSK_KEYPAD_RIGHT            = WSK_KEYSET_KEYPAD_FIRST + 10,
            WSK_KEYPAD_DOWN             = WSK_KEYSET_KEYPAD_FIRST + 11,
            WSK_KEYPAD_PAGE_UP          = WSK_KEYSET_KEYPAD_FIRST + 12,
            WSK_KEYPAD_PAGE_DOWN        = WSK_KEYSET_KEYPAD_FIRST + 13,
            WSK_KEYPAD_END              = WSK_KEYSET_KEYPAD_FIRST + 14,
            WSK_KEYPAD_BEGIN            = WSK_KEYSET_KEYPAD_FIRST + 15,
            WSK_KEYPAD_INSERT           = WSK_KEYSET_KEYPAD_FIRST + 16,
            WSK_KEYPAD_DELETE           = WSK_KEYSET_KEYPAD_FIRST + 17,
            WSK_KEYPAD_EQUAL            = WSK_KEYSET_KEYPAD_FIRST + 18,
            WSK_KEYPAD_MULTIPLY         = WSK_KEYSET_KEYPAD_FIRST + 19,
            WSK_KEYPAD_ADD              = WSK_KEYSET_KEYPAD_FIRST + 20,
            WSK_KEYPAD_SEPARATOR        = WSK_KEYSET_KEYPAD_FIRST + 21,
            WSK_KEYPAD_SUBTRACT         = WSK_KEYSET_KEYPAD_FIRST + 22,
            WSK_KEYPAD_DECIMAL          = WSK_KEYSET_KEYPAD_FIRST + 23,
            WSK_KEYPAD_DIVIDE           = WSK_KEYSET_KEYPAD_FIRST + 24,

            WSK_KEYPAD_0                = WSK_KEYSET_KEYPAD_FIRST + 25,
            WSK_KEYPAD_1                = WSK_KEYSET_KEYPAD_FIRST + 26,
            WSK_KEYPAD_2                = WSK_KEYSET_KEYPAD_FIRST + 27,
            WSK_KEYPAD_3                = WSK_KEYSET_KEYPAD_FIRST + 28,
            WSK_KEYPAD_4                = WSK_KEYSET_KEYPAD_FIRST + 29,
            WSK_KEYPAD_5                = WSK_KEYSET_KEYPAD_FIRST + 30,
            WSK_KEYPAD_6                = WSK_KEYSET_KEYPAD_FIRST + 31,
            WSK_KEYPAD_7                = WSK_KEYSET_KEYPAD_FIRST + 32,
            WSK_KEYPAD_8                = WSK_KEYSET_KEYPAD_FIRST + 33,
            WSK_KEYPAD_9                = WSK_KEYSET_KEYPAD_FIRST + 34,

            WSK_KEYPAD_PRIOR            = WSK_KEYPAD_PAGE_UP,
            WSK_KEYPAD_NEXT             = WSK_KEYPAD_PAGE_DOWN,
            WSK_KEYSET_KEYPAD_LAST      = WSK_KEYPAD_9,

            // Auxiliary functions
            WSK_KEYSET_AUX_FIRST        = WSK_KEYSETS_FIRST + 70,
            WSK_F1                      = WSK_KEYSET_AUX_FIRST + 0,
            WSK_F2                      = WSK_KEYSET_AUX_FIRST + 1,
            WSK_F3                      = WSK_KEYSET_AUX_FIRST + 2,
            WSK_F4                      = WSK_KEYSET_AUX_FIRST + 3,
            WSK_F5                      = WSK_KEYSET_AUX_FIRST + 4,
            WSK_F6                      = WSK_KEYSET_AUX_FIRST + 5,
            WSK_F7                      = WSK_KEYSET_AUX_FIRST + 6,
            WSK_F8                      = WSK_KEYSET_AUX_FIRST + 7,
            WSK_F9                      = WSK_KEYSET_AUX_FIRST + 8,
            WSK_F10                     = WSK_KEYSET_AUX_FIRST + 9,
            WSK_F11                     = WSK_KEYSET_AUX_FIRST + 10,
            WSK_F12                     = WSK_KEYSET_AUX_FIRST + 11,
            WSK_F13                     = WSK_KEYSET_AUX_FIRST + 12,
            WSK_F14                     = WSK_KEYSET_AUX_FIRST + 13,
            WSK_F15                     = WSK_KEYSET_AUX_FIRST + 14,
            WSK_F16                     = WSK_KEYSET_AUX_FIRST + 15,
            WSK_F17                     = WSK_KEYSET_AUX_FIRST + 16,
            WSK_F18                     = WSK_KEYSET_AUX_FIRST + 17,
            WSK_F19                     = WSK_KEYSET_AUX_FIRST + 18,
            WSK_F20                     = WSK_KEYSET_AUX_FIRST + 19,
            WSK_F21                     = WSK_KEYSET_AUX_FIRST + 20,
            WSK_F22                     = WSK_KEYSET_AUX_FIRST + 21,
            WSK_F23                     = WSK_KEYSET_AUX_FIRST + 22,
            WSK_F24                     = WSK_KEYSET_AUX_FIRST + 23,
            WSK_F25                     = WSK_KEYSET_AUX_FIRST + 24,
            WSK_F26                     = WSK_KEYSET_AUX_FIRST + 25,
            WSK_F27                     = WSK_KEYSET_AUX_FIRST + 26,
            WSK_F28                     = WSK_KEYSET_AUX_FIRST + 27,
            WSK_F29                     = WSK_KEYSET_AUX_FIRST + 28,
            WSK_F30                     = WSK_KEYSET_AUX_FIRST + 29,
            WSK_F31                     = WSK_KEYSET_AUX_FIRST + 30,
            WSK_F32                     = WSK_KEYSET_AUX_FIRST + 31,
            WSK_F33                     = WSK_KEYSET_AUX_FIRST + 32,
            WSK_F34                     = WSK_KEYSET_AUX_FIRST + 33,
            WSK_F35                     = WSK_KEYSET_AUX_FIRST + 34,

            WSK_L1                      = WSK_F11,
            WSK_L2                      = WSK_F12,
            WSK_L3                      = WSK_F13,
            WSK_L4                      = WSK_F14,
            WSK_L5                      = WSK_F15,
            WSK_L6                      = WSK_F16,
            WSK_L7                      = WSK_F17,
            WSK_L8                      = WSK_F18,
            WSK_L9                      = WSK_F19,
            WSK_L10                     = WSK_F20,

            WSK_R1                      = WSK_F21,
            WSK_R2                      = WSK_F22,
            WSK_R3                      = WSK_F23,
            WSK_R4                      = WSK_F24,
            WSK_R5                      = WSK_F25,
            WSK_R6                      = WSK_F26,
            WSK_R7                      = WSK_F27,
            WSK_R8                      = WSK_F28,
            WSK_R9                      = WSK_F29,
            WSK_R10                     = WSK_F30,
            WSK_R11                     = WSK_F31,
            WSK_R12                     = WSK_F32,
            WSK_R13                     = WSK_F33,
            WSK_R14                     = WSK_F34,
            WSK_R15                     = WSK_F35,

            WSK_KEYSET_AUX_LAST         = WSK_F35,

            // Modifiers
            WSK_KEYSET_MODIFIERS_FIRST  = WSK_KEYSETS_FIRST + 110,
            WSK_SHIFT_L                 = WSK_KEYSET_MODIFIERS_FIRST + 0,
            WSK_SHIFT_R                 = WSK_KEYSET_MODIFIERS_FIRST + 1,
            WSK_CONTROL_L               = WSK_KEYSET_MODIFIERS_FIRST + 2,
            WSK_CONTROL_R               = WSK_KEYSET_MODIFIERS_FIRST + 3,
            WSK_CAPS_LOCK               = WSK_KEYSET_MODIFIERS_FIRST + 4,
            WSK_SHIFT_LOCK              = WSK_KEYSET_MODIFIERS_FIRST + 5,

            WSK_META_L                  = WSK_KEYSET_MODIFIERS_FIRST + 6,
            WSK_META_R                  = WSK_KEYSET_MODIFIERS_FIRST + 7,
            WSK_ALT_L                   = WSK_KEYSET_MODIFIERS_FIRST + 8,
            WSK_ALT_R                   = WSK_KEYSET_MODIFIERS_FIRST + 9,
            WSK_SUPER_L                 = WSK_KEYSET_MODIFIERS_FIRST + 10,
            WSK_SUPER_R                 = WSK_KEYSET_MODIFIERS_FIRST + 11,
            WSK_HYPER_L                 = WSK_KEYSET_MODIFIERS_FIRST + 12,
            WSK_HYPER_R                 = WSK_KEYSET_MODIFIERS_FIRST + 13,

            WSK_KEYSET_MODIFIERS_LAST   = WSK_HYPER_R,

            WSK_KEYSETS_LAST            = 0x800000ff,

            WSK_UNKNOWN                 = 0xffffffff
        };

        constexpr inline bool is_character_key(code_t key)
        {
            return key < WSK_KEYSETS_FIRST;
        }

        constexpr inline bool is_special_key(code_t key)
        {
            return (key >= WSK_KEYSETS_FIRST) && (key <= WSK_KEYSETS_LAST);
        }

        constexpr inline bool is_unknown_key(code_t key)
        {
            return (key > WSK_KEYSETS_LAST);
        }

        constexpr inline bool is_function_key(code_t key)
        {
            return (key >= WSK_KEYSET_FUNCTION_FIRST) && (key <= WSK_KEYSET_FUNCTION_LAST);
        }

        constexpr inline bool is_control_key(code_t key)
        {
            return (key >= WSK_KEYSET_CONTROL_FIRST) && (key <= WSK_KEYSET_CONTROL_LAST);
        }

        constexpr inline bool is_keypad_key(code_t key)
        {
            return (key >= WSK_KEYSET_KEYPAD_FIRST) && (key <= WSK_KEYSET_KEYPAD_LAST);
        }

        constexpr inline bool is_aux_key(code_t key)
        {
            return (key >= WSK_KEYSET_AUX_FIRST) && (key <= WSK_KEYSET_AUX_LAST);
        }

        constexpr inline bool is_modifier_key(code_t key)
        {
            return (key >= WSK_KEYSET_MODIFIERS_FIRST) && (key <= WSK_KEYSET_MODIFIERS_LAST);
        }
    }
}

#endif /* LSP_PLUG_IN_WS_KEYCODES_H_ */
