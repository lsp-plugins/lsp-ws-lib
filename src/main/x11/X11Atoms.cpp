/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 11 дек. 2016 г.
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

#ifdef USE_LIBX11

#include <private/x11/X11Atoms.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            status_t init_atoms(Display *dpy, x11_atoms_t *atoms)
            {
                #define WM_ATOM(name) \
                    atoms->X11_ ## name = XInternAtom(dpy, #name, False); \
                    /* lsp_trace("  %s = %d", #name, int(atoms->X11_ ## name)); */
    //                if (atoms->X11_ ## name == None)
    //                    return STATUS_NOT_FOUND;

                #define WM_PREDEFINED_ATOM(name) \
                    atoms->X11_ ## name = name; \
                    /*lsp_trace("  %s = %d", #name, int(atoms->X11_ ## name))*/;

                #include <private/x11/X11AtomList.h>
                #undef WM_PREDEFINED_ATOM
                #undef WM_ATOM

                return STATUS_OK;
            }
        }
    }
}

#endif /* USE_LIBX11 */
