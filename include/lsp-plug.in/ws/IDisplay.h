/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/ipc/Thread.h>
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

        /**
         * Display interface
         */
        class LSP_WS_LIB_PUBLIC IDisplay
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
                ipc::Mutex                  sTasksLock;         // Mutex to allow multithreaded access to the queue
                size_t                      nTaskChanges;       // Number of task changes committed by the display
                dtask_t                     sMainTask;
                lltl::parray<r3d_lib_t>     s3DLibs;            // List of libraries that provide 3D backends
                lltl::parray<IR3DBackend>   s3DBackends;        // List of all 3D backend instances
                ipc::Library                s3DLibrary;         // Current backend library used
                r3d::factory_t             *p3DFactory;         // Pointer to the factory object
                ssize_t                     nCurrent3D;         // Current 3D backend
                ssize_t                     nPending3D;         // Pending 3D backend
                size_t                      nIdleInterval;      // Idle interval in milliseconds

            protected:
                friend class IR3DBackend;

                bool                taskid_exists(taskid_t id);
                void                deregister_backend(IR3DBackend *lib);
                status_t            switch_r3d_backend(r3d_lib_t *backend);
                status_t            commit_r3d_factory(const LSPString *path, r3d::factory_t *factory, const version_t *mversion);
                void                detach_r3d_backends();
                status_t            process_pending_tasks(timestamp_t time);
                static void         drop_r3d_lib(r3d_lib_t *lib);
                bool                check_duplicate(const r3d_lib_t *lib);

                /**
                 * Estimate the delay for the first scheduled task and reduce the passed polling delay
                 * if there is pending scheduled task
                 * @param ts current timestamp
                 * @param poll_delay the estimated polling delay
                 * @return the updated polling delay
                 */
                int                 compute_poll_delay(timestamp_t ts, int poll_delay);


            protected:
                virtual bool        r3d_backend_supported(const r3d::backend_metadata_t *meta);

                /**
                 * Callback function that triggers the underlying displayy to wake up if the state
                 * of task queue haas changed. Is sent once on the first change. The next time
                 * it will trigger only after process_pending_tasks() call.
                 */
                virtual void        task_queue_changed();

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

                /**
                 * Read the geometry of work area on the primary monitor which excludes
                 * any dock panels, taskbars, etc.
                 * @param r pointer to store the result
                 * @return status of operation
                 */
                virtual status_t    work_area_geometry(ws::rectangle_t *r);

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
                 * Create 3D backend for graphics. The backend should be destroyed and
                 * deleted by the caller.
                 * @return pointer to created backend
                 */
                virtual IR3DBackend *create_r3d_backend(IWindow *parent);

                /** Submit task for execution. The implementation MUST be thread safe and allow
                 * to submit tasks from ANY thread at ANY time.
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
                 * Check if drag request is still pending and was not rejected, works only
                 * in the event handler that handles current Drag&Drop operation.
                 *
                 * @return true if drag request is still pending and was not rejected
                 */
                virtual bool drag_pending();

                /**
                 * Force to reject drag&drop operation request because drag is not supported
                 * at the current position. This call should be issued in extra cases as by
                 * default display automatically rejects drag if no accept_drag() was called.
                 * @return status of operation
                 */
                virtual status_t reject_drag();

                /**
                 * Accept drag request
                 * @param sink the sink that will handle data transfer
                 * @param action drag action
                 * @param r parameters of the drag rectangle, optional, can be NULL
                 * @return status of operation
                 */
                virtual status_t accept_drag(IDataSink *sink, drag_t action, const rectangle_t *r = NULL);


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
                 * Add an alias (pseudonym) to the font name
                 * @param name the name of the alias
                 * @param alias the name of the original (aliased) font
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

                /** Get font parameters
                 *
                 * @param f font
                 * @param fp font parameters to store
                 * @return status of operation
                 */
                virtual bool get_font_parameters(const Font &f, font_parameters_t *fp);

                /** Get text parameters
                 *
                 * @param f font
                 * @param tp text parameters to store
                 * @param text text to analyze
                 * @return status of operation
                 */
                virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const char *text);

                /** Get text parameters
                 *
                 * @param f font
                 * @param tp text parameters to store
                 * @param text text to analyze
                 * @return status of operation
                 */
                virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text);

                /** Get text parameters
                 *
                 * @param f font
                 * @param tp text parameters to store
                 * @param text text to analyze
                 * @param first first character
                 * @return status of operation
                 */
                virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first);

                /** Get text parameters
                 *
                 * @param f font
                 * @param tp text parameters to store
                 * @param text text to analyze
                 * @param first first character
                 * @param last last character
                 * @return status of operation
                 */
                virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last);

                /**
                 * Enumerate list of mointors and return the pointer to the list of monitors.
                 * The list of monitors is valid until the next enum_monitors() call.
                 *
                 * @param count number of actual monitors enumerated
                 * @return pointer to enumerated monitors or error.
                 */
                virtual const MonitorInfo      *enum_monitors(size_t *count);

                /**
                 * Get the typical idle interval for the display
                 * @return the typical idle interval (default 50 ms or 20 FPS)
                 */
                virtual size_t                  idle_interval();

                /**
                 * Set the typical idle interval for the display
                 * @param interval idle interval in millisecionds
                 * @return previous value of the idle interval
                 */
                size_t                          set_idle_interval(size_t interval);

                /**
                 * Obtain the file descriptor of the connection associated with the event loop
                 * if it is supported (usually Unix-based systems with X11 protocol).
                 *
                 * @param fd pointer to store file descriptor
                 * @return status of operation, STATUS_NOT_SUPPORTED if platform does
                 * not support file desciptors for event loops
                 */
                virtual status_t                get_file_descriptor(int *fd);
        };

    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_IDISPLAY_H_ */
