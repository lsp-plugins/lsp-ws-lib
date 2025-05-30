/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 4 мая 2020 г.
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

#ifndef LSP_PLUG_IN_WS_TYPES_H_
#define LSP_PLUG_IN_WS_TYPES_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/io/IInStream.h>

namespace lsp
{
    namespace ws
    {
        typedef uint32_t            code_t;

        /** Mouse controller buttons
         *
         */
        enum mcb_t
        {
            MCB_LEFT            = 0,
            MCB_MIDDLE          = 1,
            MCB_RIGHT           = 2,
            MCB_BUTTON4         = 3,
            MCB_BUTTON5         = 4,
            MCB_BUTTON6         = 5,
            MCB_BUTTON7         = 6,

            MCB_NONE            = 0xffff
        };

        /** Mouse controller flags
         *
         */
        enum mcf_t
        {
            MCF_LEFT            = 1 << 0,
            MCF_MIDDLE          = 1 << 1,
            MCF_RIGHT           = 1 << 2,
            MCF_BUTTON4         = 1 << 3,
            MCF_BUTTON5         = 1 << 4,
            MCF_BUTTON6         = 1 << 5,
            MCF_BUTTON7         = 1 << 6,

            MCF_SHIFT           = 1 << 7,
            MCF_LOCK            = 1 << 8,
            MCF_CONTROL         = 1 << 9,

            MCF_ALT             = 1 << 10,
            MCF_MOD2            = 1 << 11,
            MCF_MOD3            = 1 << 12,
            MCF_MOD4            = 1 << 13,
            MCF_MOD5            = 1 << 14,

            MCF_SUPER           = 1 << 15,
            MCF_HYPER           = 1 << 16,
            MCF_META            = 1 << 17,
            MCF_RELEASE         = 1 << 18,

            MCF_BTN_MASK        = MCF_LEFT | MCF_MIDDLE | MCF_RIGHT | MCF_BUTTON4 | MCF_BUTTON5 | MCF_BUTTON6 | MCF_BUTTON7
        };

        /** Mouse scroll direction
         *
         */
        enum mcd_t
        {
            MCD_UP              = 0,
            MCD_DOWN            = 1,
            MCD_LEFT            = 2,
            MCD_RIGHT           = 3,

            MCD_NONE            = 0xffff
        };

        /**
         * Different grab group types,
         * sorted according to the priority of grab
         * in ascending order
         */
        enum grab_t
        {
            GRAB_LOWEST,
            GRAB_LOW,
            GRAB_NORMAL,
            GRAB_HIGH,
            GRAB_HIGHEST,

            GRAB_DROPDOWN,                  // Dropdown list

            GRAB_MENU,                      // Simple menu
            GRAB_EXTRA_MENU,                // Menu over menu

            __GRAB_TOTAL
        };

        /** Event processing flags
         *
         */
        enum event_flags_t
        {
            EVF_NONE            = 0,        // Nothing to do
            EVF_HANDLED         = 1 << 0,   // Event has been processed
            EVF_STOP            = 1 << 1,   // Stop further propagation of event to other elements
            EVF_GRAB            = 1 << 2    // Grab all further events first
        };

        /** Different drag actions
         *
         */
        enum drag_t
        {
            DRAG_COPY           = 0,
            DRAG_MOVE           = 1,
            DRAG_LINK           = 2
        };

        enum mouse_pointer_t
        {
            MP_DEFAULT,         // Default cursor
            MP_NONE,            // No cursor
            MP_ARROW,           // Standard arrow
            MP_ARROW_LEFT,      // Arrow left
            MP_ARROW_RIGHT,     // Arrow right
            MP_ARROW_UP,        // Arrow up
            MP_ARROW_DOWN,      // Arrow down
            MP_HAND,            // Hand pointer
            MP_CROSS,           // Crosshair
            MP_IBEAM,           // Text-editing I-beam
            MP_DRAW,            // Drawing tool (pencil)
            MP_PLUS,            // Plus
            MP_SIZE,            // Size
            MP_SIZE_NESW,       // Sizing cursor oriented diagonally from northeast to southwest
            MP_SIZE_NS,         // Sizing cursor oriented vertically
            MP_SIZE_WE,         // Sizing cursor oriented horizontally
            MP_SIZE_NWSE,       // Sizing cursor oriented diagonally from northwest to southeast
            MP_UP_ARROW,        // Arrow pointing up
            MP_HOURGLASS,       // Hourglass
            MP_DRAG,            // Arrow with a blank page in the lower-right corner
            MP_NO_DROP,         // Diagonal slash through a white circle
            MP_DANGER,          // Danger cursor
            MP_HSPLIT,          // Black double-vertical bar with arrows pointing right and left
            MP_VSPLIT,          // Black double-horizontal bar with arrows pointing up and down
            MP_MULTIDRAG,       // Arrow with three blank pages in the lower-right corner
            MP_APP_START,       // Arrow combined with an hourglass
            MP_HELP,            // Arrow next to a black question mark

            // Boundaries
            __MP_LAST       = MP_HELP,
            __MP_COUNT      = __MP_LAST + 1,

            // Aliases
            MP_TEXT         = MP_IBEAM,
            MP_VSIZE        = MP_SIZE_NS,
            MP_HSIZE        = MP_SIZE_WE,
            MP_WAIT         = MP_HOURGLASS,
            MP_ARROW_WAIT   = MP_APP_START,
            MP_HYPERLINK    = MP_HAND,
            MP_PENCIL       = MP_DRAW,
            MP_TABLE_CELL   = MP_PLUS
        };

        enum ui_event_type_t
        {
            UIE_UNKNOWN,
            // Keyboard events
            UIE_KEY_DOWN,
            UIE_KEY_UP,
            // Mouse events
            UIE_MOUSE_DOWN,
            UIE_MOUSE_UP,
            UIE_MOUSE_MOVE,
            UIE_MOUSE_SCROLL,
            UIE_MOUSE_CLICK,
            UIE_MOUSE_DBL_CLICK,
            UIE_MOUSE_TRI_CLICK,
            UIE_MOUSE_IN,
            UIE_MOUSE_OUT,
            // Window events
            UIE_REDRAW,
            UIE_RENDER,
            UIE_SIZE_REQUEST,
            UIE_RESIZE,                     // Window has been resized
            UIE_SHOW,                       // Window becomes visible
            UIE_HIDE,                       // Window becomes hidden
            UIE_STATE,                      // Window state changed (see window_state_t)
            UIE_CLOSE,                      // Window has been closed
            UIE_FOCUS_IN,                   // Window has been focused in
            UIE_FOCUS_OUT,                  // Window has been focused out
            // Drag&Drop events
            UIE_DRAG_ENTER,
            UIE_DRAG_LEAVE,
            UIE_DRAG_REQUEST,
            // Supplementary constants
            UIE_TOTAL,
            UIE_FIRST = UIE_KEY_DOWN,
            UIE_LAST = UIE_DRAG_REQUEST,
            UIE_END = UIE_UNKNOWN
        };

        enum border_style_t
        {
            BS_DIALOG,              // Not sizable; no minimize/maximize menu
            BS_SINGLE,              // Not sizable; minimize/maximize menu
            BS_NONE,                // Not sizable; no visible border line
            BS_POPUP,               // Popup menu window
            BS_COMBO,               // Combo box window
            BS_SIZEABLE,            // Sizeable window
            BS_DROPDOWN             // Dropdown menu window
        };

        /**
         * Allowed window actions
         */
        enum window_action_t
        {
            WA_MOVE         = 1 << 0,                                                                                                           /**< WA_MOVE */
            WA_RESIZE       = 1 << 1,                                                                                                           /**< WA_RESIZE */
            WA_MINIMIZE     = 1 << 2,                                                                                                           /**< WA_MINIMIZE */
            WA_MAXIMIZE     = 1 << 3,                                                                                                           /**< WA_MAXIMIZE */
            WA_CLOSE        = 1 << 4,                                                                                                           /**< WA_CLOSE */
            WA_STICK        = 1 << 5,                                                                                                           /**< WA_STICK */
            WA_SHADE        = 1 << 6,                                                                                                           /**< WA_SHADE */
            WA_FULLSCREEN   = 1 << 7,                                                                                                           /**< WA_FULLSCREEN */
            WA_CHANGE_DESK  = 1 << 8,                                                                                                           /**< WA_CHANGE_DESK */

            WA_ALL          = WA_MOVE | WA_RESIZE | WA_MINIMIZE | WA_MAXIMIZE | WA_CLOSE | WA_STICK | WA_SHADE | WA_FULLSCREEN | WA_CHANGE_DESK,/**< WA_ALL */
            WA_NONE         = 0,                                                                                                                /**< WA_NONE */
            WA_SINGLE       = WA_MOVE | WA_STICK | WA_MINIMIZE | WA_SHADE | WA_CHANGE_DESK | WA_CLOSE,                                          /**< WA_SINGLE */
            WA_DIALOG       = WA_MOVE | WA_STICK | WA_SHADE,                                                                                    /**< WA_DIALOG */
            WA_POPUP        = WA_NONE,                                                                                                          /**< WA_POPUP */
            WA_COMBO        = WA_NONE,                                                                                                          /**< WA_COMBO */
            WA_DROPDOWN     = WA_NONE,                                                                                                          /**< WA_DROPDOWN */
            WA_SIZABLE      = WA_ALL                                                                                                            /**< WA_SIZABLE */
        };

        /**
         * Window state
         */
        enum window_state_t
        {
            WS_NORMAL,
            WS_MINIMIZED,
            WS_MAXIMIZED
        };

        /**
         * Font flags as a bit mask
         */
        enum font_flags_t
        {
            /** Enable the bold font */
            FF_BOLD                     = 1 << 0,
            /** Enable the italic font */
            FF_ITALIC                   = 1 << 1,
            /** Enable the underline font */
            FF_UNDERLINE                = 1 << 2,

            /** Overall number of bits used by flags */
            FF_COUNT                    = 3,
            /** No flags are set */
            FF_NONE                     = 0,
            /** All flags are set */
            FF_ALL                      = FF_BOLD | FF_ITALIC | FF_UNDERLINE
        };

        /**
         * Font antialiasing settings
         */
        enum font_antialias_t
        {
            /** Use system settings for antialiasing */
            FA_DEFAULT,

            /** Force antialiasing to be disabled */
            FA_DISABLED,

            /** Force the antialiasing to be enabled */
            FA_ENABLED
        };

        typedef uint64_t    ui_timestamp_t;

        typedef struct event_t
        {
            size_t              nType;          // Type of event, see ui_event_type_t
            ssize_t             nLeft;          // Left position of something
            ssize_t             nTop;           // Top position of something
            ssize_t             nWidth;         // Width of something
            ssize_t             nHeight;        // Height of something
            code_t              nCode;          // Key code, button, scroll direction
            code_t              nRawCode;       // Raw code
            size_t              nState;         // State
            ui_timestamp_t      nTime;          // Event timestamp in milliseconds
        } event_t;

        typedef struct size_limit_t
        {
            ssize_t             nMinWidth;      // Minimum possible width in pixels
            ssize_t             nMinHeight;     // Minimum possible height in pixels
            ssize_t             nMaxWidth;      // Maximum possible width in pixels
            ssize_t             nMaxHeight;     // Maximum possible height in pixels
            ssize_t             nPreWidth;      // Preferrable width in pixels
            ssize_t             nPreHeight;     // Preferrable height in pixels
        } size_limit_t;

        typedef struct rectangle_t
        {
            ssize_t             nLeft;
            ssize_t             nTop;
            ssize_t             nWidth;
            ssize_t             nHeight;
        } rectangle_t;

        typedef struct point_t
        {
            ssize_t             nLeft;
            ssize_t             nTop;
        } point_t;

        enum surface_type_t
        {
            ST_UNKNOWN,         // Unknown surface type
            ST_IMAGE,           // Image surface
            ST_XLIB,            // Surface created by XLIB extension (X.11 Linux/FreeBSD)
            ST_SIMILAR,         // Similar surface to the parent
            ST_DDRAW,           // Surface created by Direct2D factory (Windows)
            ST_OPENGL,          // OpenGL surface
        };

        typedef struct font_parameters_t
        {
            float Ascent;       // The distance that the font extends above the baseline
            float Descent;      // The distance that the font extends below the baseline
            float Height;       // The recommended vertical distance between baselines when setting consecutive lines of text with the font
        } font_parameters_t;

        typedef struct text_parameters_t
        {
            float XBearing;     // The horizontal distance from the origin to the leftmost part of the glyphs as drawn
            float YBearing;     // The vertical distance from the origin to the topmost part of the glyphs as drawn
            float Width;        // Width of the glyphs as drawn
            float Height;       // Height of the glyphs as drawn
            float XAdvance;     // Distance to advance in the X direction after drawing these glyphs
            float YAdvance;     // distance to advance in the Y direction after drawing these glyphs
        } text_parameters_t;

        /** Corners to perform surface drawing
         *
         */
        enum corner_t
        {
            CORNER_LEFT_TOP         = 1 << 0,                                                                       //!< CORNER_LEFT_TOP
            CORNER_RIGHT_TOP        = 1 << 1,                                                                       //!< CORNER_RIGHT_TOP
            CORNER_LEFT_BOTTOM      = 1 << 2,                                                                       //!< CORNER_LEFT_BOTTOM
            CORNER_RIGHT_BOTTOM     = 1 << 3,                                                                       //!< CORNER_RIGHT_BOTTOM

            CORNERS_TOP             = CORNER_LEFT_TOP | CORNER_RIGHT_TOP,                                           //!< CORNERS_TOP
            CORNERS_BOTTOM          = CORNER_LEFT_BOTTOM | CORNER_RIGHT_BOTTOM,                                     //!< CORNERS_BOTTOM
            CORNERS_LEFT            = CORNER_LEFT_TOP | CORNER_LEFT_BOTTOM,                                         //!< CORNERS_LEFT
            CORNERS_RIGHT           = CORNER_RIGHT_TOP | CORNER_RIGHT_BOTTOM,                                       //!< CORNERS_RIGHT
            CORNERS_ALL             = CORNER_LEFT_TOP | CORNER_RIGHT_TOP | CORNER_LEFT_BOTTOM | CORNER_RIGHT_BOTTOM,//!< CORNERS_ALL
            CORNERS_NONE            = 0                                                                             //!< CORNERS_NONE
        };

        // Different kinds of clipboards
        enum clipboard_id_t
        {
            CBUF_PRIMARY,
            CBUF_SECONDARY,
            CBUF_CLIPBOARD,

            _CBUF_TOTAL
        };

        typedef struct clip_format_t
        {
            const char     *content_type;
            const char     *charset;
        } clip_format_t;

        /** timestamp type
         *
         */
        typedef uint64_t    timestamp_t;

        /** Task handler
         *
         * @param sched actual time at which the task has been scheduled
         * @param time current time at which the timer was executed
         * @param arg argument passed to task handler
         * @return status of operation
         */
        typedef status_t    (* task_handler_t)(timestamp_t sched, timestamp_t time, void *arg);

        /** Clipboard handler
         *
         * @param arg passed to the handler argument
         * @param s status of operation
         * @param is clipboard input stream object
         * @return status of operation
         */
        typedef status_t    (* clipboard_handler_t)(void *arg, status_t s, io::IInStream *is);

        /** Display task identifier
         *
         */
        typedef ssize_t     taskid_t;

        /**
         * Initialize empty event
         * @param ev event to initialize
         */
        LSP_WS_LIB_PUBLIC
        void                init_event(event_t *ev);

    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_TYPES_H_ */
