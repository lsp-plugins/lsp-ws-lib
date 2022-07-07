/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 25 окт. 2016 г.
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

#if defined(USE_LIBX11) && defined(USE_LIBCAIRO)

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/math.h>


#include <private/x11/X11CairoGradient.h>
#include <private/x11/X11CairoSurface.h>
#include <private/x11/X11Display.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <cairo/cairo-xlib.h>

// Freetype headers
#ifdef USE_LIBFREETYPE
    #include <ft2build.h>
    #include FT_SFNT_NAMES_H
    #include FT_FREETYPE_H
    #include FT_GLYPH_H
    #include FT_OUTLINE_H
    #include FT_BBOX_H
    #include FT_TYPE1_TABLES_H
#endif /* USE_LIBFREETYPE */

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            static inline cairo_antialias_t decode_antialiasing(const Font &f)
            {
                switch (f.antialiasing())
                {
                    case FA_DISABLED: return CAIRO_ANTIALIAS_NONE;
                    case FA_ENABLED: return CAIRO_ANTIALIAS_GOOD;
                    default: break;
                }
                return CAIRO_ANTIALIAS_DEFAULT;
            }

            X11CairoSurface::X11CairoSurface(X11Display *dpy, Drawable drawable, Visual *visual, size_t width, size_t height):
                ISurface(width, height, ST_XLIB)
            {
                pDisplay        = dpy;
                pCR             = NULL;
                pFO             = NULL;
                pSurface        = ::cairo_xlib_surface_create(dpy->x11display(), drawable, visual, width, height);
                nNumClips       = 0;
            }

            X11CairoSurface::X11CairoSurface(X11Display *dpy, size_t width, size_t height):
                ISurface(width, height, ST_IMAGE)
            {
                pDisplay        = dpy;
                pCR             = NULL;
                pFO             = NULL;
                pSurface        = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                nStride         = cairo_image_surface_get_stride(pSurface);
                nNumClips       = 0;
            }

            ISurface *X11CairoSurface::create(size_t width, size_t height)
            {
                return new X11CairoSurface(pDisplay, width, height);
            }

            ISurface *X11CairoSurface::create_copy()
            {
                X11CairoSurface *s = new X11CairoSurface(pDisplay, nWidth, nHeight);
                if (s == NULL)
                    return NULL;

                // Draw one surface on another
                s->begin();
                    ::cairo_set_source_surface(s->pCR, pSurface, 0.0f, 0.0f);
                    ::cairo_paint(s->pCR);
                s->end();

                return s;
            }

            IGradient *X11CairoSurface::linear_gradient(float x0, float y0, float x1, float y1)
            {
                return new X11CairoGradient(
                    ::cairo_pattern_create_linear(x0, y0, x1, y1));
            }

            IGradient *X11CairoSurface::radial_gradient(float cx0, float cy0, float cx1, float cy1, float r)
            {
                return new X11CairoGradient(
                    ::cairo_pattern_create_radial(cx0, cy0, 0, cx1, cy1, r));
            }

            X11CairoSurface::~X11CairoSurface()
            {
                destroy_context();
            }

            void X11CairoSurface::destroy_context()
            {
                if (pFO != NULL)
                {
                    cairo_font_options_destroy(pFO);
                    pFO             = NULL;
                }
                if (pCR != NULL)
                {
                    cairo_destroy(pCR);
                    pCR             = NULL;
                }
                if (pSurface != NULL)
                {
                    cairo_surface_destroy(pSurface);
                    pSurface        = NULL;
                }
            }

            void X11CairoSurface::destroy()
            {
                destroy_context();
            }

            bool X11CairoSurface::resize(size_t width, size_t height)
            {
                if (nType == ST_XLIB)
                {
                    ::cairo_xlib_surface_set_size(pSurface, width, height);
                    return true;
                }
                else if (nType == ST_IMAGE)
                {
                    // Create new surface and cairo
                    cairo_surface_t *s  = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                    if (s == NULL)
                        return false;
                    cairo_t *cr         = ::cairo_create(s);
                    if (cr == NULL)
                    {
                        cairo_surface_destroy(s);
                        return false;
                    }

                    // Draw previous content
                    ::cairo_set_source_surface(cr, pSurface, 0, 0);
                    ::cairo_fill(cr);

                    // Destroy previously used context
                    destroy_context();

                    // Update context
                    pSurface            = s;
                    if (pCR != NULL)
                    {
                        ::cairo_destroy(pCR);
                        pCR                 = cr;
                    }
                    else
                        ::cairo_destroy(cr);
                }

                return false;
            }

            void X11CairoSurface::draw(ISurface *s, float x, float y)
            {
                surface_type_t type = s->type();
                if ((type != ST_XLIB) && (type != ST_IMAGE))
                    return;
                if (pCR == NULL)
                    return;
                X11CairoSurface *cs = static_cast<X11CairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_set_source_surface(pCR, cs->pSurface, x, y);
                ::cairo_paint(pCR);
            }

            void X11CairoSurface::draw(ISurface *s, float x, float y, float sx, float sy)
            {
                surface_type_t type = s->type();
                if ((type != ST_XLIB) && (type != ST_IMAGE))
                    return;
                if (pCR == NULL)
                    return;
                X11CairoSurface *cs = static_cast<X11CairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_save(pCR);
                if (sx < 0.0f)
                    x       -= sx * s->width();
                if (sy < 0.0f)
                    y       -= sy * s->height();
                ::cairo_translate(pCR, x, y);
                ::cairo_scale(pCR, sx, sy);
                ::cairo_set_source_surface(pCR, cs->pSurface, 0.0f, 0.0f);
                ::cairo_paint(pCR);
                ::cairo_restore(pCR);
            }

            void X11CairoSurface::draw_alpha(ISurface *s, float x, float y, float sx, float sy, float a)
            {
                surface_type_t type = s->type();
                if ((type != ST_XLIB) && (type != ST_IMAGE))
                    return;
                if (pCR == NULL)
                    return;
                X11CairoSurface *cs = static_cast<X11CairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_save(pCR);
                if (sx < 0.0f)
                    x       -= sx * s->width();
                if (sy < 0.0f)
                    y       -= sy * s->height();
                ::cairo_translate(pCR, x, y);
                ::cairo_scale(pCR, sx, sy);
                ::cairo_set_source_surface(pCR, cs->pSurface, 0.0f, 0.0f);
                ::cairo_paint_with_alpha(pCR, 1.0f - a);
                ::cairo_restore(pCR);
            }

            void X11CairoSurface::draw_rotate_alpha(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
            {
                surface_type_t type = s->type();
                if ((type != ST_XLIB) && (type != ST_IMAGE))
                    return;
                if (pCR == NULL)
                    return;
                X11CairoSurface *cs = static_cast<X11CairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_save(pCR);
                ::cairo_translate(pCR, x, y);
                ::cairo_scale(pCR, sx, sy);
                ::cairo_rotate(pCR, ra);
                ::cairo_set_source_surface(pCR, cs->pSurface, 0.0f, 0.0f);
                ::cairo_paint_with_alpha(pCR, 1.0f - a);
                ::cairo_restore(pCR);
            }

            void X11CairoSurface::draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh)
            {
                surface_type_t type = s->type();
                if ((type != ST_XLIB) && (type != ST_IMAGE))
                    return;
                if (pCR == NULL)
                    return;
                X11CairoSurface *cs = static_cast<X11CairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_save(pCR);
                ::cairo_set_source_surface(pCR, cs->pSurface, x - sx, y - sy);
                ::cairo_rectangle(pCR, x, y, sw, sh);
                ::cairo_fill(pCR);
                ::cairo_restore(pCR);
            }

            void X11CairoSurface::begin()
            {
                // Force end() call
                end();

                // Create cairo objects
                pCR             = ::cairo_create(pSurface);
                if (pCR == NULL)
                    return;
                pFO             = ::cairo_font_options_create();
                if (pFO == NULL)
                    return;

                // Initialize settings
                ::cairo_set_antialias(pCR, CAIRO_ANTIALIAS_DEFAULT);
                ::cairo_set_line_join(pCR, CAIRO_LINE_JOIN_BEVEL);

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            void X11CairoSurface::end()
            {
                if (pCR == NULL)
                    return;

            #ifdef LSP_DEBUG
                if (nNumClips > 0)
                    lsp_error("Mismatching number of clip_begin() and clip_end() calls");
            #endif /* LSP_DEBUG */

                if (pFO != NULL)
                {
                    cairo_font_options_destroy(pFO);
                    pFO             = NULL;
                }
                if (pCR != NULL)
                {
                    cairo_destroy(pCR);
                    pCR             = NULL;
                }

                ::cairo_surface_flush(pSurface);
            }

            void X11CairoSurface::set_current_font(font_context_t *ctx, const Font &f)
            {
                // Apply antialiasint to the font
                ctx->aa     = cairo_font_options_get_antialias(pFO);
                cairo_font_options_set_antialias(pFO, decode_antialiasing(f));
                cairo_set_font_options(pCR, pFO);

                // Try to select custom font face
                X11Display::font_t *font = pDisplay->get_font(f.get_name());
                if (font != NULL)
                {
                    size_t index = (f.is_bold())    ? 0x1 : 0;
                    index       |= (f.is_italic())  ? 0x2 : 0;

                    cairo_font_face_t *ff = font->cr_face[index];
                    if (ff == NULL)
                    {
                        ff = cairo_ft_font_face_create_for_ft_face(font->ft_face, 0);
                        if (ff != NULL)
                        {
                            cairo_status_t cr_status = cairo_font_face_set_user_data (
                                ff, &pDisplay->sCairoUserDataKey,
                                font, (cairo_destroy_func_t) X11Display::destroy_font_object
                            );

                            if (cr_status)
                            {
                                lsp_error("FT_MANAGE Error creating cairo font face for font '%s', error=%d", font->name, int(cr_status));
                                cairo_font_face_destroy(ff);
                                ff = NULL;
                            }
                        }

                        if (ff != NULL)
                        {
                            // Increment number of references (used by cairo)
                            font->cr_face[index]    = ff;
                            ++font->refs;

                            if (f.is_bold())
                                cairo_ft_font_face_set_synthesize(ff, CAIRO_FT_SYNTHESIZE_BOLD);
                            if (f.is_italic())
                                cairo_ft_font_face_set_synthesize(ff, CAIRO_FT_SYNTHESIZE_OBLIQUE);
                        }
                    }

                    if (ff != NULL)
                    {
                        cairo_set_font_face(pCR, ff);
                        cairo_set_font_size(pCR, f.get_size());

                        ctx->font   = font;
                        ctx->face   = ff;
                        return;
                    }
                }

                // Try to select fall-back font face
                cairo_select_font_face(pCR, f.get_name(),
                    (f.is_italic()) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
                    (f.is_bold()) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL
                );
                cairo_set_font_size(pCR, f.get_size());

                ctx->font   = NULL;
                ctx->face   = cairo_get_font_face(pCR);

                return;
            }

            void X11CairoSurface::unset_current_font(font_context_t *ctx)
            {
                cairo_font_options_set_antialias(pFO, ctx->aa);
                cairo_set_font_face(pCR, NULL);

                ctx->face   = NULL;
                ctx->font   = NULL;
                ctx->aa     = CAIRO_ANTIALIAS_DEFAULT;
            }

            void X11CairoSurface::clear_rgb(uint32_t rgb)
            {
                if (pCR == NULL)
                    return;

                cairo_operator_t op = cairo_get_operator(pCR);
                ::cairo_set_operator (pCR, CAIRO_OPERATOR_SOURCE);
                ::cairo_set_source_rgba(pCR,
                    float((rgb >> 16) & 0xff)/255.0f,
                    float((rgb >> 8) & 0xff)/255.0f,
                    float(rgb & 0xff)/255.0f,
                    0.0f
                );
                ::cairo_paint(pCR);
                ::cairo_set_operator (pCR, op);
            }

            void X11CairoSurface::clear_rgba(uint32_t rgba)
            {
                if (pCR == NULL)
                    return;

                cairo_operator_t op = cairo_get_operator(pCR);
                ::cairo_set_operator (pCR, CAIRO_OPERATOR_SOURCE);
                ::cairo_set_source_rgba(pCR,
                    float((rgba >> 16) & 0xff)/255.0f,
                    float((rgba >> 8) & 0xff)/255.0f,
                    float(rgba & 0xff)/255.0f,
                    float((rgba >> 24) & 0xff)/255.0f
                );
                ::cairo_paint(pCR);
                ::cairo_set_operator (pCR, op);
            }

            inline void X11CairoSurface::setSourceRGB(const Color &col)
            {
                if (pCR == NULL)
                    return;
                float r, g, b;
                col.get_rgb(r, g, b);
                ::cairo_set_source_rgb(pCR, r, g, b);
            }

            inline void X11CairoSurface::setSourceRGBA(const Color &col)
            {
                if (pCR == NULL)
                    return;
                float r, g, b, o;
                col.get_rgbo(r, g, b, o);
                ::cairo_set_source_rgba(pCR, r, g, b, o);
            }

            void X11CairoSurface::clear(const Color &color)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(color);
                cairo_operator_t op = ::cairo_get_operator(pCR);
                ::cairo_set_operator (pCR, CAIRO_OPERATOR_SOURCE);
                ::cairo_paint(pCR);
                ::cairo_set_operator (pCR, op);
            }

            void X11CairoSurface::drawRoundRect(float xmin, float ymin, float width, float height, float radius, size_t mask)
            {
                if ((!(mask & SURFMASK_ALL_CORNER)) || (radius <= 0.0f))
                {
                    cairo_rectangle(pCR, xmin, ymin, width, height);
                    return;
                }

                const float xmax = xmin + width;
                const float ymax = ymin + height;

                if (mask & SURFMASK_LT_CORNER)
                {
                    cairo_move_to(pCR, xmin, ymin + radius);
                    cairo_arc(pCR, xmin + radius, ymin + radius, radius, M_PI, 1.5f*M_PI);
                }
                else
                    cairo_move_to(pCR, xmin, ymin);

                if (mask & SURFMASK_RT_CORNER)
                    cairo_arc(pCR, xmax - radius, ymin + radius, radius, 1.5f * M_PI, 2.0f * M_PI);
                else
                    cairo_line_to(pCR, xmax, ymin);

                if (mask & SURFMASK_RB_CORNER)
                    cairo_arc(pCR, xmax - radius, ymax - radius, radius, 0.0f, 0.5f * M_PI);
                else
                    cairo_line_to(pCR, xmax, ymax);

                if (mask & SURFMASK_LB_CORNER)
                    cairo_arc(pCR, xmin + radius, ymax - radius, radius, 0.5f * M_PI, M_PI);
                else
                    cairo_line_to(pCR, xmin, ymax);

                cairo_close_path(pCR);
            }

            void X11CairoSurface::wire_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(color);
                double w = cairo_get_line_width(pCR);
                cairo_line_join_t j = cairo_get_line_join(pCR);
                cairo_set_line_join(pCR, CAIRO_LINE_JOIN_MITER);

                float lw2 = line_width * 0.5f;
                cairo_set_line_width(pCR, line_width);
                drawRoundRect(left + lw2, top + lw2, width - line_width, height - line_width, radius, mask);

                cairo_stroke(pCR);
                cairo_set_line_width(pCR, w);
                cairo_set_line_join(pCR, j);
            }

            void X11CairoSurface::wire_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(color);
                double w = cairo_get_line_width(pCR);
                cairo_line_join_t j = cairo_get_line_join(pCR);
                cairo_set_line_join(pCR, CAIRO_LINE_JOIN_MITER);

                float lw2 = line_width * 0.5f;
                cairo_set_line_width(pCR, line_width);
                drawRoundRect(r->nLeft+ lw2, r->nTop + lw2, r->nWidth - line_width, r->nHeight - line_width, radius, mask);

                cairo_stroke(pCR);
                cairo_set_line_width(pCR, w);
                cairo_set_line_join(pCR, j);
            }

            void X11CairoSurface::wire_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);
                double w = cairo_get_line_width(pCR);
                cairo_line_join_t j = cairo_get_line_join(pCR);
                cairo_set_line_join(pCR, CAIRO_LINE_JOIN_MITER);

                float lw2 = line_width * 0.5f;
                cairo_set_line_width(pCR, line_width);
                cg->apply(pCR);
                drawRoundRect(r->nLeft + lw2, r->nTop + lw2, r->nWidth - line_width, r->nHeight - line_width, radius, mask);

                cairo_stroke(pCR);
                cairo_set_line_width(pCR, w);
                cairo_set_line_join(pCR, j);
            }

            void X11CairoSurface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);

                double w = cairo_get_line_width(pCR);
                cairo_line_join_t j = cairo_get_line_join(pCR);
                cairo_set_line_join(pCR, CAIRO_LINE_JOIN_MITER);

                float lw2 = line_width * 0.5f;
                cairo_set_line_width(pCR, line_width);
                cg->apply(pCR);
                drawRoundRect(left + lw2, top + lw2, width - line_width, height - line_width, radius, mask);

                cairo_stroke(pCR);
                cairo_set_line_width(pCR, w);
                cairo_set_line_join(pCR, j);
            }

            void X11CairoSurface::fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(color);
                drawRoundRect(left, top, width, height, radius, mask);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r)
            {
                if (pCR == NULL)
                    return;
                setSourceRGBA(color);
                drawRoundRect(r->nLeft, r->nTop, r->nWidth, r->nHeight, radius, mask);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);
                cg->apply(pCR);
                drawRoundRect(left, top, width, height, radius, mask);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);
                cg->apply(pCR);
                drawRoundRect(r->nLeft, r->nTop, r->nWidth, r->nHeight, radius, mask);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_sector(const Color &c, float cx, float cy, float radius, float angle1, float angle2)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(c);
                cairo_move_to(pCR, cx, cy);
                cairo_arc(pCR, cx, cy, radius, angle1, angle2);
                cairo_close_path(pCR);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);
                cg->apply(pCR);
                cairo_move_to(pCR, x0, y0);
                cairo_line_to(pCR, x1, y1);
                cairo_line_to(pCR, x2, y2);
                cairo_close_path(pCR);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(c);
                cairo_move_to(pCR, x0, y0);
                cairo_line_to(pCR, x1, y1);
                cairo_line_to(pCR, x2, y2);
                cairo_close_path(pCR);
                cairo_fill(pCR);
            }

            bool X11CairoSurface::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                cairo_font_extents_t fe;
                fe.ascent           = 0.0;
                fe.descent          = 0.0;
                fe.height           = 0.0;
                fe.max_x_advance    = 0.0;
                fe.max_y_advance    = 0.0;

                if ((pCR != NULL) && (f.get_name() != NULL))
                {
                    // Set current font
                    font_context_t ctx;
                    set_current_font(&ctx, f);
                    {
                        // Get font parameters
                        cairo_font_extents(pCR, &fe);
                    }
                    unset_current_font(&ctx);
                }

                // Return result
                fp->Ascent          = fe.ascent;
                fp->Descent         = fe.descent;
                fp->Height          = fe.height;
                fp->MaxXAdvance     = fe.max_x_advance;
                fp->MaxYAdvance     = fe.max_y_advance;

                return true;
            }

            bool X11CairoSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
            {
                // Initialize data structure
                cairo_text_extents_t te;
                te.x_bearing        = 0.0;
                te.y_bearing        = 0.0;
                te.width            = 0.0;
                te.height           = 0.0;
                te.x_advance        = 0.0;
                te.y_advance        = 0.0;

                if ((pCR != NULL) && (f.get_name() != NULL))
                {
                    // Set current font
                    font_context_t ctx;
                    set_current_font(&ctx, f);
                    {
                        // Get text parameters
                        cairo_glyph_t *glyphs = NULL;
                        int num_glyphs = 0;

                        cairo_scaled_font_t *scaled_font = cairo_get_scaled_font(pCR);
                        cairo_scaled_font_text_to_glyphs(scaled_font, 0.0, 0.0,
                                                   text, -1,
                                                   &glyphs, &num_glyphs,
                                                   NULL, NULL, NULL);
                        cairo_glyph_extents (pCR, glyphs, num_glyphs, &te);
                        cairo_glyph_free(glyphs);
                    }
                    unset_current_font(&ctx);
                }

                // Return result
                tp->XBearing        = te.x_bearing;
                tp->YBearing        = te.y_bearing;
                tp->Width           = te.width;
                tp->Height          = te.height;
                tp->XAdvance        = te.x_advance;
                tp->YAdvance        = te.y_advance;

                return true;
            }

            bool X11CairoSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (text == NULL)
                    return false;
                return get_text_parameters(f, tp, text->get_utf8(first, last));
            }

            void X11CairoSurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
                if ((pCR == NULL) || (f.get_name() == NULL) || (text == NULL))
                    return;

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                {
                    // Draw
                    cairo_move_to(pCR, x, y);
                    setSourceRGBA(color);
                    cairo_show_text(pCR, text);

                    if (f.is_underline())
                    {
                        cairo_text_extents_t te;
                        cairo_text_extents(pCR, text, &te);
                        float width = lsp_max(1.0f, f.get_size() / 12.0f);

                        cairo_set_line_width(pCR, width);

                        cairo_move_to(pCR, x, y + te.y_advance + 1 + width);
                        cairo_line_to(pCR, x + te.x_advance, y + te.y_advance + 1 + width);
                        cairo_stroke(pCR);
                    }
                }
                unset_current_font(&ctx);
            }

            void X11CairoSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (text == NULL)
                    return;
                out_text(f, color, x, y, text->get_utf8(first, last));
            }

            void X11CairoSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
                if ((pCR == NULL) || (f.get_name() == NULL) || (text == NULL))
                    return;

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                {
                    // Output text
                    cairo_text_extents_t extents;
                    cairo_text_extents(pCR, text, &extents);

                    float r_w   = extents.x_advance - extents.x_bearing;
                    float r_h   = extents.y_advance - extents.y_bearing;
                    float fx    = x - extents.x_bearing + (r_w + 4) * 0.5f * dx - r_w * 0.5f;
                    float fy    = y - extents.y_advance + (r_h + 4) * 0.5f * (1.0f - dy) - r_h * 0.5f + 1.0f;

                    cairo_move_to(pCR, fx, fy);
                    cairo_show_text(pCR, text);
                }
                unset_current_font(&ctx);
            }

            void X11CairoSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (text == NULL)
                    return;
                out_text_relative(f, color, x, y, dx, dy, text->get_utf8(first, last));
            }

            void X11CairoSurface::line(const Color &color, float x0, float y0, float x1, float y1, float width)
            {
                if (pCR == NULL)
                    return;

                double ow = cairo_get_line_width(pCR);
                setSourceRGBA(color);
                cairo_set_line_width(pCR, width);
                cairo_move_to(pCR, x0, y0);
                cairo_line_to(pCR, x1, y1);
                cairo_stroke(pCR);
                cairo_set_line_width(pCR, ow);
            }

            void X11CairoSurface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);
                cg->apply(pCR);

                double ow = cairo_get_line_width(pCR);
                cairo_set_line_width(pCR, width);
                cairo_move_to(pCR, x0, y0);
                cairo_line_to(pCR, x1, y1);
                cairo_stroke(pCR);
                cairo_set_line_width(pCR, ow);
            }

            void X11CairoSurface::parametric_line(const Color &color, float a, float b, float c, float width)
            {
                if (pCR == NULL)
                    return;

                double ow = cairo_get_line_width(pCR);
                setSourceRGBA(color);
                cairo_set_line_width(pCR, width);

                if (fabs(a) > fabs(b))
                {
                    cairo_move_to(pCR, - c / a, 0.0f);
                    cairo_line_to(pCR, -(c + b*nHeight)/a, nHeight);
                }
                else
                {
                    cairo_move_to(pCR, 0.0f, - c / b);
                    cairo_line_to(pCR, nWidth, -(c + a*nWidth)/b);
                }

                cairo_stroke(pCR);
                cairo_set_line_width(pCR, ow);
            }

            void X11CairoSurface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
            {
                if (pCR == NULL)
                    return;

                double ow = cairo_get_line_width(pCR);
                setSourceRGBA(color);
                cairo_set_line_width(pCR, width);

                if (fabs(a) > fabs(b))
                {
                    cairo_move_to(pCR, roundf(-(c + b*top)/a), roundf(top));
                    cairo_line_to(pCR, roundf(-(c + b*bottom)/a), roundf(bottom));
                }
                else
                {
                    cairo_move_to(pCR, roundf(left), roundf(-(c + a*left)/b));
                    cairo_line_to(pCR, roundf(right), roundf(-(c + a*right)/b));
                }

                cairo_stroke(pCR);
                cairo_set_line_width(pCR, ow);
            }

            void X11CairoSurface::parametric_bar(
                IGradient *g,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);
                cg->apply(pCR);

                if (fabs(a1) > fabs(b1))
                {
                    cairo_move_to(pCR, ssize_t(-(c1 + b1*top)/a1), ssize_t(top));
                    cairo_line_to(pCR, ssize_t(-(c1 + b1*bottom)/a1), ssize_t(bottom));
                }
                else
                {
                    cairo_move_to(pCR, ssize_t(left), ssize_t(-(c1 + a1*left)/b1));
                    cairo_line_to(pCR, ssize_t(right), ssize_t(-(c1 + a1*right)/b1));
                }

                if (fabs(a2) > fabs(b2))
                {
                    cairo_line_to(pCR, ssize_t(-(c2 + b2*bottom)/a2), ssize_t(bottom));
                    cairo_line_to(pCR, ssize_t(-(c2 + b2*top)/a2), ssize_t(top));
                }
                else
                {
                    cairo_line_to(pCR, ssize_t(right), ssize_t(-(c2 + a2*right)/b2));
                    cairo_line_to(pCR, ssize_t(left), ssize_t(-(c2 + a2*left)/b2));
                }

                cairo_close_path(pCR);
                cairo_fill(pCR);
            }

            void X11CairoSurface::wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width)
            {
                if (pCR == NULL)
                    return;

                double ow = cairo_get_line_width(pCR);
                r = lsp_max(0.0f, r - width * 0.5f);
                setSourceRGBA(c);
                cairo_set_line_width(pCR, width);
                cairo_arc(pCR, x, y, r, a1, a2);
                cairo_stroke(pCR);
                cairo_set_line_width(pCR, ow);
            }

            void X11CairoSurface::fill_poly(const Color & color, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2))
                    return;

                cairo_move_to(pCR, *(x++), *(y++));
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, *(x++), *(y++));

                setSourceRGBA(color);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_poly(IGradient *gr, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2) || (gr == NULL))
                    return;

                cairo_move_to(pCR, *(x++), *(y++));
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, *(x++), *(y++));

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(gr);
                cg->apply(pCR);
                cairo_fill(pCR);
            }

            void X11CairoSurface::wire_poly(const Color & color, float width, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2))
                    return;

                cairo_move_to(pCR, *(x++), *(y++));
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, *(x++), *(y++));

                setSourceRGBA(color);
                cairo_set_line_width(pCR, width);
                cairo_stroke(pCR);
            }

            void X11CairoSurface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2))
                    return;

                cairo_move_to(pCR, *(x++), *(y++));
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, *(x++), *(y++));

                if (width > 0.0f)
                {
                    setSourceRGBA(fill);
                    cairo_fill_preserve(pCR);

                    cairo_set_line_width(pCR, width);
                    setSourceRGBA(wire);
                    cairo_stroke(pCR);
                }
                else
                {
                    setSourceRGBA(fill);
                    cairo_fill(pCR);
                }
            }

            void X11CairoSurface::fill_circle(const Color &c, float x, float y, float r)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(c);
                cairo_arc(pCR, x, y, r, 0.0f, M_PI * 2.0f);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_circle(IGradient *g, float x, float y, float r)
            {
                if (pCR == NULL)
                    return;

                X11CairoGradient *cg = static_cast<X11CairoGradient *>(g);
                cg->apply(pCR);
                cairo_arc(pCR, x, y, r, 0, M_PI * 2.0f);
                cairo_fill(pCR);
            }

            void X11CairoSurface::fill_frame(
                const Color &color,
                size_t flags, float radius,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih)
            {
                if (pCR == NULL)
                    return;

                // Draw the frame
                float fxe = fx + fw, fye = fy + fh, ixe = ix + iw, iye = iy + ih;

                if ((ix >= fxe) || (ixe < fx) || (iy >= fye) || (iye < fy))
                {
                    setSourceRGBA(color);
                    cairo_rectangle(pCR, fx, fy, fw, fh);
                    cairo_fill(pCR);
                    return;
                }
                else if ((ix <= fx) && (ixe >= fxe) && (iy <= fy) && (iye >= fye))
                    return;

                setSourceRGBA(color);
                if (ix <= fx)
                {
                    if (iy <= fy)
                    {
                        cairo_rectangle(pCR, ixe, fy, fxe - ixe, iye - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iye, fw, fye - iye);
                        cairo_fill(pCR);
                    }
                    else if (iye >= fye)
                    {
                        cairo_rectangle(pCR, fx, fy, fw, iy - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, ixe, iy, fxe - ixe, fye - iy);
                        cairo_fill(pCR);
                    }
                    else
                    {
                        cairo_rectangle(pCR, fx, fy, fw, iy - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, ixe, iy, fxe - ixe, ih);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iye, fw, fye - iye);
                        cairo_fill(pCR);
                    }
                }
                else if (ixe >= fxe)
                {
                    if (iy <= fy)
                    {
                        cairo_rectangle(pCR, fx, fy, ix - fx, iye - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iye, fw, fye - iye);
                        cairo_fill(pCR);
                    }
                    else if (iye >= fye)
                    {
                        cairo_rectangle(pCR, fx, fy, fw, iy - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iy, ix - fx, fye - iy);
                        cairo_fill(pCR);
                    }
                    else
                    {
                        cairo_rectangle(pCR, fx, fy, fw, iy - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iy, ix - fx, ih);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iye, fw, fye - iye);
                        cairo_fill(pCR);
                    }
                }
                else
                {
                    if (iy <= fy)
                    {
                        cairo_rectangle(pCR, fx, fy, ix - fx, iye - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, ixe, fy, fxe - ixe, iye - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iye, fw, fye - iye);
                        cairo_fill(pCR);
                    }
                    else if (iye >= fye)
                    {
                        cairo_rectangle(pCR, fx, fy, fw, iy - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iy, ix - fx, fye - iy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, ixe, iy, fxe - ixe, fye - iy);
                        cairo_fill(pCR);
                    }
                    else
                    {
                        cairo_rectangle(pCR, fx, fy, fw, iy - fy);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iy, ix - fx, ih);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, ixe, iy, fxe - ixe, ih);
                        cairo_fill(pCR);
                        cairo_rectangle(pCR, fx, iye, fw, fye - iye);
                        cairo_fill(pCR);
                    }
                }

                // Ensure that there are no corners
                if ((radius <= 0.0) || (!(flags & SURFMASK_ALL_CORNER)))
                    return;

                // Can draw corners?
                float minw = 0.0f;
                minw += (flags & SURFMASK_L_CORNER) ? radius : 0.0;
                minw += (flags & SURFMASK_R_CORNER) ? radius : 0.0;
                if (iw < minw)
                    return;

                float minh = 0.0f;
                minh += (flags & SURFMASK_T_CORNER) ? radius : 0.0;
                minh += (flags & SURFMASK_B_CORNER) ? radius : 0.0;
                if (ih < minh)
                    return;

                // Draw corners
                if (flags & SURFMASK_LT_CORNER)
                {
                    cairo_move_to(pCR, ix, iy);
                    cairo_line_to(pCR, ix + radius, iy);
                    cairo_arc_negative(pCR, ix + radius, iy + radius, radius, 1.5*M_PI, 1.0*M_PI);
                    cairo_close_path(pCR);
                    cairo_fill(pCR);
                }
                if (flags & SURFMASK_RT_CORNER)
                {
                    cairo_move_to(pCR, ix + iw, iy);
                    cairo_line_to(pCR, ix + iw, iy + radius);
                    cairo_arc_negative(pCR, ix + iw - radius, iy + radius, radius, 2.0*M_PI, 1.5*M_PI);
                    cairo_close_path(pCR);
                    cairo_fill(pCR);
                }
                if (flags & SURFMASK_LB_CORNER)
                {
                    cairo_move_to(pCR, ix, iy + ih);
                    cairo_line_to(pCR, ix, iy + ih - radius);
                    cairo_arc_negative(pCR, ix + radius, iy + ih - radius, radius, 1.0*M_PI, 0.5*M_PI);
                    cairo_close_path(pCR);
                    cairo_fill(pCR);
                }
                if (flags & SURFMASK_RB_CORNER)
                {
                    cairo_move_to(pCR, ix + iw, iy + ih);
                    cairo_line_to(pCR, ix + iw - radius, iy + ih);
                    cairo_arc_negative(pCR, ix + iw - radius, iy + ih - radius, radius, 0.5*M_PI, 0.0);
                    cairo_close_path(pCR);
                    cairo_fill(pCR);
                }
            }

            bool X11CairoSurface::get_antialiasing()
            {
                if (pCR == NULL)
                    return false;

                return cairo_get_antialias(pCR) != CAIRO_ANTIALIAS_NONE;
            }

            bool X11CairoSurface::set_antialiasing(bool set)
            {
                if (pCR == NULL)
                    return false;

                bool old = cairo_get_antialias(pCR) != CAIRO_ANTIALIAS_NONE;
                cairo_set_antialias(pCR, (set) ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);

                return old;
            }

            surf_line_cap_t X11CairoSurface::get_line_cap()
            {
                if (pCR == NULL)
                    return SURFLCAP_BUTT;

                cairo_line_cap_t old = cairo_get_line_cap(pCR);

                return
                    (old == CAIRO_LINE_CAP_BUTT) ? SURFLCAP_BUTT :
                    (old == CAIRO_LINE_CAP_ROUND) ? SURFLCAP_ROUND : SURFLCAP_SQUARE;
            }

            surf_line_cap_t X11CairoSurface::set_line_cap(surf_line_cap_t lc)
            {
                if (pCR == NULL)
                    return SURFLCAP_BUTT;

                cairo_line_cap_t old = cairo_get_line_cap(pCR);

                cairo_line_cap_t cap =
                    (lc == SURFLCAP_BUTT) ? CAIRO_LINE_CAP_BUTT :
                    (lc == SURFLCAP_ROUND) ? CAIRO_LINE_CAP_ROUND :
                    CAIRO_LINE_CAP_SQUARE;

                cairo_set_line_cap(pCR, cap);

                return
                    (old == CAIRO_LINE_CAP_BUTT) ? SURFLCAP_BUTT :
                    (old == CAIRO_LINE_CAP_ROUND) ? SURFLCAP_ROUND : SURFLCAP_SQUARE;
            }

            void *X11CairoSurface::start_direct()
            {
                if ((pCR == NULL) || (pSurface == NULL) || (nType != ST_IMAGE))
                    return NULL;

                nStride = cairo_image_surface_get_stride(pSurface);
                return pData = reinterpret_cast<uint8_t *>(cairo_image_surface_get_data(pSurface));
            }

            void X11CairoSurface::end_direct()
            {
                if ((pCR == NULL) || (pSurface == NULL) || (nType != ST_IMAGE) || (pData == NULL))
                    return;

                cairo_surface_mark_dirty(pSurface);
                pData = NULL;
            }

            void X11CairoSurface::clip_begin(float x, float y, float w, float h)
            {
                if (pCR == NULL)
                    return;

                cairo_save(pCR);
                cairo_rectangle(pCR, x, y, w, h);
                cairo_clip(pCR);
                cairo_new_path(pCR);

            #ifdef LSP_DEBUG
                ++nNumClips;
            #endif /* LSP_DEBUG */
            }

            void X11CairoSurface::clip_end()
            {
                if (pCR == NULL)
                    return;

                cairo_restore(pCR);
            #ifdef LSP_DEBUG
                --nNumClips;
            #endif /* LSP_DEBUG */
            }

        }
    }

} /* namespace lsp */

#endif /* defined(USE_LIBX11) && defined(USE_LIBCAIRO) */
