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
                enum flags_t
                {
                    F_BOLD          = 1 << 0,
                    F_ITALIC        = 1 << 1,
                    F_UNDERLINE     = 1 << 2,
                    F_ANTIALIASING  = 1 << 3
                };

                char       *sName;
                float       fSize;
                int         nFlags;

            public:
                explicit Font();
                explicit Font(const char *name);
                explicit Font(const char *name, float size);
                explicit Font(float size);
                explicit Font(const Font *s);

                ~Font();

            public:
                inline bool         is_bold() const             { return nFlags & F_BOLD;           }
                inline bool         is_italic() const           { return nFlags & F_ITALIC;         }
                inline bool         is_underline() const        { return nFlags & F_UNDERLINE;      }
                inline bool         is_antialiasing() const     { return nFlags & F_ANTIALIASING;   }
                inline float        get_size() const            { return fSize;                     }
                inline const char  *get_name() const            { return sName;                     }

                inline void         set_bold(bool b)            { if (b) nFlags |= F_BOLD; else nFlags &= ~F_BOLD;                  }
                inline void         set_italic(bool i)          { if (i) nFlags |= F_ITALIC; else nFlags &= ~F_ITALIC;              }
                inline void         set_underline(bool u)       { if (u) nFlags |= F_UNDERLINE; else nFlags &= ~F_UNDERLINE;        }
                inline void         set_antialiasing(bool a)    { if (a) nFlags |= F_ANTIALIASING; else nFlags &= ~F_ANTIALIASING;  }
                inline void         set_size(float s)           { fSize = s; }

                void                set_name(const char *name);
                void                set(const Font *s);

                bool                get_parameters(ISurface *s, font_parameters_t *fp);
                bool                get_text_parameters(ISurface *s, text_parameters_t *tp, const char *text);
        };
    }
}

#endif /* LSP_PLUG_IN_WS_FONT_H_ */
