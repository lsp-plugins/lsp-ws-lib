/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 16 авг. 2019 г.
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

#include <lsp-plug.in/ws/IDataSource.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace ws
    {
        
        IDataSource::IDataSource(const char *const *mimes)
        {
            nReferences     = 0;
            size_t n        = 0;

            for (const char *const *m=mimes; *m != NULL; ++m)
                ++n;

            vMimes          = reinterpret_cast<char **>(::malloc(sizeof(char *) * (n+1)));
            if (vMimes != NULL)
            {
                for (size_t i=0; i<(n+1); ++i)
                    vMimes[i]       = NULL;

                for (size_t i=0, j=0; i<n; ++i)
                {
                    vMimes[j]   = ::strdup(mimes[i]);
                    if (vMimes[j])
                        ++j;
                }
            }
        }
        
        IDataSource::~IDataSource()
        {
            if (vMimes != NULL)
            {
                for (char **p = vMimes; *p != NULL; ++p)
                    ::free(*p);
                ::free(vMimes);
                vMimes = NULL;
            }
        }

        io::IInStream *IDataSource::open(const char *mime)
        {
            return NULL;
        }

        size_t IDataSource::acquire()
        {
            return ++nReferences;
        }

        size_t IDataSource::release()
        {
            ssize_t refs    = --nReferences;
            if (refs <= 0)
                delete this;

            return refs;
        }
    
    } /* namespace ws */
} /* namespace lsp */
