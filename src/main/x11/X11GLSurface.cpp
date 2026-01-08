/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL_GLX

#include <private/x11/X11GLSurface.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            X11GLSurface::X11GLSurface(X11Display *display, gl::SurfaceContext * context):
                gl::Surface(display, context)
            {
                pX11Display = display;
            }

            X11GLSurface::~X11GLSurface()
            {
            }

            ISurface *X11GLSurface::create(size_t width, size_t height)
            {
                if ((pSurface == NULL) || (!pSurface->valid()))
                    return NULL;

                gl::SurfaceContext *surface = new gl::SurfaceContext(pSurface);
                if (surface == NULL)
                    return NULL;
                lsp_finally { safe_release(surface); };

                X11GLSurface *s = new X11GLSurface(pX11Display, surface);
                if (s == NULL)
                    return NULL;

                s->nWidth       = width;
                s->nHeight      = height;

                return s;
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

            void X11GLSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text)
            {
                if (text == NULL)
                    return;
                out_text(f, color, x, y, text, 0, text->length());
            }

            void X11GLSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first)
            {
                if (text == NULL)
                    return;
                out_text(f, color, x, y, text, first, text->length());
            }

            void X11GLSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (!pSurface->is_drawing())
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

                // Output commaand
                gl::actions::out_text_bitmap_t * cmd = pSurface->append_command<gl::actions::out_text_bitmap_t>();
                if (cmd == NULL)
                    return;

                gl::set_color(cmd->fill, color);
                cmd->bitmap     = release_ptr(bitmap);
                cmd->x          = x + tr.x_bearing;
                cmd->y          = y + tr.y_bearing;

                // Draw underline if required
                if (f.is_underline())
                {
                    const float width       = lsp_max(1.0f, f.get_size() * (1.0f / 12.0f));

                    cmd->underline.x        = x + tr.x_bearing;
                    cmd->underline.y        = y + width;
                    cmd->underline.width    = tr.x_advance;
                    cmd->underline.height   = width;
                }
                else
                {
                    cmd->underline.x        = 0.0f;
                    cmd->underline.y        = 0.0f;
                    cmd->underline.width    = 0.0f;
                    cmd->underline.height   = 0.0f;
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

            void X11GLSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text)
            {
                if (text == NULL)
                    return;
                out_text_relative(f, color, x, y, dx, dy, text, 0, text->length());
            }

            void X11GLSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first)
            {
                if (text == NULL)
                    return;
                out_text_relative(f, color, x, y, dx, dy, text, first, text->length());
            }

            void X11GLSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (!pSurface->is_drawing())
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

                // Output commaand
                gl::actions::out_text_bitmap_t * cmd = pSurface->append_command<gl::actions::out_text_bitmap_t>();
                if (cmd == NULL)
                    return;

                const float r_w     = tr.x_advance;
                const float r_h     = -tr.y_bearing;
                const float fx      = truncf(x - float(tr.x_bearing) - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx);
                const float fy      = truncf(y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy);

                gl::set_color(cmd->fill, color);
                cmd->bitmap     = release_ptr(bitmap);
                cmd->x              = fx + tr.x_bearing;
                cmd->y              = fy + tr.y_bearing;

                // Draw underline if required
                if (f.is_underline())
                {
                    const float width       = lsp_max(1.0f, f.get_size() * (1.0f / 12.0f));

                    cmd->underline.x        = cmd->x + tr.x_bearing;
                    cmd->underline.y        = fy + width;
                    cmd->underline.width    = tr.x_advance;
                    cmd->underline.height   = width;
                }
                else
                {
                    cmd->underline.x        = 0.0f;
                    cmd->underline.y        = 0.0f;
                    cmd->underline.width    = 0.0f;
                    cmd->underline.height   = 0.0f;
                }
            #endif /* USE_LIBFREETYPE */
            }

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL_GLX */

