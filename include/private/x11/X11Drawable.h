/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 8 янв. 2026 г.
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

#ifndef PRIVATE_X11_X11DRAWABLE_H_
#define PRIVATE_X11_X11DRAWABLE_H_

#include <lsp-plug.in/ws/version.h>

#ifdef USE_LIBX11

#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/ws/IDrawable.h>

#include <X11/Xlib.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            class LSP_HIDDEN_MODIFIER X11Drawable: public ws::IDrawable
            {
                protected:
                    ::Window        hWindow;
                    ipc::Mutex      sMutex;

                public:
                    explicit X11Drawable(::Window window);
                    X11Drawable(const X11Drawable &) = delete;
                    X11Drawable(X11Drawable &&) = delete;

                    X11Drawable & operator = (const X11Drawable &) = delete;
                    X11Drawable & operator = (X11Drawable &&) = delete;

                public:
                    /**
                     * Test for validity and lock
                     * @return true if drawable is valid and has been locked
                     */
                    bool            lock();

                    /**
                     * Unlock the drawable
                     */
                    void            unlock();

                    /**
                     * Get X11 window handle
                     * @return X11 window handle
                     */
                    ::Window        x11window() const;

                public:
                    /**
                     * Test for validity
                     * @return true if drawable is valid
                     */
                    virtual bool valid() const override;

                    /**
                     * Invalidate the drawable. The method is blocking and does not
                     * update state while drawable is locked
                     */
                    virtual void invalidate() override;

                    /**
                     * Return handle to the X11 window
                     */
                    virtual void *handle() const override;
            };
        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */


#endif /* PRIVATE_X11_X11DRAWABLE_H_ */
