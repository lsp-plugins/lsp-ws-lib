/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 22 июл. 2022 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/ws/types.h>

#include <private/win/WinWindow.h>
#include <private/win/dnd.h>
#include <private/win/util.h>

#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            LSP_IUNKNOWN_IMPL(WinDNDTarget, IDropTarget)

            WinDNDTarget::WinDNDTarget(WinWindow *wnd)
            {
                nRefCount       = 0;
                pWindow         = wnd;

                pDataSink       = NULL;
                enAction        = DRAG_COPY;
                bInternal       = false;
                bPollActive     = false;
                bRejected       = false;
                sRect.nLeft     = 0;
                sRect.nTop      = 0;
                sRect.nWidth    = 0;
                sRect.nHeight   = 0;
            }

            WinDNDTarget::~WinDNDTarget()
            {
                release_resources();
            }

            void WinDNDTarget::release_resources()
            {
                // Reset confirmation state
                reset_confirm_state();

                // Drop format names, formats and mapping
                vFormatNames.flush();
                vFormats.flush();
                vFormatMapping.flush();
            }

            void WinDNDTarget::reset_confirm_state()
            {
                // Release previously used data sink
                if (pDataSink != NULL)
                {
                    pDataSink->release();
                    pDataSink   = NULL;
                }

                // Reset confirmation state
                enAction        = DRAG_COPY;
                bInternal       = false;
                sRect.nLeft     = 0;
                sRect.nTop      = 0;
                sRect.nWidth    = 0;
                sRect.nHeight   = 0;
            }

            bool WinDNDTarget::poll_active() const
            {
                // If pDataSink is not null, then drag has been accepted.
                // If bRejected is false, then no explicit reject_drag() was called.
                return (bPollActive) && (pDataSink != NULL) && (!bRejected);
            }

            const char *const *WinDNDTarget::formats() const
            {
                return (vFormatNames.size() > 0) ? vFormatNames.array() : NULL;
            }

            status_t WinDNDTarget::accept_drag(IDataSink *sink, drag_t action, const rectangle_t *r)
            {
                if (sink == NULL)
                    return STATUS_BAD_ARGUMENTS;

                // Release previously used data sink
                if (pDataSink != NULL)
                {
                    pDataSink->release();
                    pDataSink   = NULL;
                }

                // Update drag confirmation
                sink->acquire();
                pDataSink       = sink;
                enAction        = action;
                bInternal       = r != NULL;
                if (bInternal)
                    sRect           = *r;

                return STATUS_OK;
            }

            status_t WinDNDTarget::reject_drag()
            {
                bRejected       = true;
                reset_confirm_state();
                return STATUS_OK;
            }

            DWORD WinDNDTarget::get_drop_effect()
            {
                if ((pDataSink == NULL) || (bRejected))
                    return DROPEFFECT_NONE;

                switch (enAction)
                {
                    case DRAG_COPY: return DROPEFFECT_COPY;
                    case DRAG_MOVE: return DROPEFFECT_MOVE;
                    case DRAG_LINK: return DROPEFFECT_LINK;
                }
                return DROPEFFECT_NONE;
            }

            void WinDNDTarget::create_builtin_format_mapping(FORMATETC *fmt, const char * const * mimes)
            {
                for (; (mimes != NULL) && (*mimes != NULL); ++mimes)
                {
                    FORMATETC *xfmt = vFormats.add(fmt);
                    if (xfmt == NULL)
                        continue;
                    if (!vFormatMapping.create(*mimes, xfmt))
                        continue;
                    vFormatNames.add(const_cast<char *>(vFormatMapping.key(*mimes)));
                }
            }

            void WinDNDTarget::create_custom_format_mapping(FORMATETC *fmt, const char *name)
            {
                if (name == NULL)
                    return;
                FORMATETC *xfmt = vFormats.add(fmt);
                if (xfmt == NULL)
                    return;
                if (!vFormatMapping.create(name, xfmt))
                    return;
                vFormatNames.add(const_cast<char *>(vFormatMapping.key(name)));
            }

            bool WinDNDTarget::read_formats(IDataObject *pDataObj)
            {
                HRESULT hr;
                FORMATETC tmp;
                ULONG fetched;
                LSPString fmt_name;

                // Obtain format enumerator
                IEnumFORMATETC *pFmtEnum = NULL;
                hr = pDataObj->EnumFormatEtc(DATADIR_GET, &pFmtEnum);
                if ((FAILED(hr)) || (pFmtEnum == NULL))
                    return E_FAIL;
                lsp_finally { safe_release(pFmtEnum); };

                // Enumerate formats
                while ((hr = pFmtEnum->Next(1, &tmp, &fetched)) == S_OK)
                {
                    if (tmp.tymed != TYMED_HGLOBAL)
                        continue;

                    switch (tmp.cfFormat)
                    {
                        case CF_UNICODETEXT:
                            create_builtin_format_mapping(&tmp, unicode_mimes);
                            break;
                        case CF_OEMTEXT:
                            create_builtin_format_mapping(&tmp, oem_mimes);
                            break;
                        case CF_TEXT:
                            create_builtin_format_mapping(&tmp, ansi_mimes);
                            break;
                        case CF_HDROP:
                            create_builtin_format_mapping(&tmp, text_uri_list_mimes);
                            break;
                        default:
                            // Read name of the clipboard format
                            if (read_clipboard_format_name(tmp.cfFormat, &fmt_name) != STATUS_OK)
                                break;
                            if (!fmt_name.prepend_ascii("application/x-windows-"))
                                break;
                            lsp_trace("FMT: fmt=%d, asp=%ld, lind=%ld, tymed=0x%lx name=%s",
                                int(tmp.cfFormat), long(tmp.dwAspect), long(tmp.lindex), long(tmp.tymed), fmt_name.get_native());
                            create_custom_format_mapping(&tmp, fmt_name.get_utf8());
                            break;
                    }
                }

                // Add terminator
                if (!vFormatNames.add(static_cast<char *>(NULL)))
                    return false;

                return hr == S_FALSE;
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
            {
                // Validate arguments
                if ((pDataObj == NULL) || (pdwEffect == NULL))
                    return E_INVALIDARG;

                // Release previously used resources
                release_resources();

                // Read formats provided by data object
                if (!read_formats(pDataObj))
                {
                    release_resources();
                    return E_FAIL;
                }

                // Create DRAG_ENTER event and pass to window
                event_t ue;
                init_event(&ue);
                ue.nType        = UIE_DRAG_ENTER;
                pWindow->handle_event(&ue);

                return DragOver(grfKeyState, pt, pdwEffect);
            }

            void WinDNDTarget::translate_point(POINT & dpt, const POINTL & pt)
            {
                dpt.x           = pt.x;
                dpt.y           = pt.y;
                ScreenToClient(pWindow->win_handle(), &dpt);
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
            {
                if (pdwEffect == NULL)
                    return E_INVALIDARG;

                // Reset state, prepare for accept_drag() or reject_drag() calls
                reset_confirm_state();
                bPollActive     = true;
                bRejected       = false;

                // Create DRAG_REQUEST event and pass to window
                POINT dpt;
                translate_point(dpt, pt);

                event_t ue;
                init_event(&ue);
                ue.nType        = UIE_DRAG_REQUEST;
                ue.nState       = DRAG_COPY;
                ue.nLeft        = dpt.x;
                ue.nTop         = dpt.y;

                pWindow->handle_event(&ue);
                bPollActive     = false;

                // Return the drop effect
                DWORD effect    = get_drop_effect();
                lsp_trace("drop effect = 0x%lx", long(effect));
                *pdwEffect      = effect;

                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::DragLeave()
            {
                // Send event to the window
                event_t ue;
                init_event(&ue);
                ue.nType        = UIE_DRAG_LEAVE;

                pWindow->handle_event(&ue);

                // Release all associated resources
                release_resources();
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
            {
                HRESULT hr;

                // Validate arguments
                if ((pDataObj == NULL) || (pdwEffect == NULL))
                    return E_INVALIDARG;

                // Return the drop effect
                *pdwEffect      = DROPEFFECT_NONE;
                if (pDataSink == NULL)
                    return S_OK;

                // Check the rectangle
                if (bInternal)
                {
                    POINT dpt;
                    translate_point(dpt, pt);
                    if ((dpt.x < sRect.nLeft) || (dpt.x >= (sRect.nLeft + sRect.nWidth )) ||
                        (dpt.y < sRect.nTop)  || (dpt.y >= (sRect.nTop  + sRect.nHeight)))
                        return DragLeave();
                }

                // Open data sink
                ssize_t fmt_id  = pDataSink->open(vFormatNames.array());
                if ((fmt_id < 0) || (fmt_id >= ssize_t(vFormatNames.size() - 1)))
                    return S_OK;

                // Obtain format descriptor
                const char *fmt_name = vFormatNames.uget(fmt_id);
                FORMATETC *fmt  = (fmt_name != NULL) ? vFormatMapping.get(fmt_name) : NULL;
                if (fmt == NULL)
                {
                    pDataSink->close(STATUS_BAD_FORMAT);
                    return S_OK;
                }

                // Obtain the data
                STGMEDIUM stg;
                bzero(&stg, sizeof(stg));
                hr = pDataObj->GetData(fmt, &stg);
                if (FAILED(hr))
                {
                    pDataSink->close(STATUS_UNKNOWN_ERR);
                    return S_OK;
                }
                lsp_finally{ ReleaseStgMedium(&stg); };
                if (stg.tymed != TYMED_HGLOBAL)
                {
                    pDataSink->close(STATUS_UNSUPPORTED_FORMAT);
                    return S_OK;
                }

            #ifdef LSP_TRACE
                LSPString fmt_oname;
                if (read_clipboard_format_name(fmt->cfFormat, &fmt_oname) != STATUS_OK)
                {
                    switch (fmt->cfFormat)
                    {
                        case CF_UNICODETEXT: fmt_oname.set_ascii("CF_UNICODETEXT"); break;
                        case CF_OEMTEXT: fmt_oname.set_ascii("CF_OEMTEXT"); break;
                        case CF_TEXT: fmt_oname.set_ascii("CF_TEXT"); break;
                        case CF_HDROP: fmt_oname.set_ascii("CF_HDROP"); break;
                        default:
                            fmt_oname.fmt_ascii("unknown(0x%x)", int(fmt->cfFormat));
                            break;
                    }
                }
                else
                    fmt_oname.prepend_ascii("application/x-windows-");

                lsp_trace("FMT: fmt=%d, asp=%ld, lind=%ld, tymed=0x%lx name=%s",
                    int(fmt->cfFormat), long(fmt->dwAspect), long(fmt->lindex), long(fmt->tymed), fmt_oname.get_native());
            #endif /* LSP_TRACE */

                // Write data to the sink
                status_t res    = sink_hglobal_contents(pDataSink, stg.hGlobal, fmt->cfFormat, fmt_name);

                // Return the drop effect
                *pdwEffect      = (res == STATUS_OK) ? get_drop_effect() : DROPEFFECT_NONE;

                return DragLeave();
            }

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


