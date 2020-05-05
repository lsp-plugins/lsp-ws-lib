/*
 * Font.cpp
 *
 *  Created on: 4 мая 2020 г.
 *      Author: sadko
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
            nFlags      = 0;
        }

        Font::Font(const char *name)
        {
            sName       = ::strdup(name);
            fSize       = 10.0f;
            nFlags      = 0;
        }

        Font::Font(const char *name, float size)
        {
            sName       = ::strdup(name);
            fSize       = size;
            nFlags      = 0;
        }

        Font::Font(float size)
        {
            sName       = ::strdup("Sans");
            fSize       = size;
            nFlags      = 0;
        }

        Font::Font(const Font *s)
        {
            set(s);
        }

        Font::~Font()
        {
            if (sName != NULL)
            {
                ::free(sName);
                sName = NULL;
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
