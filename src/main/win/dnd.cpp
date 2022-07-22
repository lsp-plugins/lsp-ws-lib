/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <private/win/dnd.h>
#include <private/win/WinWindow.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            LSP_IUNKNOWN_IMPL(WinDNDTarget, IDropTarget)

            WinDNDTarget::WinDNDTarget(WinWindow *wnd)
            {
                nRefCount   = 0;
                pWindow     = wnd;
            }

            WinDNDTarget::~WinDNDTarget()
            {
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
            {
                // TODO
                return 0;
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
            {
                // TODO
                return 0;
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::DragLeave()
            {
                // TODO
                return 0;
            }

            HRESULT STDMETHODCALLTYPE WinDNDTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
            {
                // TODO
                return 0;
            }
        } // namespace win
    } // namespace ws
} // namespace lsp

#endif /* PLATFORM_WINDOWS */


