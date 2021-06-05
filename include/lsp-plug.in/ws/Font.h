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
                char               *sName;
                float               fSize;
                size_t              nFlags;

            public:
                explicit Font();
                explicit Font(const char *name);
                explicit Font(const char *name, float size);
                explicit Font(const char *name, float size, size_t flags);
                explicit Font(float size);
                explicit Font(const Font *s);

                ~Font();

            public:
                inline bool             is_bold() const                     { return nFlags & FF_BOLD;                      }
                inline bool             bold() const                        { return nFlags & FF_BOLD;                      }
                inline bool             is_italic() const                   { return nFlags & FF_ITALIC;                    }
                inline bool             italic() const                      { return nFlags & FF_ITALIC;                    }
                inline bool             is_underline() const                { return nFlags & FF_UNDERLINE;                 }
                inline bool             underline() const                   { return nFlags & FF_UNDERLINE;                 }
                inline font_antialias_t antialiasing() const                { return font_antialias_t(nFlags >> FF_COUNT);  }
                inline font_antialias_t antialias() const                   { return antialiasing();                        }
                inline float            get_size() const                    { return fSize;                                 }
                inline float            size() const                        { return fSize;                                 }
                inline const char      *get_name() const                    { return sName;                                 }
                inline const char      *name() const                        { return sName;                                 }
                inline size_t           flags() const                       { return nFlags;                                }

                inline void             set_bold(bool b)                    { if (b) nFlags |= FF_BOLD; else nFlags &= ~FF_BOLD;                            }
                inline void             set_italic(bool i)                  { if (i) nFlags |= FF_ITALIC; else nFlags &= ~FF_ITALIC;                        }
                inline void             set_underline(bool u)               { if (u) nFlags |= FF_UNDERLINE; else nFlags &= ~FF_UNDERLINE;                  }
                inline void             set_antialiasing(font_antialias_t a){ nFlags = (nFlags & FF_ALL) | (a << FF_COUNT); }
                inline void             set_antialias(font_antialias_t a)   { set_antialiasing(a);                          }
                inline void             set_size(float s)                   { fSize = s;                                    }
                inline void             set_flags(size_t flags)             { nFlags = flags & FF_ALL;                      }

                void                    set_name(const char *name);
                void                    set(const Font *s);
                void                    set(const char *name, float size, size_t flags = 0);

                bool                    get_parameters(ISurface *s, font_parameters_t *fp);
                bool                    get_text_parameters(ISurface *s, text_parameters_t *tp, const char *text);
        };
    }
}

#endif /* LSP_PLUG_IN_WS_FONT_H_ */
