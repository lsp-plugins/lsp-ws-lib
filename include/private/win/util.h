/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 23 июл. 2022 г.
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

#ifndef PRIVATE_WIN_UTIL_H_
#define PRIVATE_WIN_UTIL_H_

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/ws/IDataSink.h>

#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            extern const char * const unicode_mimes[];
            extern const char * const ansi_mimes[];
            extern const char * const oem_mimes[];

            /**
             * Return the index of the MIME in the NULL-terminated list
             * @param s name of the mime
             * @param mimes list of the mimes
             * @return index of the mime in the list or negative value if not present
             */
            ssize_t     index_of_mime(const char *s, const char * const *mimes);

            /**
             * Read the name of clipboard format to the string
             * @param fmt the clipboard format
             * @param dst destination string to store data
             * @return status of operation
             */
            status_t    read_clipboard_format_name(UINT fmt, LSPString *dst);

            /**
             * Read the contents of HGLOBAL variable and send it to the sink
             * @param dst destination sink to send data
             * @param g HGLOBAL object
             * @param mime the name of the MIME format
             * @return status of operation
             */
            status_t    sink_hglobal_contents(IDataSink *dst, HGLOBAL g, const char *mime);
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_UTIL_H_ */
