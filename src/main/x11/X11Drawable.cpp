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

#include <private/x11/X11Drawable.h>

#ifdef USE_LIBX11

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            X11Drawable::X11Drawable(::Window window)
            {
                atomic_store(&hWindow, window);
            }

            bool X11Drawable::lock()
            {
                // Test
                if (!X11Drawable::valid())
                    return false;

                // Lock
                if (!sMutex.lock())
                    return false;

                // Test again
                if (X11Drawable::valid())
                    return true;

                sMutex.unlock();
                return false;
            }

            void X11Drawable::unlock()
            {
                sMutex.unlock();
            }

            ::Window X11Drawable::x11window() const
            {
                return atomic_load(&hWindow);
            }

            bool X11Drawable::valid() const
            {
                const ::Window wnd = atomic_load(&hWindow);
                return (wnd != None);
            }

            void X11Drawable::invalidate()
            {
                sMutex.lock();
                lsp_finally { sMutex.unlock(); };
                atomic_store(&hWindow, ::Window(None));
            }

            void *X11Drawable::handle() const
            {
                const ::Window wnd = atomic_load(&hWindow);
                return reinterpret_cast<void *>(wnd);
            }
        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */


