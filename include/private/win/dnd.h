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

#ifndef PRIVATE_WIN_DND_H_
#define PRIVATE_WIN_DND_H_

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <private/win/com.h>

#include <oleidl.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            class WinWindow;

            class LSP_SYMBOL_HIDDEN WinDNDTarget: public IDropTarget
            {
                LSP_IUNKNOWN_IFACE

                private:
                    WinWindow      *pWindow;

                public:
                    explicit WinDNDTarget(WinWindow *wnd);
                    virtual ~WinDNDTarget();

                public: // IDropTarget
                    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
                    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
                    virtual HRESULT STDMETHODCALLTYPE DragLeave() override;
                    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
            };
        }
    }
}

#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_DND_H_ */
