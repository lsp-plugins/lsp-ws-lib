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

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/stdlib/string.h>

#include <private/win/util.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            //-----------------------------------------------------------------
            const char * const unicode_mimes[] =
            {
                "UTF8_STRING",
                "text/plain;charset=utf-8",
                "text/plain;charset=UTF-16LE",
                "text/plain;charset=UTF-16BE",
                NULL
            };

            const char * const ansi_mimes[] =
            {
                "text/plain;charset=US-ASCII",
                NULL
            };

            const char * const oem_mimes[] =
            {
                "text/plain",
                NULL
            };

            //-----------------------------------------------------------------
            ssize_t index_of_mime(const char *s, const char * const *mimes)
            {
                for (ssize_t idx = 0; (mimes != NULL) && (*mimes != NULL); ++mimes, ++idx)
                    if (!strcasecmp(s, *mimes))
                        return idx;
                return -1;
            }

            static status_t write_to_sink(IDataSink *dst, const void *data, size_t bytes)
            {
                if (data == NULL)
                    return dst->close(STATUS_NO_MEM);
                status_t res = dst->write(data, bytes);
                status_t res2 = dst->close(res);
                return (res != STATUS_OK) ? res : res2;
            }

            //-----------------------------------------------------------------
            status_t read_clipboard_format_name(UINT fmt, LSPString *dst)
            {
                ssize_t cap = 64;
                WCHAR *buf = static_cast<WCHAR *>(malloc(sizeof(WCHAR) * cap));
                if (buf == NULL)
                    return STATUS_NO_MEM;
                lsp_finally{ free(buf); };

                while (true)
                {
                    ssize_t res = GetClipboardFormatNameW(fmt, buf, cap);
                    if (res == 0)
                        return STATUS_UNKNOWN_ERR;
                    else if (res < (cap - 2))
                    {
                        while ((res > 0) && (buf[res-1] == '\0'))
                            --res;
                        if (!dst->set_utf16(buf, res))
                            return STATUS_NO_MEM;
                        return STATUS_OK;
                    }

                    // Grow the buffer size
                    cap <<= 1;
                    WCHAR *nbuf = static_cast<WCHAR *>(realloc(buf, sizeof(WCHAR) * cap));
                    if (buf == NULL)
                        return STATUS_NO_MEM;
                    buf         = nbuf;
                }
            }

            status_t sink_hglobal_contents(IDataSink *dst, HGLOBAL g, const char *mime)
            {
                ssize_t index;
                size_t n;
                LSPString tmp;

                // Analyze arguments
                if (dst == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if ((g == NULL) || (mime == NULL))
                    return dst->close(STATUS_BAD_ARGUMENTS);

                // Lock the global object
                size_t gsize = GlobalSize(g);
                void *ptr    = GlobalLock(g);
                if (ptr == NULL)
                    return dst->close(STATUS_NO_MEM);
                lsp_finally{ GlobalUnlock(g); };

                // Try unicode first
                if ((index = index_of_mime(mime, unicode_mimes)) >= 0)
                {
                    // Unicode string
                    WCHAR *s    = static_cast<WCHAR *>(ptr);
                    n           = wcsnlen(s, gsize);
                    if (!tmp.set_utf16(s, n))
                        return dst->close(STATUS_NO_MEM);
                    if (!tmp.to_unix())
                        return dst->close(STATUS_NO_MEM);

                    // Perform conversion
                    switch (index)
                    {
                        case 0:
                        case 1: // UTF-8
                        {
                            const char *xs  = tmp.get_utf8();
                            return write_to_sink(dst, xs, tmp.temporal_size());
                        }
                        case 2: // UTF-16LE
                        {
                            const lsp_utf16_t *xs = __IF_LEBE(
                                tmp.get_utf16(),
                                reinterpret_cast<const lsp_utf16_t *>(tmp.get_native("UTF-16LE"))
                            );
                            return write_to_sink(dst, xs, tmp.temporal_size());
                        }
                        case 3: // UTF-16BE
                        {
                            const lsp_utf16_t *xs = __IF_LEBE(
                                reinterpret_cast<const lsp_utf16_t *>(tmp.get_native("UTF-16BE")),
                                tmp.get_utf16()
                            );
                            return write_to_sink(dst, xs, tmp.temporal_size());
                        }
                        default:
                            break;
                    }

                    return dst->close(STATUS_BAD_FORMAT);
                }

                // Try OEM text
                if ((index = index_of_mime(mime, oem_mimes)) >= 0)
                {
                    // OEM string
                    char *s     = static_cast<char *>(ptr);
                    n           = strnlen(s, gsize);
                    if (!tmp.set_native(s, n))
                        return dst->close(STATUS_NO_MEM);
                    if (!tmp.to_unix())
                        return dst->close(STATUS_NO_MEM);

                    const char *xs  = tmp.get_native();
                    return write_to_sink(dst, xs, tmp.temporal_size());
                }

                // Try US-ASCII text
                if ((index = index_of_mime(mime, ansi_mimes)) >= 0)
                {
                    // ANSI string
                    char *s     = static_cast<char *>(ptr);
                    n           = strnlen(s, gsize);
                    if (!tmp.set_ascii(s, n))
                        return dst->close(STATUS_NO_MEM);
                    if (!tmp.to_unix())
                        return dst->close(STATUS_NO_MEM);

                    const char *xs  = tmp.get_ascii();
                    return write_to_sink(dst, xs, tmp.temporal_size());
                }

                // Write Custom format
                return write_to_sink(dst, ptr, gsize);
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


