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

#ifndef PRIVATE_WIN_COM_H_
#define PRIVATE_WIN_COM_H_

#define LSP_IUNKNOWN_IFACE \
    private: \
        ULONG                   nRefCount; \
    \
    public: /* IUnknown */ \
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override; \
        virtual ULONG STDMETHODCALLTYPE AddRef() override; \
        virtual ULONG STDMETHODCALLTYPE Release() override;

#define LSP_IUNKNOWN_IMPL(Class, ParentIface) \
    HRESULT STDMETHODCALLTYPE Class::QueryInterface(REFIID iid, void **ppvObject) \
    { \
        if (IsEqualIID(iid, IID_IUnknown) || \
            IsEqualIID(iid, __uuidof(ParentIface))) \
        { \
            *ppvObject = safe_acquire(this); \
            return S_OK; \
        } \
    \
        *ppvObject = NULL; \
        return E_NOINTERFACE; \
    } \
    \
    ULONG STDMETHODCALLTYPE Class::AddRef() \
    { \
        return InterlockedIncrement(&nRefCount); \
    } \
    \
    ULONG STDMETHODCALLTYPE Class::Release() \
    { \
        ULONG newCount = InterlockedDecrement(&nRefCount); \
        if (newCount == 0) \
            delete this; \
        return newCount; \
    }

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            // Releases reference, if non-null.
            template <class T>
                static inline void safe_release(T * &obj)
                {
                    if (obj != NULL)
                    {
                        obj->Release();
                        obj = NULL;
                    }
                }

            // Acquires an additional reference, if non-null.
            template <class T>
                inline T *safe_acquire(T * obj)
                {
                    if (obj != NULL)
                        obj->AddRef();
                    return obj;
                }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PRIVATE_WIN_COM_H_ */
