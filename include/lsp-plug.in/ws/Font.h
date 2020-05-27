/*
 * Font.h
 *
 *  Created on: 4 мая 2020 г.
 *      Author: sadko
 */

#ifndef LSP_PLUG_IN_WS_FONT_H_
#define LSP_PLUG_IN_WS_FONT_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/types.h>

namespace lsp
{
    namespace ws
    {
        class ISurface;

        /**
         * Font descriptor class
         */
        class Font
        {
            private:
                Font & operator = (const Font &);

            private:
                char       *sName;
                float       fSize;
                size_t      nFlags;

            public:
                explicit Font();
                explicit Font(const char *name);
                explicit Font(const char *name, float size);
                explicit Font(float size);
                explicit Font(const Font *s);

                ~Font();

            public:
                inline bool         is_bold() const             { return nFlags & FF_BOLD;          }
                inline bool         is_italic() const           { return nFlags & FF_ITALIC;        }
                inline bool         is_underline() const        { return nFlags & FF_UNDERLINE;     }
                inline bool         is_antialiasing() const     { return nFlags & FF_ANTIALIASING;  }
                inline float        get_size() const            { return fSize;                     }
                inline const char  *get_name() const            { return sName;                     }
                inline size_t       flags() const               { return nFlags;                    }

                inline void         set_bold(bool b)            { if (b) nFlags |= FF_BOLD; else nFlags &= ~FF_BOLD;                    }
                inline void         set_italic(bool i)          { if (i) nFlags |= FF_ITALIC; else nFlags &= ~FF_ITALIC;                }
                inline void         set_underline(bool u)       { if (u) nFlags |= FF_UNDERLINE; else nFlags &= ~FF_UNDERLINE;          }
                inline void         set_antialiasing(bool a)    { if (a) nFlags |= FF_ANTIALIASING; else nFlags &= ~FF_ANTIALIASING;    }
                inline void         set_size(float s)           { fSize = s; }
                inline void         set_flags(size_t flags)     { nFlags = flags & FF_ALL;          }

                void                set_name(const char *name);
                void                set(const Font *s);

                bool                get_parameters(ISurface *s, font_parameters_t *fp);
                bool                get_text_parameters(ISurface *s, text_parameters_t *tp, const char *text);
        };
    }
}

#endif /* LSP_PLUG_IN_WS_FONT_H_ */
