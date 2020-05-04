/*
 * X11Atoms.cpp
 *
 *  Created on: 11 дек. 2016 г.
 *      Author: sadko
 */

#include <lsp-plug.in/common/types.h>

#ifdef USE_XLIB

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

#endif /* USE_XLIB */
