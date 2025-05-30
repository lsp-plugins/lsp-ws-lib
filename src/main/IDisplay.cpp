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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/r3d/iface/version.h>
#include <lsp-plug.in/r3d/iface/factory.h>
#include <lsp-plug.in/r3d/iface/builtin.h>
#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IR3DBackend.h>
#include <lsp-plug.in/io/Dir.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/ipc/Library.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/ws/IWindow.h>

#define R3D_LIBRARY_FILE_PART       "lsp-r3d"

namespace lsp
{
    namespace ws
    {
    #ifdef PLATFORM_POSIX
        #ifdef ARCH_32BIT
            static const char *library_paths[] =
            {
            #ifdef LSP_INSTALL_PREFIX
                LSP_INSTALL_PREFIX "/lib",
                LSP_INSTALL_PREFIX "/lib64",
                LSP_INSTALL_PREFIX "/bin",
                LSP_INSTALL_PREFIX "/sbin",
            #endif /* LSP_INSTALL_PREFIX */

                "/usr/local/lib32",
                "/usr/lib32",
                "/lib32",
                "/usr/local/lib",
                "/usr/lib",
                "/lib",
                "/usr/local/bin",
                "/usr/bin",
                "/bin",
                "/usr/local/sbin",
                "/usr/sbin",
                "/sbin",
                NULL
            };
        #endif

        #ifdef ARCH_64BIT
            static const char *library_paths[] =
            {
            #ifdef LSP_INSTALL_PREFIX
                LSP_INSTALL_PREFIX "/lib",
                LSP_INSTALL_PREFIX "/lib64",
                LSP_INSTALL_PREFIX "/bin",
                LSP_INSTALL_PREFIX "/sbin",
            #endif /* LSP_INSTALL_PREFIX */

                "/usr/local/lib64",
                "/usr/lib64",
                "/lib64",
                "/usr/local/lib",
                "/usr/lib",
                "/lib",
                "/usr/local/bin",
                "/usr/bin",
                "/bin",
                "/usr/local/sbin",
                "/usr/sbin",
                "/sbin",
                NULL
            };
        #endif /* ARCH_64_BIT */
    #endif /* PLATFORM_POSIX */

        static const ::lsp::version_t ws_lib_version=LSP_DEFINE_VERSION(LSP_WS_LIB);
        static const ::lsp::version_t r3d_iface_version=LSP_DEFINE_VERSION(LSP_R3D_IFACE);

        IDisplay::IDisplay()
        {
            nTaskID             = 0;
            nTaskChanges        = 0;
            p3DFactory          = NULL;
            nCurrent3D          = 0;
            nPending3D          = 0;
            sMainTask.nID       = 0;
            sMainTask.nTime     = 0;
            sMainTask.pHandler  = NULL;
            sMainTask.pArg      = NULL;
            nIdleInterval       = 50;
        }

        IDisplay::~IDisplay()
        {
        }

        const R3DBackendInfo *IDisplay::enum_backend(size_t id) const
        {
            const r3d_lib_t *lib = s3DLibs.get(id);
            return (lib != NULL) ? &lib->info : NULL;
        };

        const R3DBackendInfo *IDisplay::current_backend() const
        {
            const r3d_lib_t *lib = s3DLibs.get(nCurrent3D);
            return (lib != NULL) ? &lib->info : NULL;
        }

        ssize_t IDisplay::current_backend_id() const
        {
            return nCurrent3D;
        }

        status_t IDisplay::select_backend(const R3DBackendInfo *backend)
        {
            if (backend == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Lookup the corresponding backend
            for (size_t i=0, n=s3DLibs.size(); i<n; ++i)
            {
                const r3d_lib_t *lib = s3DLibs.uget(i);
                if (backend == &lib->info)
                {
                    nPending3D      = i;
                    break;
                }
            }

            return STATUS_OK;
        }

        status_t IDisplay::select_backend_id(size_t id)
        {
            const r3d_lib_t *lib = s3DLibs.get(id);
            if (lib == NULL)
                return STATUS_NOT_FOUND;

            nPending3D      = id;
            return STATUS_OK;
        }

        void IDisplay::lookup_r3d_backends(const io::Path *path, const char *part)
        {
            io::Dir dir;

            lsp_trace("Lookup R3D in directory:%s", path->as_native());

            status_t res = dir.open(path);
            if (res != STATUS_OK)
                return;

            io::Path child;
            LSPString item, substring;
            if (!substring.set_utf8(part))
                return;

            io::fattr_t fattr;
            while ((res = dir.read(&item, false)) == STATUS_OK)
            {
                if (item.index_of(&substring) < 0)
                    continue;
                if (!ipc::Library::valid_library_name(&item))
                    continue;

                if ((res = child.set(path, &item)) != STATUS_OK)
                    continue;
                if ((res = child.stat(&fattr)) != STATUS_OK)
                    continue;

                switch (fattr.type)
                {
                    case io::fattr_t::FT_DIRECTORY:
                    case io::fattr_t::FT_BLOCK:
                    case io::fattr_t::FT_CHARACTER:
                        continue;
                    default:
                        register_r3d_backend(&child);
                        break;
                }
            }
        }

        void IDisplay::lookup_r3d_backends(const char *path, const char *part)
        {
            io::Path tmp;
            if (tmp.set(path) != STATUS_OK)
                return;
            lookup_r3d_backends(&tmp, part);
        }

        void IDisplay::lookup_r3d_backends(const LSPString *path, const char *part)
        {
            io::Path tmp;
            if (tmp.set(path) != STATUS_OK)
                return;
            lookup_r3d_backends(&tmp, part);
        }

        status_t IDisplay::register_r3d_backend(const io::Path *path)
        {
            if (path == NULL)
                return STATUS_BAD_ARGUMENTS;
            return register_r3d_backend(path->as_string());
        }

        status_t IDisplay::register_r3d_backend(const char *path)
        {
            LSPString tmp;
            if (path == NULL)
                return STATUS_BAD_ARGUMENTS;
            if (!tmp.set_utf8(path))
                return STATUS_NO_MEM;
            return register_r3d_backend(&tmp);
        }

        bool IDisplay::r3d_backend_supported(const r3d::backend_metadata_t *meta)
        {
            if (meta->wnd_type == r3d::WND_HANDLE_NONE)
                return true;
            return false;
        }

        void IDisplay::drop_r3d_lib(r3d_lib_t *lib)
        {
            if (lib == NULL)
                return;

            version_destroy(&lib->info.version);
            delete lib;
        }

        bool IDisplay::check_duplicate(const r3d_lib_t *lib)
        {
            for (size_t i=0, n=s3DLibs.size(); i<n; ++i)
            {
                const r3d_lib_t *src = s3DLibs.uget(i);

                if ((src->info.uid.equals(&lib->info.uid))
                    && (src->info.display.equals(&lib->info.display))
                    && (src->info.lc_key.equals(&lib->info.lc_key))
                    && (src->info.offscreen == lib->info.offscreen)
                    && (version_cmp(src->info.version, lib->info.version) == 0))
                    return true;
            }

            return false;
        }

        int IDisplay::compute_poll_delay(timestamp_t ts, int poll_delay)
        {
            // Lock the mutex
            sTasksLock.lock();
            lsp_finally { sTasksLock.unlock(); };

            // Perform operations
            if (sTasks.size() <= 0)
                return poll_delay;

            dtask_t *t          = sTasks.first();
            ssize_t delta       = t->nTime - ts;
            if (delta <= 0)
                return 0;
            else if (delta <= poll_delay)
                return delta;

            return poll_delay;
        }

        status_t IDisplay::commit_r3d_factory(const LSPString *path, r3d::factory_t *factory, const version_t *mversion)
        {
            for (size_t id=0; ; ++id)
            {
                // Get metadata record
                const r3d::backend_metadata_t *meta = factory->metadata(factory, id);
                if (meta == NULL)
                    break;
                else if (meta->id == NULL)
                    continue;

                // Create library descriptor
                r3d_lib_t *r3dlib   = new r3d_lib_t();
                if (r3dlib == NULL)
                    return STATUS_NO_MEM;

                r3dlib->builtin         = (path != NULL) ? NULL : factory;
                r3dlib->local_id        = id;
                r3dlib->info.offscreen  = meta->wnd_type == r3d::WND_HANDLE_NONE;
                version_copy(&r3dlib->info.version, mversion);
                if (path != NULL)
                {
                    if (!r3dlib->info.library.set(path))
                    {
                        drop_r3d_lib(r3dlib);
                        return STATUS_NO_MEM;
                    }
                }

                if ((!r3dlib->info.uid.set_utf8(meta->id)) ||
                    (!r3dlib->info.display.set_utf8((meta->display != NULL) ? meta->display : meta->id)) ||
                    (!r3dlib->info.lc_key.set_utf8((meta->lc_key != NULL) ? meta->lc_key : meta->id))
                    )
                {
                    drop_r3d_lib(r3dlib);
                    return STATUS_NO_MEM;
                }

                // Check for duplicates
                if (check_duplicate(r3dlib))
                {
                    lsp_trace("    library %s provides duplicated backend %s (%s)",
                            r3dlib->info.library.get_native(),
                            r3dlib->info.uid.get_native(),
                            r3dlib->info.display.get_native());
                    drop_r3d_lib(r3dlib);
                    return STATUS_DUPLICATED;
                }

                // Add library descriptor to the list
                if (!s3DLibs.add(r3dlib))
                {
                    drop_r3d_lib(r3dlib);
                    return STATUS_NO_MEM;
                }
            }

            return STATUS_OK;
        }

        status_t IDisplay::register_r3d_backend(const LSPString *path)
        {
            ipc::Library lib;

            lsp_trace("  probing library %s", path->get_native());

            // Open library
            status_t res = lib.open(path);
            if (res != STATUS_OK)
                return res;

            // Perform R3D interface version control
            module_version_t vfunc = reinterpret_cast<module_version_t>(lib.import(LSP_R3D_IFACE_VERSION_FUNC_NAME));
            const version_t *mversion = (vfunc != NULL) ? vfunc() : NULL; // Obtain interface version
            if (mversion == NULL)
            {
                lsp_trace("    not provided R3D interface version");
                lib.close();
                return STATUS_INCOMPATIBLE;
            }
            else if (version_cmp(&r3d_iface_version, mversion) != 0)
            {
                lsp_trace("    mismatched R3D interface version: %d.%d.%d-%s vs %d.%d.%d-%s",
                            r3d_iface_version.major, r3d_iface_version.minor, r3d_iface_version.micro, r3d_iface_version.branch,
                            mversion->major, mversion->minor, mversion->micro, mversion->branch);
                lib.close();
                return STATUS_INCOMPATIBLE;
            }

            // Get the module version
            vfunc = reinterpret_cast<module_version_t>(lib.import(LSP_VERSION_FUNC_NAME));
            mversion = (vfunc != NULL) ? vfunc() : NULL; // Obtain interface version
            if (mversion == NULL)
            {
                lsp_trace("    missing module version function");
                lib.close();
                return STATUS_INCOMPATIBLE;
            }

            // Lookup function
            r3d::factory_function_t func = reinterpret_cast<r3d::factory_function_t>(lib.import(LSP_R3D_FACTORY_FUNCTION_NAME));
            if (func == NULL)
            {
                lsp_trace("    missing factory function %s", LSP_R3D_FACTORY_FUNCTION_NAME);
                lib.close();
                return STATUS_NOT_FOUND;
            }

            // Try to instantiate factory
            size_t found = 0;
            for (int idx=0; ; ++idx)
            {
                r3d::factory_t *factory  = func(idx);
                if (factory == NULL)
                    break;

                // Fetch metadata and store
                res = commit_r3d_factory(path, factory, mversion);
                ++found;
            }

            // Close the library
            lib.close();
            return (found > 0) ? res : STATUS_NOT_FOUND;
        }

        status_t IDisplay::init(int argc, const char **argv)
        {
            status_t res;

            // Scan for built-in libraries
            for (int idx=0; ; ++idx)
            {
                r3d::factory_t *factory = r3d::Factory::enumerate(idx);
                if (factory == NULL)
                    break;

                // Commit factory to the list of backends
                if ((res = commit_r3d_factory(NULL, factory, &ws_lib_version)) != STATUS_OK)
                    return res;
            }

            // Scan for another locations
            io::Path path;
            res = ipc::Library::get_self_file(&path);
            if (res == STATUS_OK)
                res     = path.parent();
            if (res == STATUS_OK)
                lookup_r3d_backends(&path, R3D_LIBRARY_FILE_PART);

            // Scan for standard paths
        #ifdef PLATFORM_POSIX
            for (const char **paths = library_paths; *paths != NULL; ++paths)
                lookup_r3d_backends(*paths, R3D_LIBRARY_FILE_PART);
        #endif /* PLATFORM_POSIX */

            return STATUS_OK;
        }

        void IDisplay::destroy()
        {
            // Destroy all backends
            for (size_t j=0,m=s3DBackends.size(); j<m;++j)
            {
                // Get backend
                IR3DBackend *backend = s3DBackends.get(j);
                if (backend == NULL)
                    continue;

                // Destroy backend
                backend->destroy();
                delete backend;
            }

            // Destroy all libs
            for (size_t j=0, m=s3DLibs.size(); j<m; ++j)
            {
                r3d_lib_t *r3dlib = s3DLibs.uget(j);
                drop_r3d_lib(r3dlib);
            }

            // Flush list of backends and close library
            s3DLibs.flush();
            s3DBackends.flush();
            p3DFactory = NULL;
            s3DLibrary.close();
        }

        void IDisplay::detach_r3d_backends()
        {
            // Destroy all backends
            for (size_t j=0,m=s3DBackends.size(); j<m;++j)
            {
                // Get backend
                IR3DBackend *backend = s3DBackends.get(j);
                if (backend != NULL)
                    backend->destroy();
            }
        }

        void IDisplay::task_queue_changed()
        {
        }

        status_t IDisplay::process_pending_tasks(timestamp_t time)
        {
            // Sync backends
            if (nCurrent3D != nPending3D)
            {
                r3d_lib_t *lib = s3DLibs.get(nPending3D);
                if (lib != NULL)
                {
                    if (switch_r3d_backend(lib) == STATUS_OK)
                        nCurrent3D = nPending3D;
                }
                else
                    nPending3D = nCurrent3D;
            }

            // Call the main task
            if (sMainTask.pHandler != NULL)
                sMainTask.pHandler(time, time, sMainTask.pArg);

            // Execute all other pending tasks
            dtask_t task;
            status_t result = STATUS_OK;

            sTasksLock.lock();
            lsp_finally { sTasksLock.unlock(); };

            for (size_t i=0, n = sTasks.size(); i < n; ++i)
            {
                // Fetch new task from queue
                {
                    // Get next task
                    dtask_t *t      = sTasks.first();
                    if ((t == NULL) || (t->nTime > time))
                        break;

                    // Remove the task from the queue
                    task = *t;
                    if (!sTasks.shift())
                    {
                        result = STATUS_UNKNOWN_ERR;
                        break;
                    }
                }

                // Process the task with unlocked task queue
                sTasksLock.unlock();
                lsp_finally { sTasksLock.lock(); };
                {
                    status_t hresult    = task.pHandler(task.nTime, time, task.pArg);
                    if (hresult != STATUS_OK)
                        result      = hresult;
                }
            }

            // Reset the number of changes for tasks so the task_queue_changed()
            // call becomes possible again
            nTaskChanges        = 0;

            return result;
        }

        status_t IDisplay::main()
        {
            return STATUS_SUCCESS;
        }

        void IDisplay::sync()
        {
        }

        void IDisplay::deregister_backend(IR3DBackend *backend)
        {
            // Try to remove backend
            if (!s3DBackends.premove(backend))
                return;

            // Need to unload library?
            if (s3DBackends.size() <= 0)
            {
                p3DFactory  = NULL;
                s3DLibrary.close();
            }
        }

        IR3DBackend *IDisplay::create_r3d_backend(IWindow *parent)
        {
            if (parent == NULL)
                return NULL;

            // Obtain current backend
            r3d_lib_t *lib = s3DLibs.get(nCurrent3D);
            if (lib == NULL)
                return NULL;

            // Check that factory is present
            if (p3DFactory == NULL)
            {
                if (s3DBackends.size() > 0)
                    return NULL;

                // Try to load factory
                if (switch_r3d_backend(lib) != STATUS_OK)
                    return NULL;
            }

            // Call factory to create backend
            r3d::backend_t *backend = p3DFactory->create(p3DFactory, lib->local_id);
            if (backend == NULL)
                return NULL;

            // Initialize backend
            void *handle = NULL;
            status_t res = (backend->init_offscreen) ? backend->init_offscreen(backend) : STATUS_NOT_SUPPORTED;
            if (res != STATUS_OK)
            {
                res = (backend->init_window) ? backend->init_window(backend, &handle) : STATUS_NOT_SUPPORTED;
                if (res != STATUS_OK)
                {
                    backend->destroy(backend);
                    return NULL;
                }
            }

            // Create R3D backend wrapper
            IR3DBackend *r3d = new IR3DBackend(this, backend, parent->handle(), handle);
            if (r3d == NULL)
            {
                backend->destroy(backend);
                return NULL;
            }

            // Register backend wrapper
            if (!s3DBackends.add(r3d))
            {
                r3d->destroy();
                delete r3d;
                return NULL;
            }

            // Return successful operation
            return r3d;
        }

        status_t IDisplay::switch_r3d_backend(r3d_lib_t *lib)
        {
            status_t res;
            r3d::factory_t *factory;
            ipc::Library dlib;

            if (!lib->builtin)
            {
                // Load the library
                res = dlib.open(&lib->info.library);
                if (res != STATUS_OK)
                    return res;

                // Find the proper factory function
                factory = NULL;
                r3d::factory_function_t func = reinterpret_cast<r3d::factory_function_t>(dlib.import(LSP_R3D_FACTORY_FUNCTION_NAME));
                if (func != NULL)
                {
                    // Lookup for the proper factory
                    for (int idx=0; ; ++idx)
                    {
                        // Enumerate factory
                        factory     = func(idx);
                        if (factory == NULL)
                            break;

                        // Get metadata
                        const r3d::backend_metadata_t *meta = factory->metadata(factory, lib->local_id);
                        if ((meta != NULL) &&
                            (strcmp(meta->id, lib->info.uid.get_utf8()) == 0))
                            break;
                    }
                }

                // Return error if factory function was not found
                if (factory == NULL)
                {
                    dlib.close();
                    return STATUS_NOT_FOUND;
                }
            }
            else
                factory     = lib->builtin;

            // Now, iterate all registered backend wrappers and change the backend
            for (size_t i=0, n=s3DBackends.size(); i<n; ++i)
            {
                // Obtain current backend
                IR3DBackend *r3d = s3DBackends.get(i);
                if (r3d == NULL)
                    continue;

                // Call factory to create backend
                void *handle = NULL;
                r3d::backend_t *backend = factory->create(factory, lib->local_id);
                if (backend != NULL)
                {
                    // Initialize backend
                    status_t res = backend->init_offscreen(backend);
                    if (res != STATUS_OK)
                    {
                        res = backend->init_window(backend, &handle);
                        if (res != STATUS_OK)
                        {
                            backend->destroy(backend);
                            backend = NULL;
                            handle  = NULL;
                        }
                    }
                }

                // Call backend wrapper to replace the backend
                r3d->replace_backend(backend, handle);
            }

            // Deregister currently used library
            dlib.swap(&s3DLibrary);
            dlib.close();

            // Register new pointer to the backend factory
            p3DFactory  = factory;

            return STATUS_OK;
        }

        status_t IDisplay::main_iteration()
        {
            return STATUS_SUCCESS;
        }

        void IDisplay::quit_main()
        {
        }

        status_t IDisplay::wait_events(wssize_t millis)
        {
            if (millis > 0)
                ipc::Thread::sleep(millis);
            return STATUS_OK;
        }

        size_t IDisplay::screens()
        {
            return 0;
        }

        size_t IDisplay::default_screen()
        {
            return 0;
        }

        status_t IDisplay::screen_size(size_t screen, ssize_t *w, ssize_t *h)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IDisplay::work_area_geometry(ws::rectangle_t *r)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        IWindow *IDisplay::create_window()
        {
            return NULL;
        }

        IWindow *IDisplay::create_window(size_t screen)
        {
            return NULL;
        }

        IWindow *IDisplay::create_window(void *handle)
        {
            return NULL;
        }

        IWindow *IDisplay::wrap_window(void *handle)
        {
            return NULL;
        }

        bool IDisplay::taskid_exists(taskid_t id)
        {
            for (size_t i=0, n=sTasks.size(); i<n; ++i)
            {
                dtask_t *task = sTasks.uget(i);
                if (task == NULL)
                    continue;
                if (task->nID == id)
                    return true;
            }
            return false;
        }

        taskid_t IDisplay::submit_task(timestamp_t time, task_handler_t handler, void *arg)
        {
            if (handler == NULL)
                return -STATUS_BAD_ARGUMENTS;

            sTasksLock.lock();
            lsp_finally { sTasksLock.unlock(); };

            ssize_t first = 0, last = sTasks.size() - 1;

            // Find the place to add the task
            while (first <= last)
            {
                ssize_t center  = (first + last) >> 1;
                dtask_t *t      = sTasks.uget(center);
                if (t->nTime <= time)
                    first           = center + 1;
                else
                    last            = center - 1;
            }

            // Generate task ID
            do
            {
                nTaskID     = (nTaskID + 1) & 0x7fffff;
            } while (taskid_exists(nTaskID));

            // Add task to the found place keeping it's time order
            dtask_t *t = sTasks.insert(first);
            if (t == NULL)
                return -STATUS_NO_MEM;

            t->nID          = nTaskID;
            t->nTime        = time;
            t->pHandler     = handler;
            t->pArg         = arg;

            // Trigger the queue if task has been submitted
            if (nTaskChanges++ == 0)
                task_queue_changed();

            return t->nID;
        }

        status_t IDisplay::cancel_task(taskid_t id)
        {
            if (id < 0)
                return STATUS_INVALID_UID;

            sTasksLock.lock();
            lsp_finally { sTasksLock.unlock(); };

            // Remove task from the queue
            for (size_t i=0; i<sTasks.size(); ++i)
                if (sTasks.uget(i)->nID == id)
                {
                    sTasks.remove(i);
                    return STATUS_OK;
                }

            return STATUS_NOT_FOUND;
        }

        status_t IDisplay::set_clipboard(size_t id, IDataSource *c)
        {
            if (c == NULL)
                return STATUS_BAD_ARGUMENTS;
            c->acquire();
            c->release();
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IDisplay::get_clipboard(size_t id, IDataSink *dst)
        {
            if (dst == NULL)
                return STATUS_BAD_ARGUMENTS;
            dst->acquire();
            dst->release();
            return STATUS_NOT_IMPLEMENTED;
        }

        bool IDisplay::drag_pending()
        {
            return false;
        }

        status_t IDisplay::reject_drag()
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IDisplay::accept_drag(IDataSink *sink, drag_t action, const rectangle_t *r)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        const char * const *IDisplay::get_drag_ctypes()
        {
            return NULL;
        }

        void IDisplay::set_main_callback(task_handler_t handler, void *arg)
        {
            sMainTask.pHandler      = handler;
            sMainTask.pArg          = arg;
        }

        status_t IDisplay::get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IDisplay::add_font(const char *name, const char *path)
        {
            if ((name == NULL) || (path == NULL))
                return STATUS_BAD_ARGUMENTS;

            LSPString tmp;
            if (!tmp.set_utf8(path))
                return STATUS_NO_MEM;

            return add_font(name, &tmp);
        }

        status_t IDisplay::add_font(const char *name, const io::Path *path)
        {
            if ((name == NULL) || (path == NULL))
                return STATUS_BAD_ARGUMENTS;
            return add_font(name, path->as_utf8());
        }

        status_t IDisplay::add_font(const char *name, const LSPString *path)
        {
            if (name == NULL)
                return STATUS_BAD_ARGUMENTS;

            io::InFileStream ifs;
            status_t res = ifs.open(path);
            if (res != STATUS_OK)
                return res;

            lsp_trace("Loading font '%s' from file '%s'", name, path->get_native());
            res = add_font(name, &ifs);
            status_t res2 = ifs.close();

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IDisplay::add_font(const char *name, io::IInStream *is)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IDisplay::add_font_alias(const char *name, const char *alias)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IDisplay::remove_font(const char *name)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        void IDisplay::remove_all_fonts()
        {
        }

        bool IDisplay::get_font_parameters(const Font &f, font_parameters_t *fp)
        {
            return false;
        }

        bool IDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
        {
            if (text == NULL)
                return false;
            LSPString tmp;
            if (!tmp.set_utf8(text))
                return false;
            return get_text_parameters(f, tp, &tmp, 0, tmp.length());
        }

        bool IDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text)
        {
            if (text == NULL)
                return false;
            return get_text_parameters(f, tp, text, 0, text->length());
        }

        bool IDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first)
        {
            if (text == NULL)
                return false;
            return get_text_parameters(f, tp, text, first, text->length());
        }

        bool IDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
        {
            return false;
        }

        const MonitorInfo *IDisplay::enum_monitors(size_t *count)
        {
            if (count != NULL)
                *count = 0;
            return NULL;
        }

        size_t IDisplay::idle_interval()
        {
            return nIdleInterval;
        }

        size_t IDisplay::set_idle_interval(size_t interval)
        {
            size_t old = nIdleInterval;
            nIdleInterval = interval;
            return old;
        }

    } /* namespace ws */
} /* namespace lsp */
