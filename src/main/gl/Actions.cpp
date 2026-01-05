/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 5 янв. 2026 г.
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

#include <private/gl/Actions.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/ws/Font.h>
#include <lsp-plug.in/runtime/LSPString.h>

#include <private/gl/SurfaceContext.h>
#include <private/gl/Data.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            namespace actions
            {
                inline void init_fill(gl::fill_t & fill)
                {
                    fill.type               = gl::FILL_NONE;
                }

                inline void destroy_fill(gl::fill_t & fill)
                {
                    if (fill.type == gl::FILL_TEXTURE)
                        safe_release(fill.surface);
                }

                action_t *init(action_t *action, action_type_t type)
                {
                    if (action == NULL)
                        return NULL;

                    action->type    = type;
                    switch (type)
                    {
                        case DRAW_SURFACE:
                            action->draw_surface.surface    = NULL;
                            break;
                        case DRAW_RAW:
                            action->draw_raw.data           = NULL;
                            break;
                        case WIRE_RECT:
                            init_fill(action->wire_rect.fill);
                            break;
                        case FILL_RECT:
                            init_fill(action->fill_rect.fill);
                            break;
                        case FILL_SECTOR:
                            init_fill(action->fill_sector.fill);
                            break;
                        case FILL_TRIANGLE:
                            init_fill(action->fill_triangle.fill);
                            break;
                        case FILL_CIRCLE:
                            init_fill(action->fill_circle.fill);
                            break;
                        case WIRE_ARC:
                            init_fill(action->wire_arc.fill);
                            break;
                        case OUT_TEXT:
                            new (&action->out_text.font, lsp::inplace_new_tag_t()) ws::Font();
                            new (&action->out_text.text, lsp::inplace_new_tag_t()) LSPString();
                            break;
                        case OUT_TEXT_RELATIVE:
                            new (&action->out_text_relative.font, lsp::inplace_new_tag_t()) ws::Font();
                            new (&action->out_text_relative.text, lsp::inplace_new_tag_t()) LSPString();
                            break;
                        case LINE:
                            init_fill(action->line.fill);
                            break;
                        case PARAMETRIC_LINE:
                            init_fill(action->parametric_line.fill);
                            break;
                        case CLIPPED_PARAMETRIC_LINE:
                            init_fill(action->clipped_parametric_line.fill);
                            break;
                        case CLIPPED_PARAMETRIC_BAR:
                            init_fill(action->clipped_parametric_bar.fill);
                            break;
                        case FILL_FRAME:
                            init_fill(action->fill_frame.fill);
                            break;
                        case DRAW_POLY:
                            init_fill(action->draw_poly.fill);
                            init_fill(action->draw_poly.wire);
                            break;
                        default:
                            break;
                    }

                    return action;
                }

                void destroy(action_t *action)
                {
                    if (action == NULL)
                        return;

                    switch (action->type)
                    {
                        case DRAW_SURFACE:
                            safe_release(action->draw_surface.surface);
                            break;
                        case DRAW_RAW:
                            if (action->draw_raw.data != NULL)
                            {
                                free(action->draw_raw.data);
                                action->draw_raw.data   = NULL;
                            }
                            break;
                        case WIRE_RECT:
                            destroy_fill(action->wire_rect.fill);
                            break;
                        case FILL_RECT:
                            destroy_fill(action->fill_rect.fill);
                            break;
                        case FILL_SECTOR:
                            destroy_fill(action->fill_sector.fill);
                            break;
                        case FILL_TRIANGLE:
                            destroy_fill(action->fill_triangle.fill);
                            break;
                        case FILL_CIRCLE:
                            destroy_fill(action->fill_circle.fill);
                            break;
                        case WIRE_ARC:
                            destroy_fill(action->wire_arc.fill);
                            break;
                        case OUT_TEXT:
                            action->out_text.font.~Font();
                            action->out_text.text.~LSPString();
                            break;
                        case OUT_TEXT_RELATIVE:
                            action->out_text_relative.font.~Font();
                            action->out_text_relative.text.~LSPString();
                            break;
                        case LINE:
                            destroy_fill(action->line.fill);
                            break;
                        case PARAMETRIC_LINE:
                            destroy_fill(action->parametric_line.fill);
                            break;
                        case CLIPPED_PARAMETRIC_LINE:
                            destroy_fill(action->clipped_parametric_line.fill);
                            break;
                        case CLIPPED_PARAMETRIC_BAR:
                            destroy_fill(action->clipped_parametric_bar.fill);
                            break;
                        case FILL_FRAME:
                            destroy_fill(action->fill_frame.fill);
                            break;
                        case DRAW_POLY:
                            destroy_fill(action->draw_poly.fill);
                            destroy_fill(action->draw_poly.wire);
                            break;
                        default:
                            break;
                    }
                }

            } /* namespace actions */

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */


