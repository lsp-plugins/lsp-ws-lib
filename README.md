# lsp-ws-lib

LSP window subsystem core library.

This library provides abstraction layer for working with
window subsystem of the target software platform.

This library does not provide or use any global variables and can be
easily used with other versions of itself. Factroy function should be
used to build most suitable display instance.

It provides:
* Common event format definition and constants.
* Factory for building most suitable display.
* ws::IDisplay interface for accessing the graphics display.
* ws::IWindow interface for accessing windows.
* ws::IR3DBackend for accessing accelerated 3D rendering backends.
* Asynchronous data flow primitives:
  * IDataSource as a data source.
  * IDataSink as a data collector.
  * IEventHandler for handling events.
* Drawing primitives:
  * ws::Font to manage font properties.
  * ws::IGradient to manage gradients.
  * ws::ISurface to use drawing surfaces.

Requirements
======

The following packages need to be installed for building:

* gcc >= 4.9 (All OS)
* make >= 4.0 (All OS)
* cairo >= 1.14 (Linux, BSD)
* sndfile >= 1.0.25 (Linux, BSD)
* freetype (Linux, BSD)
* xlib (Linux, BSD)

Building
======

To build the library, perform the following commands:

```bash
make config # Configure the build
make fetch # Fetch dependencies from Git repository
make
sudo make install
```

To get more build options, run:

```bash
make help
```

To uninstall library, simply issue:

```bash
make uninstall
```

To clean all binary files, run:

```bash
make clean
```

To clean the whole project tree including configuration files, run:

```bash
make prune
```

Usage
======
```C++
#include <lsp-plug.in/ws/ws.h>

using namespace lsp;

// Event handler class for window events
class Handler: public ws::IEventHandler
{
    private:
        ws::IWindow    *pWnd;

    public:
        inline Handler(ws::IWindow *wnd)
        {
            pWnd        = wnd;
        }

        virtual status_t handle_event(const ws::event_t *ev)
        {
            // Triggered redraw request, just fill the window with some color
            if (ev->nType == ws::UIE_REDRAW)
            {
                Color c(0.0f, 0.5f, 0.75f);
                ws::ISurface *s = pWnd->get_surface();
                if (s != NULL)
                    s->clear(&c);

                return STATUS_OK;
            }
            
            // Triggered close request, force display to quit main loop
            if (ev->nType == ws::UIE_CLOSE)
            {
                pWnd->hide();
                pWnd->display()->quit_main();
            }
            
            // Call parent handler (can be omitted since 
            // IEventHandler::handle_event is just a stub method)
            return IEventHandler::handle_event(ev);
        }
};

int main(int argc, const char **argv)
{
	// Create display
    ws::IDisplay *dpy = ws::create_display(argc, &argv[1]);
    
    // Create window and initialize
    ws::IWindow *wnd = dpy->create_window();
    wnd->init();
    wnd->set_caption("Test window", "Test window");
    wnd->set_border_style(ws::BS_DIALOG);
    wnd->set_window_actions(ws::WA_MOVE | ws::WA_RESIZE | ws::WA_CLOSE);
    wnd->resize(320, 200);
    wnd->set_size_constraints(160, 100, 640, 400)

    // Show the window at the center of the screen
    ssize_t sw, sh;
    size_t screen = wnd->screen();
    dpy->screen_size(screen, &sw, &sh);
    wnd->move((sw - wnd->width()) / 2, (sh - wnd->height()) / 2);
    wnd->show();

	// Setup handler for the window
    Handler h(wnd);
    wnd->set_handler(&h);

    // Enter the main event loop
    dpy->main();

    // Destroy window and display
    wnd->destroy();
    delete wnd;
    ws::free_display(dpy);
}
    
```
