/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 25 янв. 2025 г.
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

#include <private/x11/X11GLSurface.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            #define ADD_TVERTEX(v, v_ci, v_x, v_y, v_s, v_t) \
                v->x        = v_x; \
                v->y        = v_y; \
                v->s        = v_s; \
                v->t        = v_t; \
                v->cmd      = v_ci; \
                ++v;

            #define ADD_VERTEX(v, v_ci, v_x, v_y)  ADD_TVERTEX(v, v_ci, v_x, v_y, 0.0f, 0.0f)

            X11GLSurface::X11GLSurface(X11Display *display, gl::IContext *ctx, size_t width, size_t height):
                gl::Surface(display, ctx, width, height)
            {
                pX11Display = display;
            }

            X11GLSurface::X11GLSurface(X11Display *display, size_t width, size_t height):
                gl::Surface(width, height)
            {
                pX11Display = display;
            }

            X11GLSurface::~X11GLSurface()
            {
            }

            gl::Surface *X11GLSurface::create_nested(size_t width, size_t height)
            {
                return new X11GLSurface(pX11Display, width, height);
            }

            bool X11GLSurface::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                // Get font parameter using font manager
            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pX11Display->font_manager();
                if (mgr != NULL)
                {
                    if (mgr->get_font_parameters(&f, fp))
                        return true;
                }
            #endif /* USE_LIBFREETYPE */

                fp->Ascent          = 0.0f;
                fp->Descent         = 0.0f;
                fp->Height          = 0.0f;
                return false;
            }

            bool X11GLSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
            {
                if (text == NULL)
                    return false;

                // Get text parameter using font manager
            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pX11Display->font_manager();
                if (mgr != NULL)
                {
                    LSPString tmp;
                    if (!tmp.set_utf8(text))
                        return false;

                    ft::text_range_t tr;
                    if (mgr->get_text_parameters(&f, &tr, &tmp, 0, tmp.length()))
                    {
                        tp->XBearing    = tr.x_bearing;
                        tp->YBearing    = tr.y_bearing;
                        tp->Width       = tr.width;
                        tp->Height      = tr.height;
                        tp->XAdvance    = tr.x_advance;
                        tp->YAdvance    = tr.y_advance;
                        return true;
                    }
                }
            #endif /* USE_LIBFREETYPE */

                tp->XBearing        = 0.0f;
                tp->YBearing        = 0.0f;
                tp->Width           = 0.0f;
                tp->Height          = 0.0f;
                tp->XAdvance        = 0.0f;
                tp->YAdvance        = 0.0f;

                return false;
            }

            bool X11GLSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (text == NULL)
                    return false;

                // Get text parameter using font manager
            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pX11Display->font_manager();
                if (mgr != NULL)
                {
                    ft::text_range_t tr;
                    if (mgr->get_text_parameters(&f, &tr, text, first, last))
                    {
                        tp->XBearing    = tr.x_bearing;
                        tp->YBearing    = tr.y_bearing;
                        tp->Width       = tr.width;
                        tp->Height      = tr.height;
                        tp->XAdvance    = tr.x_advance;
                        tp->YAdvance    = tr.y_advance;
                        return true;
                    }
                }
            #endif /* USE_LIBFREETYPE */

                tp->XBearing        = 0.0f;
                tp->YBearing        = 0.0f;
                tp->Width           = 0.0f;
                tp->Height          = 0.0f;
                tp->XAdvance        = 0.0f;
                tp->YAdvance        = 0.0f;

                return false;
            }

            void X11GLSurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
                if (text == NULL)
                    return;

                LSPString tmp;
                if (!tmp.set_utf8(text))
                    return;

                out_text(f, color, x, y, &tmp, 0, tmp.length());
            }

            void X11GLSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (!bIsDrawing)
                    return;
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

            #ifdef USE_LIBFREETYPE
                // Rasterize text string
                ft::FontManager *mgr = pX11Display->font_manager();
                if (mgr == NULL)
                    return;

                ft::text_range_t tr;
                dsp::bitmap_t *bitmap   = mgr->render_text(&f, &tr, text, first, last);
                if (bitmap == NULL)
                    return;
                lsp_finally { ft::free_bitmap(bitmap); };

                // Allocate texture
                gl::Texture *tex = new gl::Texture(pContext);
                if (tex == NULL)
                    return;
                lsp_finally { safe_release(tex); };

                // Initialize texture
                if (tex->set_image(bitmap->data, bitmap->width, bitmap->height, bitmap->stride, gl::TEXTURE_ALPHA8) != STATUS_OK)
                    return;

                // Output the text
                {
                    const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, tex, color);
                    if (res < 0)
                        return;
                    lsp_finally { sBatch.end(); };

                    // Draw primitives
                    x                  += tr.x_bearing;
                    y                  += tr.y_bearing;

                    const uint32_t ci   = uint32_t(res);
                    const float xe      = x + bitmap->width;
                    const float ye      = y + bitmap->height;

                    const uint32_t vi   = sBatch.next_vertex_index();
                    gl::vertex_t *v     = sBatch.add_vertices(4);
                    if (v == NULL)
                        return;

                    ADD_TVERTEX(v, ci, x, y, 0.0f, 0.0f);
                    ADD_TVERTEX(v, ci, x, ye, 0.0f, 1.0f);
                    ADD_TVERTEX(v, ci, xe, ye, 1.0f, 1.0f);
                    ADD_TVERTEX(v, ci, xe, y, 1.0f, 0.0f);

                    sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
                }

                // Draw underline if required
                if (f.is_underline())
                {
                    const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, color);
                    if (res < 0)
                        return;
                    lsp_finally { sBatch.end(); };

                    const float width = lsp_max(1.0f, f.get_size() / 12.0f);
                    fill_rect(uint32_t(res),
                        x, y + tr.y_advance + 1.0f + width * 0.5f,
                        x + tr.x_advance, y + tr.y_advance + 1.0f + width * 1.5f);
                }

            #endif /* USE_LIBFREETYPE */
            }

            void X11GLSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
                if (text == NULL)
                    return;

                LSPString tmp;
                if (!tmp.set_utf8(text))
                    return;

                out_text_relative(f, color, x, y, dx, dy, &tmp, 0, tmp.length());
            }

            void X11GLSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (!bIsDrawing)
                    return;
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

            #ifdef USE_LIBFREETYPE
                // Rasterize text string
                ft::FontManager *mgr = pX11Display->font_manager();
                if (mgr == NULL)
                    return;

                ft::text_range_t tr;
                dsp::bitmap_t *bitmap   = mgr->render_text(&f, &tr, text, first, last);
                if (bitmap == NULL)
                    return;
                lsp_finally { ft::free_bitmap(bitmap); };

                // Allocate texture
                gl::Texture *tex = new gl::Texture(pContext);
                if (tex == NULL)
                    return;
                lsp_finally { safe_release(tex); };

                // Initialize texture
                if (tex->set_image(bitmap->data, bitmap->width, bitmap->height, bitmap->stride, gl::TEXTURE_ALPHA8) != STATUS_OK)
                    return;

                // Output the text
                float r_w, r_h, fx, fy;
                {
                    const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, tex, color);
                    if (res < 0)
                        return;
                    lsp_finally { sBatch.end(); };

                    // Draw primitives
                    r_w                 = tr.x_advance;
                    r_h                 = -tr.y_bearing;
                    fx                  = truncf(x - float(tr.x_bearing) - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx);
                    fy                  = truncf(y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy);
                    x                   = fx + float(tr.x_bearing);
                    y                   = fy + float(tr.y_bearing);

                    const uint32_t ci   = uint32_t(res);
                    const float xe      = x + bitmap->width;
                    const float ye      = y + bitmap->height;

                    const uint32_t vi   = sBatch.next_vertex_index();
                    gl::vertex_t *v     = sBatch.add_vertices(4);
                    if (v == NULL)
                        return;

                    ADD_TVERTEX(v, ci, x, y, 0.0f, 0.0f);
                    ADD_TVERTEX(v, ci, x, ye, 0.0f, 1.0f);
                    ADD_TVERTEX(v, ci, xe, ye, 1.0f, 1.0f);
                    ADD_TVERTEX(v, ci, xe, y, 1.0f, 0.0f);

                    sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
                }

                // Draw underline if required
                if (f.is_underline())
                {
                    const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, color);
                    if (res < 0)
                        return;
                    lsp_finally { sBatch.end(); };

                    const float width = lsp_max(1.0f, f.get_size() / 12.0f);
                    fill_rect(uint32_t(res),
                        fx, fy + tr.y_advance + 1.0f + width * 0.5f,
                        fx + tr.x_advance, fy + tr.y_advance + 1.0f + width * 1.5f);
                }

            #endif /* USE_LIBFREETYPE */
            }

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */



