/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 5 июл. 2022 г.
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

#ifndef PRIVATE_WIN_WINDDGRADIENT_H_
#define PRIVATE_WIN_WINDDGRADIENT_H_

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/ISurface.h>
#include <lsp-plug.in/ws/IGradient.h>

#include <private/win/WinDisplay.h>

#include <d2d1.h>
#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            class LSP_HIDDEN_MODIFIER WinDDGradient: public IGradient
            {
                private:
                    ID2D1RenderTarget  *pDC;
                    ID2D1Brush         *pBrush;
                    union
                    {
                        D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES   sLinear;
                        D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES   sRadial;
                    };
                    D2D1_GRADIENT_STOP  vPoints[2];
                    bool                bLinear;

                private:
                    inline void     drop_brush();

                public:
                    explicit WinDDGradient(ID2D1RenderTarget *dc, const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES & props);
                    explicit WinDDGradient(ID2D1RenderTarget *dc, const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES & props);
                    WinDDGradient(const WinDDGradient &) = delete;
                    WinDDGradient(WinDDGradient &&) = delete;
                    virtual ~WinDDGradient() override;

                    WinDDGradient & operator = (const WinDDGradient &) = delete;
                    WinDDGradient & operator = (WinDDGradient &&) = delete;

                public:
                    virtual void set_start(float r, float g, float b, float a=0.0f) override;
                    virtual void set_start(const Color &c) override;
                    virtual void set_start(const Color &c, float a) override;

                    virtual void set_stop(float r, float g, float b, float a=0.0f) override;
                    virtual void set_stop(const Color &c) override;
                    virtual void set_stop(const Color &c, float a) override;

                public:
                    ID2D1Brush     *get_brush();
            };

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_WINDDGRADIENT_H_ */
