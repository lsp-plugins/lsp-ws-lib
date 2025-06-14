/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 19 дек. 2016 г.
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

 #ifndef UI_CocoaCairoGradient_H_
 #define UI_CocoaCairoGradient_H_
 
 #include <lsp-plug.in/ws/version.h>
 
 #ifdef PLATFORM_MACOSX
 
 #include <lsp-plug.in/common/types.h>
 #include <lsp-plug.in/ws/IGradient.h>
 
 #include <cairo/cairo.h>
 
 namespace lsp
 {
     namespace ws
     {
         namespace cocoa
         {
             class LSP_HIDDEN_MODIFIER CocoaCairoGradient: public IGradient
             {
                 public:
                     typedef struct linear_t
                     {
                         float x1;
                         float y1;
                         float x2;
                         float y2;
                     } linear_t;
 
                     typedef struct radial_t
                     {
                         float x1;
                         float y1;
                         float x2;
                         float y2;
                         float r;
                     } radial_t;
 
                 protected:
                     typedef struct color_t
                     {
                         float r, g, b, a;
                     } color_t;
 
                 protected:
                     cairo_pattern_t    *pCP;
                     union
                     {
                         linear_t    sLinear;
                         radial_t    sRadial;
                     };
                     color_t             sStart;
                     color_t             sEnd;
                     bool                bLinear;
 
                 protected:
                     void drop_pattern();
 
                 public:
                     explicit CocoaCairoGradient(const linear_t & params);
                     explicit CocoaCairoGradient(const radial_t & params);
                     CocoaCairoGradient(const CocoaCairoGradient &) = delete;
                     CocoaCairoGradient(CocoaCairoGradient &&) = delete;
                     virtual ~CocoaCairoGradient() override;
 
                     CocoaCairoGradient & operator = (const CocoaCairoGradient &) = delete;
                     CocoaCairoGradient & operator = (CocoaCairoGradient &&) = delete;
 
                 public:
                     virtual void set_start(float r, float g, float b, float a) override;
                     virtual void set_stop(float r, float g, float b, float a) override;
 
                 public:
                     void apply(cairo_t *cr);
             };
 
         } /* namespace cocoa */
     } /* namespace ws */
 } /* namespace lsp */
 
 #endif /* defined(PLATFORM_MACOSX) */
 
 #endif /* UI_X11_CocoaCairoGradient_H_ */
 