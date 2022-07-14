/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 13 июл. 2022 г.
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

#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/lltl/parray.h>

#include <private/win/fonts.h>

#include <combaseapi.h>
#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            //-----------------------------------------------------------------
            // Common methods
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

            //-----------------------------------------------------------------
            // WinFontFileStream implementation
            WinFontFileStream::WinFontFileStream(WinFontFileLoader *loader)
            {
                nRefCount       = 0;
                nOffset         = 0;
                pLoader         = safe_acquire(loader);
            }

            WinFontFileStream::~WinFontFileStream()
            {
                safe_release(pLoader);
            }

            HRESULT STDMETHODCALLTYPE WinFontFileStream::QueryInterface(REFIID iid, void **ppvObject)
            {
                if (IsEqualIID(iid, IID_IUnknown) ||
                    IsEqualIID(iid, __uuidof(IDWriteFontFileStream)))
                {
                    *ppvObject = safe_acquire(this);
                    return S_OK;
                }

                *ppvObject = NULL;
                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE WinFontFileStream::AddRef()
            {
                return InterlockedIncrement(&nRefCount);
            }

            ULONG STDMETHODCALLTYPE WinFontFileStream::Release()
            {
                ULONG newCount = InterlockedDecrement(&nRefCount);
                if (newCount == 0)
                    delete this;
                return newCount;
            }

            HRESULT STDMETHODCALLTYPE WinFontFileStream::ReadFileFragment(
                void const **fragmentStart,
                UINT64 fileOffset,
                UINT64 fragmentSize,
                OUT void **fragmentContext)
            {
                // Validate the position
                wssize_t end    = fileOffset + fragmentSize;
                if (end <= pLoader->nSize)
                {
                    // Return the pointer
                    *fragmentStart      = &pLoader->pData[fileOffset];
                    *fragmentContext    = NULL;
                    return S_OK;
                }

                // Out of bounds
                *fragmentStart      = NULL;
                *fragmentContext    = NULL;
                return E_FAIL;
            }

            void STDMETHODCALLTYPE WinFontFileStream::ReleaseFileFragment(void *fragmentContext)
            {
                // Do nothing
            }

            HRESULT STDMETHODCALLTYPE WinFontFileStream::GetFileSize(OUT UINT64 *fileSize)
            {
                if (fileSize == NULL)
                    return E_INVALIDARG;
                *fileSize           = pLoader->nSize;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE WinFontFileStream::GetLastWriteTime(OUT UINT64 *lastWriteTime)
            {
                if (lastWriteTime == NULL)
                    return E_INVALIDARG;
                lastWriteTime       = 0;
                return S_OK;
            }

            //-----------------------------------------------------------------
            // WinCustomFontLoader implementation
            WinFontFileLoader::WinFontFileLoader(io::OutMemoryStream *os)
            {
                nRefCount       = 0;
                nSize           = os->size();
                pData           = os->release();
            }

            WinFontFileLoader::~WinFontFileLoader()
            {
                if (pData != NULL)
                {
                    free(pData);
                    pData       = NULL;
                }
                nSize       = 0;
            }

            HRESULT STDMETHODCALLTYPE WinFontFileLoader::QueryInterface(REFIID iid, void** ppvObject)
            {
                if (IsEqualIID(iid, IID_IUnknown) ||
                    IsEqualIID(iid, __uuidof(IDWriteFontFileLoader)))
                {
                    *ppvObject = safe_acquire(this);
                    return S_OK;
                }

                *ppvObject = NULL;
                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE WinFontFileLoader::AddRef()
            {
                return InterlockedIncrement(&nRefCount);
            }

            ULONG STDMETHODCALLTYPE WinFontFileLoader::Release()
            {
                ULONG newCount = InterlockedDecrement(&nRefCount);
                if (newCount == 0)
                    delete this;
                return newCount;
            }

            HRESULT STDMETHODCALLTYPE WinFontFileLoader::CreateStreamFromKey(
                void const *fontFileReferenceKey,
                UINT32 fontFileReferenceKeySize,
                OUT IDWriteFontFileStream **fontFileStream)
            {
                // Validate arguments
                if (fontFileStream == NULL)
                    return E_INVALIDARG;
                *fontFileStream     = NULL;
                if (fontFileReferenceKeySize != sizeof(WinFontFileLoader * const *))
                    return E_INVALIDARG;
                WinFontFileLoader *loader = *static_cast<WinFontFileLoader * const *>(fontFileReferenceKey);
                if (loader != this)
                    return E_INVALIDARG;

                // Create data stream
                WinFontFileStream *s= new WinFontFileStream(this);
                if (s == NULL)
                    return E_OUTOFMEMORY;

                // Return the result
                *fontFileStream     = safe_acquire(s);
                return S_OK;
            }

            //-----------------------------------------------------------------
            // WinFontFileEnumerator implementation
            WinFontFileEnumerator::WinFontFileEnumerator(IDWriteFactory *factory, WinFontFileLoader *loader)
            {
                nRefCount           = 0;
                pFactory            = safe_acquire(factory);
                pCurrFile           = NULL;
                pLoader             = safe_acquire(loader);
                nNextIndex          = 0;
            }

            WinFontFileEnumerator::~WinFontFileEnumerator()
            {
                safe_release(pLoader);
                safe_release(pCurrFile);
                safe_release(pFactory);
            }

            HRESULT STDMETHODCALLTYPE WinFontFileEnumerator::QueryInterface(REFIID iid, void **ppvObject)
            {
                if (IsEqualIID(iid, IID_IUnknown) ||
                    IsEqualIID(iid, __uuidof(IDWriteFontFileEnumerator)))
                {
                    *ppvObject = safe_acquire(this);
                    return S_OK;
                }

                *ppvObject = NULL;
                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE WinFontFileEnumerator::AddRef()
            {
                return InterlockedIncrement(&nRefCount);
            }

            ULONG STDMETHODCALLTYPE WinFontFileEnumerator::Release()
            {
                ULONG newCount = InterlockedDecrement(&nRefCount);
                if (newCount == 0)
                    delete this;
                return newCount;
            }

            HRESULT STDMETHODCALLTYPE WinFontFileEnumerator::MoveNext(OUT BOOL *hasCurrentFile)
            {
                HRESULT hr = S_OK;
                if (hasCurrentFile == NULL)
                    return E_INVALIDARG;

                *hasCurrentFile = FALSE;
                safe_release(pCurrFile);
                if (nNextIndex >= 1)
                    return hr;

                hr = pFactory->CreateCustomFontFileReference(&pLoader, sizeof(&pLoader), pLoader, &pCurrFile);
                if ((FAILED(hr)) || (pCurrFile == NULL))
                    return hr;

                *hasCurrentFile = TRUE;
                ++nNextIndex;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE WinFontFileEnumerator::GetCurrentFontFile(OUT IDWriteFontFile **fontFile)
            {
                if (fontFile == NULL)
                    return E_INVALIDARG;

                IDWriteFontFile *res = safe_acquire(pCurrFile);
                *fontFile = res;

                return (res != NULL) ? S_OK : E_FAIL;
            }

            //-----------------------------------------------------------------
            // WinFontCollectionLoader implementation
            WinFontCollectionLoader::WinFontCollectionLoader()
            {
                nRefCount   = 0;
            }

            WinFontCollectionLoader::~WinFontCollectionLoader()
            {
            }

            HRESULT STDMETHODCALLTYPE WinFontCollectionLoader::QueryInterface(REFIID iid, void **ppvObject)
            {
                if (IsEqualIID(iid, IID_IUnknown) ||
                    IsEqualIID(iid, __uuidof(IDWriteFontCollectionLoader)))
                {
                    *ppvObject = safe_acquire(this);
                    return S_OK;
                }

                *ppvObject = NULL;
                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE WinFontCollectionLoader::AddRef()
            {
                return InterlockedIncrement(&nRefCount);
            }

            ULONG STDMETHODCALLTYPE WinFontCollectionLoader::Release()
            {
                ULONG newCount = InterlockedDecrement(&nRefCount);
                if (newCount == 0)
                    delete this;
                return newCount;
            }

            HRESULT STDMETHODCALLTYPE WinFontCollectionLoader::CreateEnumeratorFromKey(
                IDWriteFactory *factory,
                const void *collectionKey,
                UINT32 collectionKeySize,
                IDWriteFontFileEnumerator **fontFileEnumerator)
            {
                // Validate arguments
                if (fontFileEnumerator == NULL)
                    return E_INVALIDARG;
                *fontFileEnumerator = NULL;
                if (collectionKeySize != sizeof(WinFontCollectionLoader * const *))
                    return E_INVALIDARG;
                WinFontFileLoader *loader = *static_cast<WinFontFileLoader * const *>(collectionKey);

                // Create font file enumerator
                WinFontFileEnumerator *enumerator = new WinFontFileEnumerator(factory, loader);
                if (enumerator == NULL)
                    return E_OUTOFMEMORY;

                // Return result
                *fontFileEnumerator = safe_acquire(enumerator);
                return S_OK;
            }

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


