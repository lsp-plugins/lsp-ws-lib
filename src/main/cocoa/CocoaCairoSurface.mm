/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *           (C) 2025 Marvin Edeler <marvin.edeler@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 12 June 2025
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

#ifdef PLATFORM_MACOSX

#import <CoreGraphics/CoreGraphics.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/math.h>

#include <cairo.h>
#include <cairo-quartz.h>

#include <private/freetype/FontManager.h>
#include <private/cocoa/CocoaCairoGradient.h>
#include <private/cocoa/CocoaCairoSurface.h>
#include <private/cocoa/CocoaDisplay.h>
#include <private/cocoa/CocoaCairoView.h>

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            constexpr float k_color = 1.0f / 255.0f;

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

            CocoaCairoSurface::CocoaCairoSurface(CocoaDisplay *dpy, CocoaCairoView *view, size_t width, size_t height):
                ISurface(width, height, ST_IMAGE)
            {
                lsp_trace("Surface %p constructed with view %p", this, view);
                pDisplay        = dpy;
                pCocoaView      = view;
                pCR             = NULL;
                pFO             = NULL;
                pRoot           = NULL;
                pSurface        = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                fOriginX        = 0.0f;
                fOriginY        = 0.0f;

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif 
            }

            CocoaCairoSurface::CocoaCairoSurface(CocoaDisplay *dpy, size_t width, size_t height):
                ISurface(width, height, ST_IMAGE)
            {
                pDisplay        = dpy;
                pCR             = NULL;
                pFO             = NULL;
                pRoot           = NULL;
                pSurface        = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                fOriginX        = 0.0f;
                fOriginY        = 0.0f;

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            CocoaCairoSurface::CocoaCairoSurface(CocoaDisplay *dpy, cairo_surface_t *surface, size_t width, size_t height):
                ISurface(width, height, ST_SIMILAR)
            {
                pDisplay        = dpy;
                pCocoaView      = NULL;
                pCR             = NULL;
                pFO             = NULL;
                pRoot           = NULL;
                pSurface        = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                fOriginX        = 0.0f;
                fOriginY        = 0.0f;

//                pSurface        = ::cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, width, height);
            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            IDisplay *CocoaCairoSurface::display()
            {
                return pDisplay;
            }

            cairo_surface_t *CocoaCairoSurface::get_image_surface()
            {
                if (!pSurface || cairo_surface_get_type(pSurface) != CAIRO_SURFACE_TYPE_IMAGE)
                    return nullptr;

                int width = cairo_image_surface_get_width(pSurface);
                int height = cairo_image_surface_get_height(pSurface);
                cairo_format_t format = cairo_image_surface_get_format(pSurface);

                // Create new surface with same format and size
                cairo_surface_t *copy = cairo_image_surface_create(format, width, height);
                if (!copy)
                    return nullptr;

                // Create a cairo context for the destination surface
                cairo_t *cr = cairo_create(copy);
                cairo_set_source_surface(cr, pSurface, 0, 0);
                cairo_paint(cr);
                cairo_destroy(cr);

                return copy;
            }

            ISurface *CocoaCairoSurface::create(size_t width, size_t height)
            {
                return new CocoaCairoSurface(pDisplay, pSurface, width, height);
            }

            IGradient *CocoaCairoSurface::linear_gradient(float x0, float y0, float x1, float y1)
            {
                return new CocoaCairoGradient(
                    CocoaCairoGradient::linear_t {
                        x0, y0,
                        x1, y1});
            }

            IGradient *CocoaCairoSurface::radial_gradient(float cx0, float cy0, float cx1, float cy1, float r)
            {
                return new CocoaCairoGradient(
                    CocoaCairoGradient::radial_t {
                        cx0, cy0,
                        cx1, cy1,
                        r});
            }

            CocoaCairoSurface::~CocoaCairoSurface()
            {
                destroy_context(true);
            }

            void CocoaCairoSurface::destroy_context(bool root)
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
                if ((pRoot != NULL) && (root))
                {
                    cairo_surface_destroy(pSurface);
                    pRoot           = NULL;
                }
            }

            void CocoaCairoSurface::destroy()
            {
                if (pCocoaView != NULL)
                {
                    [[NSNotificationCenter defaultCenter] removeObserver:pCocoaView];
                }
                destroy_context(true);
            }

            bool CocoaCairoSurface::valid() const
            {
                return pSurface != NULL;
            }

            status_t CocoaCairoSurface::resize(size_t width, size_t height)
            {
                if (pCR != NULL)
                    return STATUS_BAD_STATE;

                // Create new surface and cairo
                cairo_surface_t *s  = NULL;
                if ((nType == ST_IMAGE) || (nType == ST_XLIB))
                    s  = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                else if (nType == ST_SIMILAR)
                    s  = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
//                    s  = ::cairo_surface_create_similar(pSurface, CAIRO_CONTENT_COLOR_ALPHA, width, height);

                if (s == NULL)
                    return STATUS_NO_MEM;

                // Destroy previously used context and update surface pointer
                destroy_context(false);
                pSurface            = s;
                nWidth              = width;
                nHeight             = height;

                return STATUS_OK;
            }

            void CocoaCairoSurface::draw(ISurface *s, float x, float y, float sx, float sy, float a)
            {
                if (pCR == NULL)
                    return;
                surface_type_t type = s->type();
                if ((type != ST_IMAGE) && (type != ST_SIMILAR))
                    return;

                CocoaCairoSurface *cs = static_cast<CocoaCairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                float sw = fabsf(sx * s->width()), sh = fabsf(sy * s->height());
                ::cairo_save(pCR);
                lsp_finally { ::cairo_restore(pCR); };

                ::cairo_rectangle(pCR, x, y, sw, sh);
                ::cairo_clip(pCR);
                
                if ((sx != 1.0f) && (sy != 1.0f))
                {
                    if (sx < 0.0f)
                        x       -= sx * s->width();
                    if (sy < 0.0f)
                        y       -= sy * s->height();

                    ::cairo_translate(pCR, x, y);
                    ::cairo_scale(pCR, sx, sy);

                    ::cairo_set_source_surface(pCR, cs->pSurface, 0.0f, 0.0f);
                }
                else
                    ::cairo_set_source_surface(pCR, cs->pSurface, x, y);
                
                /*
                if ((sx != 1.0f) || (sy != 1.0f))
                {
                    if (sx < 0.0f)
                        x -= sx * s->width();
                    if (sy < 0.0f)
                        y -= sy * s->height();

                    ::cairo_translate(pCR, x, y);

                    ::cairo_translate(pCR, x, y + (sy * s->height()));
                    ::cairo_scale(pCR, sx, -sy);
                    ::cairo_set_source_surface(pCR, cs->pSurface, 0.0f, 0.0f);
                }
                else
                {
                    // If scaling is 1, still need to flip Y for Cocoa
                    ::cairo_translate(pCR, x, y + s->height());
                    ::cairo_scale(pCR, 1.0f, -1.0f);
                    ::cairo_set_source_surface(pCR, cs->pSurface, 0.0f, 0.0f);
                } */

                // Draw the surface
                if (a > 0.0f)
                    ::cairo_paint_with_alpha(pCR, 1.0f - a);
                else
                    ::cairo_paint(pCR);
            }

            void CocoaCairoSurface::draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
            {
                surface_type_t type = s->type();
                if ((type != ST_XLIB) && (type != ST_IMAGE) && (type != ST_SIMILAR))
                    return;
                if (pCR == NULL)
                    return;
                CocoaCairoSurface *cs = static_cast<CocoaCairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_save(pCR);
                ::cairo_translate(pCR, x, y);
                ::cairo_scale(pCR, sx, sy);
                ::cairo_rotate(pCR, ra);

                ::cairo_set_source_surface(pCR, cs->pSurface, 0.0f, 0.0f);
                if (a > 0.0f)
                    ::cairo_paint_with_alpha(pCR, 1.0f - a);
                else
                    ::cairo_paint(pCR);
                ::cairo_restore(pCR);
            }

            void CocoaCairoSurface::draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a)
            {
                surface_type_t type = s->type();
                if ((type != ST_XLIB) && (type != ST_IMAGE) && (type != ST_SIMILAR))
                    return;
                if (pCR == NULL)
                    return;
                CocoaCairoSurface *cs = static_cast<CocoaCairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_save(pCR);
                lsp_finally { ::cairo_restore(pCR); };

                ::cairo_rectangle(pCR, x, y, sw, sh);
                ::cairo_clip(pCR);
                ::cairo_set_source_surface(pCR, cs->pSurface, x - sx, y - sy);
                if (a > 0.0f)
                    ::cairo_paint_with_alpha(pCR, 1.0f - a);
                else
                    ::cairo_paint(pCR);
            }

            void CocoaCairoSurface::draw_raw(
                const void *data, size_t width, size_t height, size_t stride,
                float x, float y, float sx, float sy, float a)
            {
                if (pCR == NULL)
                    return;

                cairo_surface_t *cs = cairo_image_surface_create_for_data(
                    static_cast<unsigned char *>(const_cast<void *>(data)),
                    CAIRO_FORMAT_ARGB32,
                    width, height, stride);
                if (cs == NULL)
                    return;
                lsp_finally { cairo_surface_destroy(cs); };

                // Draw one surface on another
                ::cairo_save(pCR);
                lsp_finally { ::cairo_restore(pCR); };

                
                if ((sx != 1.0f) && (sy != 1.0f))
                {
                    if (sx < 0.0f)
                        x       -= sx * width;
                    if (sy < 0.0f)
                        y       -= sy * height;

                    ::cairo_translate(pCR, x, y);
                    ::cairo_scale(pCR, sx, sy);
                    ::cairo_set_source_surface(pCR, cs, 0.0f, 0.0f);
                }
                else
                    ::cairo_set_source_surface(pCR, cs, x, y);

                // Draw the surface
                if (a > 0.0f)
                    ::cairo_paint_with_alpha(pCR, 1.0f - a);
                else
                    ::cairo_paint(pCR);
            }

            void CocoaCairoSurface::begin()
            {
                if (pCocoaView != nullptr)
                    lsp_trace("Surface %p begin, pCocoaView=%p, w=%zu, h=%zu", this, pCocoaView, nWidth, nHeight);
                
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
                ::cairo_set_antialias(pCR, CAIRO_ANTIALIAS_FAST);
                ::cairo_set_line_join(pCR, CAIRO_LINE_JOIN_BEVEL);
                ::cairo_set_tolerance(pCR, 0.5);

                // In CairoView/Window we have also to flip Y for draw from topleft
                if (pCocoaView) {
                    ::cairo_translate(pCR, 0, nHeight);
                    ::cairo_scale(pCR, 1, -1);
                }

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            void CocoaCairoSurface::end()
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

                //lsp_trace("Before autoreleasepool: %p", pCocoaView);
                @autoreleasepool {
                    if (pCocoaView != NULL)
                    {
                        cairo_surface_t *exposeSurface = get_image_surface();

                        if (exposeSurface != NULL)
                        {
                            lsp_trace("Surface %p end, Send event for pCocoaView=%p", this, pCocoaView);
                            [[NSNotificationCenter defaultCenter] postNotificationName:@"ForceExpose"
                                                        object:pCocoaView
                                                        userInfo:@{@"Surface": [NSValue valueWithPointer: exposeSurface]}];
                        }
                        
                        //[[pCocoaWindow contentView] setImage: pSurface]; 
                        //[[pCocoaWindow contentView] triggerRedraw]; 
                    } 
                }

                ::cairo_surface_flush(pSurface);

                // Copy back surface to front surface if it is present
                if (pRoot != NULL)
                {
                    cairo_t *cr = ::cairo_create(pRoot);
                    if (cr == NULL)
                        return;
                    lsp_finally {
                        cairo_destroy(pCR);
                    };

                    ::cairo_set_source_surface(cr, pSurface, 0, 0);
                    ::cairo_paint(cr);
                    ::cairo_surface_flush(pRoot);
                }
            }

            void CocoaCairoSurface::set_current_font(font_context_t *ctx, const Font &f)
            {
                // Apply antialiasint to the font
                ctx->aa     = cairo_font_options_get_antialias(pFO);
                cairo_font_options_set_antialias(pFO, decode_antialiasing(f));
                cairo_set_font_options(pCR, pFO);

                cairo_select_font_face(pCR, f.get_name(),
                    (f.is_italic()) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
                    (f.is_bold()) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL
                );
                cairo_set_font_size(pCR, f.get_size());

                ctx->face   = cairo_get_font_face(pCR);

                return;
            }

            void CocoaCairoSurface::unset_current_font(font_context_t *ctx)
            {
                cairo_font_options_set_antialias(pFO, ctx->aa);
                cairo_set_font_face(pCR, NULL);

                ctx->face   = NULL;
                ctx->aa     = CAIRO_ANTIALIAS_DEFAULT;
            }

            void CocoaCairoSurface::clear_rgb(uint32_t rgb)
            {
                if (pCR == NULL)
                    return;

                cairo_operator_t op = cairo_get_operator(pCR);
                ::cairo_set_operator (pCR, CAIRO_OPERATOR_SOURCE);
                ::cairo_set_source_rgba(pCR,
                    float((rgb >> 16) & 0xff) * k_color,
                    float((rgb >> 8) & 0xff) * k_color,
                    float(rgb & 0xff) * k_color,
                    0.0f
                );
                ::cairo_paint(pCR);
                ::cairo_set_operator (pCR, op);
            }

            void CocoaCairoSurface::clear_rgba(uint32_t rgba)
            {
                if (pCR == NULL)
                    return;

                cairo_operator_t op = cairo_get_operator(pCR);
                ::cairo_set_operator (pCR, CAIRO_OPERATOR_SOURCE);
                ::cairo_set_source_rgba(pCR,
                    float((rgba >> 16) & 0xff) * k_color,
                    float((rgba >> 8) & 0xff) * k_color,
                    float(rgba & 0xff) * k_color,
                    float((rgba >> 24) & 0xff) * k_color
                );
                ::cairo_paint(pCR);
                ::cairo_set_operator (pCR, op);
            }

            inline void CocoaCairoSurface::setSourceRGB(const Color &col)
            {
                if (pCR == NULL)
                    return;
                float r, g, b;
                col.get_rgb(r, g, b);
                ::cairo_set_source_rgb(pCR, r, g, b);
            }

            inline void CocoaCairoSurface::setSourceRGBA(const Color &col)
            {
                if (pCR == NULL)
                    return;
                float r, g, b, o;
                col.get_rgbo(r, g, b, o);
                ::cairo_set_source_rgba(pCR, r, g, b, o);
            }

            void CocoaCairoSurface::clear(const Color &color)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(color);
                cairo_operator_t op = ::cairo_get_operator(pCR);
                ::cairo_set_operator (pCR, CAIRO_OPERATOR_SOURCE);
                ::cairo_paint(pCR);
                ::cairo_set_operator (pCR, op);
            }

            void CocoaCairoSurface::drawRoundRect(float xmin, float ymin, float width, float height, float radius, size_t mask)
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

            void CocoaCairoSurface::wire_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height, float line_width)
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

            void CocoaCairoSurface::wire_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
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

            void CocoaCairoSurface::wire_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);
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

            void CocoaCairoSurface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);

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

            void CocoaCairoSurface::fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(color);
                drawRoundRect(left, top, width, height, radius, mask);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r)
            {
                if (pCR == NULL)
                    return;
                setSourceRGBA(color);
                drawRoundRect(r->nLeft, r->nTop, r->nWidth, r->nHeight, radius, mask);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);
                cg->apply(pCR);
                drawRoundRect(left, top, width, height, radius, mask);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);
                cg->apply(pCR);
                drawRoundRect(r->nLeft, r->nTop, r->nWidth, r->nHeight, radius, mask);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, float left, float top, float width, float height)
            {
                if (pCR == NULL)
                    return;
                surface_type_t type = s->type();
                if ((type != ST_IMAGE) && (type != ST_SIMILAR))
                    return;

                CocoaCairoSurface *cs = static_cast<CocoaCairoSurface *>(s);
                if (cs->pSurface == NULL)
                    return;

                // Draw one surface on another
                ::cairo_save(pCR);
                lsp_finally { ::cairo_restore(pCR); };

                cairo_pattern_t *p = ::cairo_pattern_create_for_surface(cs->pSurface);
                if (p == NULL)
                    return;
                lsp_finally { ::cairo_pattern_destroy(p); };

                cairo_matrix_t matrix;
                matrix.xx       = 1.0f;
                matrix.xy       = 0.0f;
                matrix.x0       = -(fOriginX + left);

                matrix.yx       = 0.0f;
                matrix.yy       = 1.0f;
                matrix.y0       = -(fOriginY + top);

                ::cairo_pattern_set_matrix(p, &matrix);
                ::cairo_pattern_set_extend(p, CAIRO_EXTEND_NONE);
                ::cairo_pattern_set_filter(p, CAIRO_FILTER_BILINEAR);

                ::cairo_set_source(pCR, p);
                drawRoundRect(left, top, width, height, radius, mask);
                ::cairo_clip(pCR);
                ::cairo_paint_with_alpha(pCR, 1.0f - alpha);
            }

            void CocoaCairoSurface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, const ws::rectangle_t *r)
            {
                fill_rect(s, alpha, mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight);
            }

            void CocoaCairoSurface::fill_sector(const Color &c, float x, float y, float r, float a1, float a2)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(c);
                if (fabsf(a2 - a1) < 2.0f * M_PI)
                {
                    cairo_move_to(pCR, x, y);

                    if (a2 < a1)
                        cairo_arc_negative(pCR, x, y, r, a1, a2);
                    else
                        cairo_arc(pCR, x, y, r, a1, a2);
                }
                else
                    cairo_arc(pCR, x, y, r, 0.0f, M_PI * 2.0f);
                cairo_close_path(pCR);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);
                cg->apply(pCR);
                cairo_move_to(pCR, x0, y0);
                cairo_line_to(pCR, x1, y1);
                cairo_line_to(pCR, x2, y2);
                cairo_close_path(pCR);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
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

            bool CocoaCairoSurface::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                // Get font parameter using font manager
            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pDisplay->font_manager();
                if (mgr != NULL)
                {
                    if (mgr->get_font_parameters(&f, fp))
                        return true;
                }
            #endif /* USE_LIBFREETYPE */

                // Do the usual job using Cairo
                if ((pCR == NULL) || (f.get_name() == NULL))
                {
                    fp->Ascent          = 0.0f;
                    fp->Descent         = 0.0f;
                    fp->Height          = 0.0f;
                    return true;
                }

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                lsp_finally { unset_current_font(&ctx); };

                // Get font parameters
                cairo_font_extents_t fe;
                cairo_font_extents(pCR, &fe);

                fp->Ascent          = fe.ascent;
                fp->Descent         = fe.descent;
                fp->Height          = fe.height;

                return true;
            }

            bool CocoaCairoSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
            {
                if (text == NULL)
                    return false;

                // Get text parameter using font manager
            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pDisplay->font_manager();
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

                if ((pCR == NULL) || (f.get_name() == NULL))
                {
                    tp->XBearing        = 0.0f;
                    tp->YBearing        = 0.0f;
                    tp->Width           = 0.0f;
                    tp->Height          = 0.0f;
                    tp->XAdvance        = 0.0f;
                    tp->YAdvance        = 0.0f;

                    return true;
                }

                // Initialize data structure
                cairo_text_extents_t te;

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                lsp_finally { unset_current_font(&ctx); };

                // Get text parameters
                cairo_text_extents(pCR, text, &te);
                tp->XBearing        = te.x_bearing;
                tp->YBearing        = te.y_bearing;
                tp->Width           = te.width;
                tp->Height          = te.height;
                tp->XAdvance        = te.x_advance;
                tp->YAdvance        = te.y_advance;

                return true;
            }

            bool CocoaCairoSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (text == NULL)
                    return false;

                // Get text parameter using font manager
            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pDisplay->font_manager();
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

                if ((pCR == NULL) || (f.get_name() == NULL))
                {
                    tp->XBearing        = 0.0f;
                    tp->YBearing        = 0.0f;
                    tp->Width           = 0.0f;
                    tp->Height          = 0.0f;
                    tp->XAdvance        = 0.0f;
                    tp->YAdvance        = 0.0f;

                    return true;
                }

                // Initialize data structure
                cairo_text_extents_t te;

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                lsp_finally { unset_current_font(&ctx); };

                // Get text parameters
                cairo_text_extents(pCR, text->get_utf8(first, last), &te);
                tp->XBearing        = te.x_bearing;
                tp->YBearing        = te.y_bearing;
                tp->Width           = te.width;
                tp->Height          = te.height;
                tp->XAdvance        = te.x_advance;
                tp->YAdvance        = te.y_advance;

                return true;
            }

            void CocoaCairoSurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
                if ((pCR == NULL) || (f.get_name() == NULL) || (text == NULL))
                    return;

            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pDisplay->font_manager();
                if (mgr != NULL)
                {
                    LSPString tmp;
                    if (!tmp.set_utf8(text))
                        return;

                    ft::text_range_t tr;
                    dsp::bitmap_t *bitmap   = mgr->render_text(&f, &tr, &tmp, 0, tmp.length());
                    if (bitmap != NULL)
                    {
                        lsp_finally { ft::free_bitmap(bitmap); };

                        // Draw the text bitmap at the specified position
                        cairo_surface_t *fs = cairo_image_surface_create_for_data(
                            bitmap->data,
                            CAIRO_FORMAT_A8,
                            bitmap->width,
                            bitmap->height,
                            bitmap->stride);
                        if (fs == NULL)
                            return;
                        lsp_finally{ cairo_surface_destroy(fs); };

                        setSourceRGBA(color);
                        const float sx  = x + tr.x_bearing;
                        const float sy  = y + tr.y_bearing;
                        cairo_mask_surface(pCR, fs, sx, sy);

                        // Draw underline if required
                        if (f.is_underline())
                        {
                            const float width = lsp_max(1.0f, f.get_size() / 12.0f);
                            const float bottom  = y + width * 1.5f;

                            cairo_set_line_width(pCR, width);
                            cairo_move_to(pCR, sx, bottom);
                            cairo_line_to(pCR, sx + tr.x_advance, bottom);
                            cairo_stroke(pCR);
                        }

                        return;
                    }
                }
            #endif /* USE_LIBFREETYPE */

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                lsp_finally { unset_current_font(&ctx); };

                // Draw
                cairo_move_to(pCR, x, y);
                setSourceRGBA(color);
                cairo_show_text(pCR, text);

                // Draw underline if required
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

            void CocoaCairoSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                if ((pCR == NULL) || (f.get_name() == NULL) || (text == NULL))
                    return;

            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pDisplay->font_manager();
                if (mgr != NULL)
                {
                    ft::text_range_t tr;
                    dsp::bitmap_t *bitmap   = mgr->render_text(&f, &tr, text, first, last);
                    if (bitmap != NULL)
                    {
                        lsp_finally { ft::free_bitmap(bitmap); };

                        // Draw the text bitmap at the specified position
                        cairo_surface_t *fs = cairo_image_surface_create_for_data(
                            bitmap->data,
                            CAIRO_FORMAT_A8,
                            bitmap->width,
                            bitmap->height,
                            bitmap->stride);
                        if (fs == NULL)
                            return;
                        lsp_finally{ cairo_surface_destroy(fs); };

                        setSourceRGBA(color);
                        const float sx = x + tr.x_bearing;
                        const float sy = y + tr.y_bearing;
                        cairo_mask_surface(pCR, fs, sx, sy);

                        // Draw underline if required
                        if (f.is_underline())
                        {
                            const float width = lsp_max(1.0f, f.get_size() / 12.0f);
                            const float bottom  = y + width * 1.5f;

                            cairo_set_line_width(pCR, width);
                            cairo_move_to(pCR, sx, bottom);
                            cairo_line_to(pCR, sx + tr.x_advance, bottom);
                            cairo_stroke(pCR);
                        }

                        return;
                    }
                }
            #endif /* USE_LIBFREETYPE */

                const char *utf8_text = text->get_utf8(first, last);
                if (utf8_text == NULL)
                    return;

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                lsp_finally { unset_current_font(&ctx); };

                // Draw
                cairo_move_to(pCR, x, y);
                setSourceRGBA(color);
                cairo_show_text(pCR, utf8_text);

                // Draw underline if required
                if (f.is_underline())
                {
                    cairo_text_extents_t te;
                    cairo_text_extents(pCR, utf8_text, &te);
                    float width = lsp_max(1.0f, f.get_size() / 12.0f);

                    cairo_set_line_width(pCR, width);

                    cairo_move_to(pCR, x, y + te.y_advance + 1 + width);
                    cairo_line_to(pCR, x + te.x_advance, y + te.y_advance + 1 + width);
                    cairo_stroke(pCR);
                }
            }

            void CocoaCairoSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
                if ((pCR == NULL) || (f.get_name() == NULL) || (text == NULL))
                    return;

                float r_w, r_h, fx, fy;

            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pDisplay->font_manager();
                if (mgr != NULL)
                {
                    LSPString tmp;
                    if (!tmp.set_utf8(text))
                        return;

                    ft::text_range_t tr;
                    dsp::bitmap_t *bitmap   = mgr->render_text(&f, &tr, &tmp, 0, tmp.length());
                    if (bitmap != NULL)
                    {
                        lsp_finally { ft::free_bitmap(bitmap); };

                        // Draw the text bitmap at the specified position
                        cairo_surface_t *fs = cairo_image_surface_create_for_data(
                            bitmap->data,
                            CAIRO_FORMAT_A8,
                            bitmap->width,
                            bitmap->height,
                            bitmap->stride);
                        if (fs == NULL)
                            return;
                        lsp_finally{ cairo_surface_destroy(fs); };

                        setSourceRGBA(color);
                        r_w   = tr.x_advance;
                        r_h   = -tr.y_bearing;
                        fx    = truncf(x - tr.x_bearing - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx);
                        fy    = truncf(y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy);
                        cairo_mask_surface(pCR, fs, fx + tr.x_bearing, fy + tr.y_bearing);

                        // Draw underline if required
                        if (f.is_underline())
                        {
                            const float width = lsp_max(1.0f, f.get_size() / 12.0f);
                            const float bottom  = fy + width * 1.5f;

                            cairo_set_line_width(pCR, width);
                            cairo_move_to(pCR, fx, bottom);
                            cairo_line_to(pCR, fx + tr.x_advance, bottom);
                            cairo_stroke(pCR);
                        }

                        return;
                    }
                }
            #endif /* USE_LIBFREETYPE */

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                lsp_finally { unset_current_font(&ctx); };

                // Output text
                cairo_text_extents_t te;
                cairo_text_extents(pCR, text, &te);

                r_w   = te.x_advance;
                r_h   = -te.y_bearing;
                fx    = truncf(x - te.x_bearing - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx);
                fy    = truncf(y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy);

                setSourceRGBA(color);
                cairo_move_to(pCR, fx, fy);
                cairo_show_text(pCR, text);

                // Draw underline if required
                if (f.is_underline())
                {
                    float width = lsp_max(1.0f, f.get_size() / 12.0f);

                    cairo_set_line_width(pCR, width);
                    cairo_move_to(pCR, fx, fy + te.y_advance + 1 + width);
                    cairo_line_to(pCR, fx + te.x_advance, fy + te.y_advance + 1 + width);
                    cairo_stroke(pCR);
                }
            }

            void CocoaCairoSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                if ((pCR == NULL) || (f.get_name() == NULL) || (text == NULL))
                    return;

                float r_w, r_h, fx, fy;

            #ifdef USE_LIBFREETYPE
                ft::FontManager *mgr = pDisplay->font_manager();
                if (mgr != NULL)
                {
                    ft::text_range_t tr;
                    dsp::bitmap_t *bitmap   = mgr->render_text(&f, &tr, text, first, last);
                    if (bitmap != NULL)
                    {
                        lsp_finally { ft::free_bitmap(bitmap); };

                        // Draw the text bitmap at the specified position
                        cairo_surface_t *fs = cairo_image_surface_create_for_data(
                            bitmap->data,
                            CAIRO_FORMAT_A8,
                            bitmap->width,
                            bitmap->height,
                            bitmap->stride);
                        if (fs == NULL)
                            return;
                        lsp_finally{ cairo_surface_destroy(fs); };

                        setSourceRGBA(color);
                        r_w   = tr.x_advance;
                        r_h   = -tr.y_bearing;
                        fx    = truncf(x - tr.x_bearing - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx);
                        fy    = truncf(y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy);
                        cairo_mask_surface(pCR, fs, fx + tr.x_bearing, fy + tr.y_bearing);

                        // Draw underline if required
                        if (f.is_underline())
                        {
                            const float width = lsp_max(1.0f, f.get_size() / 12.0f);
                            const float bottom  = fy + width * 1.5f;

                            cairo_set_line_width(pCR, width);
                            cairo_move_to(pCR, fx, bottom);
                            cairo_line_to(pCR, fx + tr.x_advance, bottom);
                            cairo_stroke(pCR);
                        }

                        return;
                    }
                }
            #endif /* USE_LIBFREETYPE */

                const char *utf8_text = text->get_utf8(first, last);
                if (utf8_text == NULL)
                    return;

                // Set current font
                font_context_t ctx;
                set_current_font(&ctx, f);
                lsp_finally { unset_current_font(&ctx); };

                // Output text
                cairo_text_extents_t te;
                cairo_text_extents(pCR, utf8_text, &te);

                r_w   = te.x_advance;
                r_h   = -te.y_bearing;
                fx    = truncf(x - te.x_bearing - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx);
                fy    = truncf(y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy);

                setSourceRGBA(color);
                cairo_move_to(pCR, fx, fy);
                cairo_show_text(pCR, utf8_text);

                // Draw underline if required
                if (f.is_underline())
                {
                    float width = lsp_max(1.0f, f.get_size() / 12.0f);

                    cairo_set_line_width(pCR, width);
                    cairo_move_to(pCR, fx, fy + te.y_advance + 1 + width);
                    cairo_line_to(pCR, fx + te.x_advance, fy + te.y_advance + 1 + width);
                    cairo_stroke(pCR);
                }
            }

            void CocoaCairoSurface::line(const Color &color, float x0, float y0, float x1, float y1, float width)
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

            void CocoaCairoSurface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);
                cg->apply(pCR);

                double ow = cairo_get_line_width(pCR);
                cairo_set_line_width(pCR, width);
                cairo_move_to(pCR, x0, y0);
                cairo_line_to(pCR, x1, y1);
                cairo_stroke(pCR);
                cairo_set_line_width(pCR, ow);
            }

            void CocoaCairoSurface::parametric_line(const Color &color, float a, float b, float c, float width)
            {
                if (pCR == NULL)
                    return;

                double ow = cairo_get_line_width(pCR);
                setSourceRGBA(color);
                cairo_set_line_width(pCR, width);

                if (fabsf(a) > fabsf(b))
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

            void CocoaCairoSurface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
            {
                if (pCR == NULL)
                    return;

                double ow = cairo_get_line_width(pCR);
                setSourceRGBA(color);
                cairo_set_line_width(pCR, width);

                if (fabsf(a) > fabsf(b))
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

            void CocoaCairoSurface::parametric_bar(
                IGradient *g,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);
                cg->apply(pCR);

                if (fabsf(a1) > fabsf(b1))
                {
                    cairo_move_to(pCR, ssize_t(-(c1 + b1*top)/a1), ssize_t(top));
                    cairo_line_to(pCR, ssize_t(-(c1 + b1*bottom)/a1), ssize_t(bottom));
                }
                else
                {
                    cairo_move_to(pCR, ssize_t(left), ssize_t(-(c1 + a1*left)/b1));
                    cairo_line_to(pCR, ssize_t(right), ssize_t(-(c1 + a1*right)/b1));
                }

                if (fabsf(a2) > fabsf(b2))
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

            void CocoaCairoSurface::wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width)
            {
                if (pCR == NULL)
                    return;

                double ow = cairo_get_line_width(pCR);
                r = lsp_max(0.0f, r - width * 0.5f);
                setSourceRGBA(c);
                cairo_set_line_width(pCR, width);
                if (fabsf(a2 - a1) >= 2.0f * M_PI)
                    cairo_arc(pCR, x, y, r, 0.0f, 2.0f * M_PI);
                else if (a2 < a1)
                    cairo_arc_negative(pCR, x, y, r, a1, a2);
                else
                    cairo_arc(pCR, x, y, r, a1, a2);
                cairo_stroke(pCR);
                cairo_set_line_width(pCR, ow);
            }

            void CocoaCairoSurface::fill_poly(const Color & color, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2))
                    return;

                cairo_move_to(pCR, x[0], y[0]);
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, x[i], y[i]);

                setSourceRGBA(color);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_poly(IGradient *gr, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2) || (gr == NULL))
                    return;

                cairo_move_to(pCR, x[0], y[0]);
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, x[i], y[i]);

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(gr);
                cg->apply(pCR);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::wire_poly(const Color & color, float width, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2))
                    return;

                cairo_move_to(pCR, x[0], y[0]);
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, x[i], y[i]);

                setSourceRGBA(color);
                cairo_set_line_width(pCR, width);
                cairo_stroke(pCR);
            }

            void CocoaCairoSurface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
                if ((pCR == NULL) || (n < 2))
                    return;

                cairo_move_to(pCR, x[0], y[0]);
                for (size_t i=1; i < n; ++i)
                    cairo_line_to(pCR, x[i], y[i]);

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

            void CocoaCairoSurface::fill_circle(const Color &c, float x, float y, float r)
            {
                if (pCR == NULL)
                    return;

                setSourceRGBA(c);
                cairo_arc(pCR, x, y, r, 0.0f, M_PI * 2.0f);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_circle(IGradient *g, float x, float y, float r)
            {
                if (pCR == NULL)
                    return;

                CocoaCairoGradient *cg = static_cast<CocoaCairoGradient *>(g);
                cg->apply(pCR);
                cairo_arc(pCR, x, y, r, 0, M_PI * 2.0f);
                cairo_fill(pCR);
            }

            void CocoaCairoSurface::fill_frame(
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

            bool CocoaCairoSurface::get_antialiasing()
            {
                if (pCR == NULL)
                    return false;

                return cairo_get_antialias(pCR) != CAIRO_ANTIALIAS_NONE;
            }

            bool CocoaCairoSurface::set_antialiasing(bool set)
            {
                if (pCR == NULL)
                    return false;

                bool old = cairo_get_antialias(pCR) != CAIRO_ANTIALIAS_NONE;
                cairo_set_antialias(pCR, (set) ? CAIRO_ANTIALIAS_GOOD : CAIRO_ANTIALIAS_NONE);

                return old;
            }

            ws::point_t CocoaCairoSurface::set_origin(const ws::point_t & origin)
            {
                return set_origin(origin.nLeft, origin.nTop);
            }

            ws::point_t CocoaCairoSurface::set_origin(ssize_t left, ssize_t top)
            {
                ws::point_t result;
                result.nLeft    = fOriginX;
                result.nTop     = fOriginY;

                if (pCR == NULL)
                    return result;

                fOriginX        = left;
                fOriginY        = top;

                cairo_matrix_t matrix;
                matrix.xx       = 1.0f;
                matrix.xy       = 0.0f;
                matrix.x0       = fOriginX;

                matrix.yx       = 0.0f;
                matrix.yy       = 1.0f;
                matrix.y0       = fOriginY;

                cairo_set_matrix(pCR, &matrix);

                return result;
            }

            void CocoaCairoSurface::clip_begin(float x, float y, float w, float h)
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

            void CocoaCairoSurface::clip_end()
            {
                if (pCR == NULL)
                    return;

            #ifdef LSP_DEBUG
                if (nNumClips <= 0)
                {
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
                    return;
                }
                -- nNumClips;
            #endif /* LSP_DEBUG */

                cairo_restore(pCR);
            }

        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */

#endif /* defined(PLATFORM_MAXOSX) */
