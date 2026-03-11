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

#ifndef LSP_PLUG_IN_WS_IDRAWABLE_H_
#define LSP_PLUG_IN_WS_IDRAWABLE_H_

#include <lsp-plug.in/common/atomic.h>

namespace lsp
{
    namespace ws
    {
        /**
         * Drawable descriptor interface to allow access by graphic APIs like OpenGL
         */
        class IDrawable
        {
            private:
                uatomic_t           nReferences;

            public:
                IDrawable();
                IDrawable(const IDrawable &) = delete;
                IDrawable(IDrawable &&) = delete;
                virtual ~IDrawable();

                IDrawable & operator = (const IDrawable &) = delete;
                IDrawable & operator = (IDrawable &&) = delete;

            public:
                uatomic_t   reference_up();
                uatomic_t   reference_down();

            public:
                /**
                 * Check that drawable is valid
                 * @return true if drawable is valid
                 */
                virtual bool valid() const;

                /**
                 * Invalidate the drawable. Once drawable descriptor becomes invalidated, it can not become valid anymore.
                 * Window should return new descriptor if it becomes drawable again.
                 */
                virtual void invalidate();

                /**
                 * Get drawable handle casted to raw pointer
                 */
                virtual void *handle() const;
        };
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_IDRAWABLE_H_ */
