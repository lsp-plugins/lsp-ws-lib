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

#ifndef PRIVATE_WIN_FONTS_H_
#define PRIVATE_WIN_FONTS_H_

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/lltl/darray.h>

#include <dwrite.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            class WinFontFileStream;
            class WinFontFileLoader;
            class WinFontFileEnumerator;
            class WinFontCollectionLoader;

            /**
             * Custom implementation of IDWriteFontFileStream interface for collections
             * of fonts embedded in the application as resources. The font collection
             * key is an index in the WinCustomFonts collection.
             */
            class LSP_SYMBOL_HIDDEN WinFontFileStream: public IDWriteFontFileStream
            {
                private:
                    ULONG                   nRefCount;
                    size_t                  nOffset;
                    WinFontFileLoader      *pLoader;

                public:
                    explicit WinFontFileStream(WinFontFileLoader *loader);
                    virtual ~WinFontFileStream();

                public: // IUnknown
                    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override;
                    virtual ULONG STDMETHODCALLTYPE AddRef() override;
                    virtual ULONG STDMETHODCALLTYPE Release() override;

                public: // IDWriteFontFileStream
                    virtual HRESULT STDMETHODCALLTYPE ReadFileFragment(
                        void const **fragmentStart,
                        UINT64 fileOffset,
                        UINT64 fragmentSize,
                        OUT void **fragmentContext) override;

                    virtual void STDMETHODCALLTYPE ReleaseFileFragment(void *fragmentContext) override;
                    virtual HRESULT STDMETHODCALLTYPE GetFileSize(OUT UINT64 *fileSize) override;
                    virtual HRESULT STDMETHODCALLTYPE GetLastWriteTime(OUT UINT64 *lastWriteTime) override;
            };

            /**
             * Custom implementation of IDWriteFontFileLoader interface for collections
             * of fonts embedded in the application as resources. The font collection
             * key is an index in the WinCustomFonts collection.
             */
            class LSP_SYMBOL_HIDDEN WinFontFileLoader: public IDWriteFontFileLoader
            {
                private:
                    friend class WinFontFileStream;

                private:
                    ULONG                   nRefCount;
                    uint8_t                *pData;
                    size_t                  nSize;

                public:
                    explicit WinFontFileLoader(io::OutMemoryStream *os);
                    virtual ~WinFontFileLoader();

                public: // IUnknown
                    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override;
                    virtual ULONG STDMETHODCALLTYPE AddRef() override;
                    virtual ULONG STDMETHODCALLTYPE Release() override;

                public: // IDWriteFontFileLoader
                    virtual HRESULT STDMETHODCALLTYPE CreateStreamFromKey(
                        void const* fontFileReferenceKey,
                        UINT32 fontFileReferenceKeySize,
                        OUT IDWriteFontFileStream **fontFileStream) override;
            };

            /**
             * The implementation of custom IDWriteFontFileEnumerator interface for a collection
             * of fonts embedded in the application as resources. The font collection
             * key is a pointer to custom font loader.
             */
            class LSP_SYMBOL_HIDDEN WinFontFileEnumerator: public IDWriteFontFileEnumerator
            {
                private:
                    ULONG                   nRefCount;
                    IDWriteFactory         *pFactory;
                    IDWriteFontFile        *pCurrFile;
                    WinFontFileLoader      *pLoader;
                    size_t                  nNextIndex;

                public:
                    explicit WinFontFileEnumerator(IDWriteFactory *factory, WinFontFileLoader *loader);
                    virtual ~WinFontFileEnumerator();

                public: // IUnknown
                    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override;
                    virtual ULONG STDMETHODCALLTYPE AddRef() override;
                    virtual ULONG STDMETHODCALLTYPE Release() override;

                public: // IDWriteFontFileEnumerator
                    virtual HRESULT STDMETHODCALLTYPE MoveNext(OUT BOOL *hasCurrentFile) override;
                    virtual HRESULT STDMETHODCALLTYPE GetCurrentFontFile(OUT IDWriteFontFile **fontFile) override;
            };

            /**
             * The implementation of custom IDWriteFontCollectionLoader interface
             * for collections of fonts embedded in the application as resources. The font
             * collection key is an array of resource IDs.
             */
            class LSP_SYMBOL_HIDDEN WinFontCollectionLoader: public IDWriteFontCollectionLoader
            {
                private:
                    ULONG                   nRefCount;

                public:
                    explicit WinFontCollectionLoader();
                    virtual ~WinFontCollectionLoader();

                public: // IUnknown
                    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override;
                    virtual ULONG STDMETHODCALLTYPE AddRef() override;
                    virtual ULONG STDMETHODCALLTYPE Release() override;

                public: // IDWriteFontCollectionLoader
                    virtual HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey(
                        IDWriteFactory *factory,
                        const void *collectionKey,
                        UINT32 collectionKeySize,
                        IDWriteFontFileEnumerator **fontFileEnumerator) override;
            };

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_FONTS_H_ */
