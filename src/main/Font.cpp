/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 4 мая 2020 г.
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

#include <lsp-plug.in/ws/Font.h>
#include <lsp-plug.in/ws/ISurface.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace ws
    {
        Font::Font()
        {
            sName       = ::strdup("Sans");
            fSize       = 10.0f;
            nFlags      = FA_DEFAULT << FF_COUNT;
        }

        Font::Font(const char *name)
        {
            sName       = ::strdup(name);
            fSize       = 10.0f;
            nFlags      = FA_DEFAULT << FF_COUNT;
        }

        Font::Font(const char *name, float size)
        {
            sName       = ::strdup(name);
            fSize       = size;
            nFlags      = FA_DEFAULT << FF_COUNT;
        }

        Font::Font(const char *name, float size, size_t flags, ws::font_antialias_t antialias)
        {
            sName       = ::strdup(name);
            fSize       = size;
            nFlags      = (flags & FF_ALL) | (antialias << FF_COUNT);
        }

        Font::Font(float size)
        {
            sName       = ::strdup("Sans");
            fSize       = size;
            nFlags      = FA_DEFAULT << FF_COUNT;
        }

        Font::Font(const Font *s)
        {
            sName       = NULL;
            set(s);
        }

        Font::~Font()
        {
            if (sName != NULL)
            {
                ::free(sName);
                sName       = NULL;
            }
        }

        void Font::set(const Font *s)
        {
            if (sName != NULL)
                ::free(sName);
            sName       = (s->sName != NULL) ? strdup(s->sName) : NULL;
            fSize       = s->fSize;
            nFlags      = s->nFlags;
        }

        void Font::set(const char *name, float size)
        {
            if (sName != NULL)
                ::free(sName);
            sName       = (name != NULL) ? strdup(name) : NULL;
            fSize       = size;
        }

        void Font::set(const char *name, float size, size_t flags, ws::font_antialias_t antialias)
        {
            if (sName != NULL)
                ::free(sName);
            sName       = (name != NULL) ? strdup(name) : NULL;
            fSize       = size;
            nFlags      = (flags & FF_ALL) | (antialias << FF_COUNT);
        }

        void Font::set_name(const char *name)
        {
            if (sName != NULL)
                ::free(sName);
            sName   = (name != NULL) ? strdup(name) : NULL;
        }

        bool Font::get_parameters(ISurface *s, font_parameters_t *fp)
        {
            return s->get_font_parameters(*this, fp);
        }

        bool Font::get_text_parameters(ISurface *s, text_parameters_t *tp, const char *text)
        {
            return s->get_text_parameters(*this, tp, text);
        }
    }
}
