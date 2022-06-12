/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 12 дек. 2016 г.
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

#ifndef LSP_PLUG_IN_WS_IDISPLAY_H_
#define LSP_PLUG_IN_WS_IDISPLAY_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/ipc/Library.h>
#include <lsp-plug.in/ws/types.h>
#include <lsp-plug.in/r3d/iface/backend.h>
#include <lsp-plug.in/r3d/iface/factory.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/parray.h>

#include <lsp-plug.in/ws/ISurface.h>
#include <lsp-plug.in/ws/IDataSink.h>
#include <lsp-plug.in/ws/IDataSource.h>
#include <lsp-plug.in/ws/IWindow.h>

namespace lsp
{
    namespace ws
    {
        class IWindow;
        class IR3DBackend;

        typedef struct R3DBackendInfo
        {
            LSPString   library;
            LSPString   uid;
            LSPString   display;
            LSPString   lc_key;
            version_t   version;            // Module version
            bool        offscreen;          // Off-screen rendering engine
        } R3DBackendInfo;

        typedef struct MonitorInfo
        {
            LSPString           name;       // The name of monitor
            bool                primary;    // The monitor is primary
            ws::rectangle_t     rect;       // The position and size of monitor
        } MonitorInfo;

        /** Display
         *
         */
        class IDisplay
        {
            private:
                IDisplay & operator = (const IDisplay &);
                IDisplay(const IDisplay &);

            protected:
                typedef struct dtask_t
                {
                    taskid_t        nID;
                    timestamp_t     nTime;
                    task_handler_t  pHandler;
                    void           *pArg;
                } dtask_t;

                typedef struct r3d_lib_t
                {
                    R3DBackendInfo  info;           // Information
                    r3d::factory_t *builtin;        // Built-in factory
                    size_t          local_id;       // Local identifier
                } r3d_lib_t;

            protected:
                taskid_t                    nTaskID;
                lltl::darray<dtask_t>       sTasks;
                dtask_t                     sMainTask;
                lltl::parray<r3d_lib_t>     s3DLibs;            // List of libraries that provide 3D backends
                lltl::parray<IR3DBackend>   s3DBackends;        // List of all 3D backend instances
                ipc::Library                s3DLibrary;         // Current backend library used
                r3d::factory_t             *p3DFactory;         // Pointer to the factory object
                ssize_t                     nCurrent3D;         // Current 3D backend
                ssize_t                     nPending3D;         // Pending 3D backend
                ISurface                   *pEstimation;        // Estimation surface

            protected:
                friend class IR3DBackend;

                bool                taskid_exists(taskid_t id);
                void                deregister_backend(IR3DBackend *lib);
                status_t            switch_r3d_backend(r3d_lib_t *backend);
                status_t            commit_r3d_factory(const LSPString *path, r3d::factory_t *factory, const version_t *mversion);
                void                detach_r3d_backends();
                void                call_main_task(timestamp_t time);
                virtual bool        r3d_backend_supported(const r3d::backend_metadata_t *meta);
                static void         drop_r3d_lib(r3d_lib_t *lib);
                bool                check_duplicate(const r3d_lib_t *lib);

            public:
                explicit IDisplay();
                virtual ~IDisplay();

                /**
                 * Initialize display
                 * @param argc number of additional arguments
                 * @param argv array of additional arguments
                 * @return status of operation
                 */
                virtual status_t    init(int argc, const char **argv);

                /**
                 * Destroy display
                 */
                virtual void        destroy();

            public:
                /**
                 * Enter the main loop. The main loop can be interrupted by calling
                 * the quit_main() function.
                 *
                 * @return status of execution
                 */
                virtual status_t    main();

                /**
                 * Perform a single iteration for the main loop.
                 *
                 * @return status of iteration completion.
                 */
                virtual status_t    main_iteration();

                /**
                 * Leave the main loop.
                 */
                virtual void        quit_main();

                /**
                 * Wait for new events
                 *
                 * @param millis maximum amount of time to wait for new event
                 */
                virtual status_t    wait_events(wssize_t millis);

                /**
                 * Return number of available screens.
                 * @return number of available screens.
                 */
                virtual size_t      screens();

                /**
                 * Get number of default screen
                 * @return number of default screen
                 */
                virtual size_t      default_screen();

                /**
                 * Perform the underlying protocol transfer synchronization
                 */
                virtual void        sync();

                /**
                 * Get size of the screen
                 * @param screen sreen number
                 * @param w pointer to store width
                 * @param h pointer to store height
                 * @return status of operation
                 */
                virtual status_t    screen_size(size_t screen, ssize_t *w, ssize_t *h);

            public:
                /**
                 * Enumerate backends
                 * @param id backend number starting with 0
                 * @return backend descriptor or NULL if backend with such identifier does not exist
                 */
                const R3DBackendInfo *enum_backend(size_t id) const;

                /**
                 * Get currently used backend for 3D rendering
                 * @return selected backend descriptor
                 */
                const R3DBackendInfo *current_backend() const;

                /**
                 * Get currently used backend identifier for 3D rendering
                 * @return selected backend identifier
                 */
                ssize_t current_backend_id() const;

                /**
                 * Select backend for rendering by specifying it's descriptor
                 * This function does not chnage the backend immediately,
                 * instead of this the backend switch operation is performed in the
                 * main loop
                 * @param backend backend for rendering
                 * @return status of operation
                 */
                status_t select_backend(const R3DBackendInfo *backend);

                /**
                 * Select backend for rendering by specifying it's descriptor identifier
                 * This function does not chnage the backend immediately,
                 * instead of this the backend switch operation is performed in the
                 * main loop
                 * @param id backend's descriptor identifier
                 * @return status of operation
                 */
                status_t select_backend_id(size_t id);

                /**
                 * Lookup the specified directory for existing 3D backends
                 * @param path the directory to perform lookup
                 * @param part the substring that should be contained in the library file name
                 */
                void lookup_r3d_backends(const io::Path *path, const char *part);

                /**
                 * Lookup the specified directory for existing 3D backends
                 * @param path the directory to perform lookup
                 * @param part the substring that should be contained in the library file name
                 */
                void lookup_r3d_backends(const char *path, const char *part);

                /**
                 * Lookup the specified directory for existing 3D backends
                 * @param path the directory to perform lookup
                 * @param part the substring that should be contained in the library file name
                 */
                void lookup_r3d_backends(const LSPString *path, const char *part);

                /**
                 * Try to register the library as a 3D backend
                 * @param path the path to the library
                 * @return status of operation
                 */
                status_t register_r3d_backend(const io::Path *path);

                /**
                 * Try to register the library as a 3D backend
                 * @param path the path to the library
                 * @return status of operation
                 */
                status_t register_r3d_backend(const char *path);

                /**
                 * Try to register the library as a 3D backend
                 * @param path the path to the library
                 * @return status of operation
                 */
                status_t register_r3d_backend(const LSPString *path);


                /** Create native window
                 *
                 * @return native window
                 */
                virtual IWindow *create_window();

                /** Create window at the specified screen
                 *
                 * @param screen screen
                 * @return status native window
                 */
                virtual IWindow *create_window(size_t screen);

                /** Create native window as child of specified window handle
                 *
                 * @return native window as child of specified window handle
                 */
                virtual IWindow *create_window(void *handle);

                /**
                 * Wrap window handle
                 * @param handle handle to wrap
                 * @return native window as wrapper of the handle
                 */
                virtual IWindow *wrap_window(void *handle);

                /**
                 * Create 3D backend for graphics
                 * @return pointer to created backend
                 */
                virtual IR3DBackend *create_r3d_backend(IWindow *parent);

                /** Create surface for drawing
                 *
                 * @param width surface width
                 * @param height surface height
                 * @return surface or NULL on error
                 */
                virtual ISurface *create_surface(size_t width, size_t height);

                /**
                 * Get estimation surface. This surface is not for drawing but
                 * for estimating additional parameters like text parameters, etc.
                 *
                 * @return pointer to estimation surface
                 */
                virtual ISurface *estimation_surface();

                /** Submit task for execution
                 *
                 * @param time time when the task should be triggered (timestamp in milliseconds)
                 * @param handler task handler
                 * @param arg task handler argument
                 * @return submitted task identifier or negative error code
                 */
                virtual taskid_t submit_task(timestamp_t time, task_handler_t handler, void *arg);

                /** Cancel submitted task
                 *
                 * @param id task identifier to cancel
                 * @return status of operation
                 */
                virtual status_t cancel_task(taskid_t id);

                /**
                 * Associate data source with the specified clipboard
                 * @param id clipboard identifier
                 * @param src data source to use
                 * @return status of operation
                 */
                virtual status_t set_clipboard(size_t id, IDataSource *src);

                /**
                 * Sink clipboard data source to the specified handler.
                 * After the data source has been processed, release() should
                 * be called on data source
                 * @param id clipboard identifier
                 * @return pointer to data source or NULL
                 */
                virtual status_t get_clipboard(size_t id, IDataSink *dst);

                /**
                 * Reject drag request because drag is not supported at the current position
                 * @return status of operation
                 */
                virtual status_t reject_drag();

                /**
                 * Accept drag request
                 * @param sink the sink that will handle data transfer
                 * @param action drag action
                 * @param internal true if we want to receive notifications inside of the drag rectangle
                 * @param r parameters of the drag rectangle, can be NULL
                 * @return status of operation
                 */
                virtual status_t accept_drag(IDataSink *sink, drag_t action, bool internal, const rectangle_t *r);

                /**
                 * Get currently pending content type of a drag
                 * @return NULL-terminated list of pending content types,
                 *   may be NULL if there is no currently pending Drag&Drop request
                 */
                virtual const char * const *get_drag_ctypes();

                /**
                 * Get current cursor location
                 * @param screen current screen where the pointer is located
                 * @param left pointer to store X position
                 * @param top pointer to store Y position
                 * @return status of operation
                 */
                virtual status_t get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top);

                /**
                 * Set callback which will be called after each main iteration
                 * @param handler callback handler routine
                 * @param arg additional argument
                 */
                void set_main_callback(task_handler_t handler, void *arg);

                /**
                 * Load font and add to the repository
                 * @param name font name in UTF-8
                 * @param path path to the file that contains the font data (UTF-8)
                 * @return status of operation
                 */
                virtual status_t add_font(const char *name, const char *path);

                /**
                 * Load font and add to the repository
                 * @param name font name in UTF-8
                 * @param path path to the file that contains the font data (UTF-8)
                 * @return status of operation
                 */
                virtual status_t add_font(const char *name, const io::Path *path);

                /**
                 * Load font and add to the repository
                 * @param name font name in UTF-8
                 * @param path path to the file that contains the font data (UTF-8)
                 * @return status of operation
                 */
                virtual status_t add_font(const char *name, const LSPString *path);

                /**
                 * Load font from stream and add to the repository
                 * @param name font name in UTF-8
                 * @param path path to the file that contains the font data (UTF-8)
                 * @return status of operation
                 */
                virtual status_t add_font(const char *name, io::IInStream *is);

                /**
                 * Add an alias to the font name
                 * @param name font name
                 * @param alias alias for the font name
                 * @return status of operation
                 */
                virtual status_t add_font_alias(const char *name, const char *alias);

                /**
                 * Remove font or font alias by identifier
                 * @param name name font name or font alias
                 * @return status of operation
                 */
                virtual status_t remove_font(const char *name);

                /**
                 * Remove all previously loaded custom fonts and aliases
                 */
                virtual void remove_all_fonts();

                /**
                 * Enumerate list of mointors and return the pointer to the list of monitors.
                 * The list of monitors is valid until the next enum_monitors() call.
                 *
                 * @param count number of actual monitors enumerated
                 * @return pointer to enumerated monitors or error.
                 */
                virtual const MonitorInfo       *enum_monitors(size_t *count);
        };

    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_IDISPLAY_H_ */
