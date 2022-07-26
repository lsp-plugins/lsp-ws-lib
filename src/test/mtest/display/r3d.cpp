/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 25 июл. 2022 г.
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

#include <lsp-plug.in/ws/factory.h>
#include <lsp-plug.in/ws/IEventHandler.h>
#include <lsp-plug.in/ws/IR3DBackend.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/mtest.h>

#define FRAME_PERIOD        (1000 / 25) /* Launch at 25 Hz rate */

namespace lsp
{
    namespace
    {
        #define D3(x, y, z)     { x, y, z, 1.0f     }
        #define V3(dx, dy, dz)  { dx, dy, dz, 0.0f  }
        #define C3(r, g, b)     { r, g, b, 0.0f     }

        typedef struct axis_point3d_t
        {
            r3d::dot4_t     v;
            r3d::color_t    c;
        } axis_point3d_t;

        static axis_point3d_t axis_lines[] =
        {
            // X axis (red)
            { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 2.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            // Y axis (green)
            { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { 0.0f, 2.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            // Z axis (blue)
            { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
            { { 0.0f, 0.0f, 2.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        static axis_point3d_t dot[] = {
            { { 0.0f, 0.0f, 0.0f, 1.0f}, { 1.0f, 1.0f, 1.0f, 1.0f } }
        };

        // 3D mesh data for box: vertex, normal, color
        static r3d::dot4_t box_vertex[] =
        {
            D3(  1.0f,  1.0f,  1.0f ),
            D3( -1.0f,  1.0f,  1.0f ),
            D3( -1.0f, -1.0f,  1.0f ),
            D3(  1.0f, -1.0f,  1.0f ),

            D3(  1.0f,  1.0f, -1.0f ),
            D3( -1.0f,  1.0f, -1.0f ),
            D3( -1.0f, -1.0f, -1.0f ),
            D3(  1.0f, -1.0f, -1.0f )
        };

        static r3d::color_t box_colors[] =
        {
            C3( 1.0f, 0.0f, 0.0f ),
            C3( 0.0f, 1.0f, 0.0f ),
            C3( 0.0f, 0.0f, 1.0f ),
            C3( 1.0f, 1.0f, 0.0f ),

            C3( 1.0f, 0.0f, 1.0f ),
            C3( 0.0f, 1.0f, 1.0f ),
            C3( 1.0f, 1.0f, 1.0f ),
            C3( 0.5f, 0.5f, 0.5f )
        };

        static r3d::vec4_t box_normal[] =
        {
            V3(  1.0f,  0.0f,  0.0f ),
            V3( -1.0f,  0.0f,  0.0f ),
            V3(  0.0f,  1.0f,  0.0f ),
            V3(  0.0f, -1.0f,  0.0f ),
            V3(  0.0f,  0.0f,  1.0f ),
            V3(  0.0f,  0.0f, -1.0f )
        };

        // Indices
        static uint32_t box_vertex_idx[] =
        {
            0, 1, 2,  0, 2, 3,
            0, 4, 5,  0, 5, 1,
            1, 5, 6,  1, 6, 2,
            0, 3, 7,  0, 7, 4,
            3, 2, 6,  3, 6, 7,
            5, 4, 7,  5, 7, 6
        };

        static uint32_t box_normal_idx[] =
        {
            4, 4, 4,  4, 4, 4,
            2, 2, 2,  2, 2, 2,
            1, 1, 1,  1, 1, 1,
            0, 0, 0,  0, 0, 0,
            3, 3, 3,  3, 3, 3,
            5, 5, 5,  5, 5, 5,
        };

    #ifdef ARCH_LE
        static void abgr32_to_prgba32(void *dst, const void *src, size_t count)
        {
            const uint32_t *s   = reinterpret_cast<const uint32_t *>(src);
            uint32_t *d         = reinterpret_cast<uint32_t *>(dst);

            for (size_t i=0; i<count; ++i)
            {
                uint32_t c      = s[i];
                d[i]            = 0xff000000 | c;
            }
        }
    #else /* ARCH_BE */
        static void abgr32_to_prgba32(void *dst, const void *src, size_t count)
        {
            const uint32_t *s   = reinterpret_cast<const uint32_t *>(src);
            uint32_t *d         = reinterpret_cast<uint32_t *>(dst);

            for (size_t i=0; i<count; ++i)
            {
                uint32_t c      = s[i];
                d[i]            = 0x000000ff | c;
            }
        }
    #endif

        static void init_point_xyz(r3d::dot4_t *p, float x, float y, float z)
        {
            p->x        = x;
            p->y        = y;
            p->z        = z;
            p->w        = 1.0f;
        }

        static void init_vector_dxyz(r3d::vec4_t *v, float dx, float dy, float dz)
        {
            v->dx       = dx;
            v->dy       = dy;
            v->dz       = dz;
            v->dw       = 0.0f;
        }

        void init_vector_p2(r3d::vec4_t *v, const r3d::dot4_t *p1, const r3d::dot4_t *p2)
        {
            v->dx       = p2->x - p1->x;
            v->dy       = p2->y - p1->y;
            v->dz       = p2->z - p1->z;
            v->dw       = 0.0f;
        }

        static void init_matrix_identity(r3d::mat4_t *m)
        {
            float *v    = m->m;

            v[0]        = 1.0f;
            v[1]        = 0.0f;
            v[2]        = 0.0f;
            v[3]        = 0.0f;

            v[4]        = 0.0f;
            v[5]        = 1.0f;
            v[6]        = 0.0f;
            v[7]        = 0.0f;

            v[8]        = 0.0f;
            v[9]        = 0.0f;
            v[10]       = 1.0f;
            v[11]       = 0.0f;

            v[12]       = 0.0f;
            v[13]       = 0.0f;
            v[14]       = 0.0f;
            v[15]       = 1.0f;
        }

        static void init_matrix_rotate_z(r3d::mat4_t *m, float angle)
        {
            float s     = sinf(angle);
            float c     = cosf(angle);
            float *M    = m->m;

            M[0]        = c;
            M[1]        = s;
            M[2]        = 0.0f;
            M[3]        = 0.0f;
            M[4]        = -s;
            M[5]        = c;
            M[6]        = 0.0f;
            M[7]        = 0.0f;
            M[8]        = 0.0f;
            M[9]        = 0.0f;
            M[10]       = 1.0f;
            M[11]       = 0.0f;
            M[12]       = 0.0f;
            M[13]       = 0.0f;
            M[14]       = 0.0f;
            M[15]       = 1.0f;
        }

        static void init_matrix_translate(r3d::mat4_t *m, float dx, float dy, float dz)
        {
            float *v    = m->m;

            v[0]        = 1.0f;
            v[1]        = 0.0f;
            v[2]        = 0.0f;
            v[3]        = 0.0f;

            v[4]        = 0.0f;
            v[5]        = 1.0f;
            v[6]        = 0.0f;
            v[7]        = 0.0f;

            v[8]        = 0.0f;
            v[9]        = 0.0f;
            v[10]       = 1.0f;
            v[11]       = 0.0f;

            v[12]       = dx;
            v[13]       = dy;
            v[14]       = dz;
            v[15]       = 1.0;
        }

        static void init_matrix_frustum(r3d::mat4_t *m, float left, float right, float bottom, float top, float znear, float zfar)
        {
            float *M    = m->m;
            M[0]        = 2.0f * znear / (right - left);
            M[1]        = 0.0f;
            M[2]        = 0.0f;
            M[3]        = 0.0f;

            M[4]        = 0.0f;
            M[5]        = 2.0f * znear / (top - bottom);
            M[6]        = 0.0f;
            M[7]        = 0.0f;

            M[8]        = (right + left) / (right - left);
            M[9]        = (top + bottom) / (top - bottom);
            M[10]       = - (zfar + znear) / (zfar - znear);
            M[11]       = -1.0f;

            M[12]       = 0.0f;
            M[13]       = 0.0f;
            M[14]       = -2.0f * zfar * znear / (zfar - znear);
            M[15]       = 0.0f;
        }

        static void init_matrix_lookat_p1v2(r3d::mat4_t *m, const r3d::dot4_t *pov, const r3d::vec4_t *fwd, const r3d::vec4_t *up)
        {
            r3d::vec4_t f, s, u;
            float fw, fs;

            // Normalize forward vector
            fw      = sqrtf(fwd->dx*fwd->dx + fwd->dy*fwd->dy + fwd->dz*fwd->dz);
            f.dx    = fwd->dx / fw;
            f.dy    = fwd->dy / fw;
            f.dz    = fwd->dz / fw;
            f.dw    = 0.0f;

            // Compute and normalize side vector
            s.dx    = f.dy*up->dz - f.dz*up->dy;
            s.dy    = f.dz*up->dx - f.dx*up->dz;
            s.dz    = f.dx*up->dy - f.dy*up->dx;
            s.dw    = 0.0f;

            fs      = sqrtf(s.dx*s.dx + s.dy*s.dy + s.dz*s.dz);
            s.dx   /= fs;
            s.dy   /= fs;
            s.dz   /= fs;

            // Compute orthogonal up vector
            u.dx    = f.dy*s.dz - f.dz*s.dy;
            u.dy    = f.dz*s.dx - f.dx*s.dz;
            u.dz    = f.dx*s.dy - f.dy*s.dx;
            u.dw    = 0.0f;

            // Fill matrix
            float *M    = m->m;
            M[0]    =  s.dx;
            M[1]    =  u.dx;
            M[2]    =  f.dx;
            M[3]    =  0.0f;

            M[4]    =  s.dy;
            M[5]    =  u.dy;
            M[6]    =  f.dy;
            M[7]    =  0.0f;

            M[8]    =  s.dz;
            M[9]    =  u.dz;
            M[10]   =  f.dz;
            M[11]   =  0.0f;

            M[12]   = -(s.dx*pov->x + s.dy*pov->y + s.dz*pov->z);
            M[13]   = -(u.dx*pov->x + u.dy*pov->y + u.dz*pov->z);
            M[14]   = -(f.dx*pov->x + f.dy*pov->y + f.dz*pov->z);
            M[15]   =  1.0f;
        }
    } /* namespace */
} /* namespace lsp */

MTEST_BEGIN("ws.display", r3d)

    class Handler: public ws::IEventHandler
    {
        private:
            test_type_t        *pTest;
            ws::IWindow        *pWnd;
            ws::IR3DBackend    *pBackend;
            ws::taskid_t        nTaskId;

            r3d::mat4_t         sWorld;
            r3d::mat4_t         sView;
            r3d::mat4_t         sProj;

            size_t              nPeriod;
            size_t              nYaw;
            size_t              nStep;

            float               fFov;
            r3d::dot4_t         sPov;
            r3d::dot4_t         sDst;
            r3d::vec4_t         sTop;

            bool                bOversampling;

        protected:
            static  status_t execute_timer(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
            {
                Handler *pthis      = static_cast<Handler *>(arg);
                if (pthis == NULL)
                    return STATUS_OK;

                pthis->nTaskId      = -1;
                pthis->on_timer();
                pthis->launch(time + FRAME_PERIOD);

                return STATUS_OK;
            }

        public:
            inline Handler(test_type_t *test, ws::IWindow *wnd, ws::IR3DBackend *r3d)
            {
                pTest       = test;
                pWnd        = wnd;
                pBackend    = r3d;
                nTaskId     = -1;

                nPeriod     = 0x100000;
                nYaw        = 0;
                nStep       = (nPeriod * FRAME_PERIOD) / 10000.0f;

                fFov        = 70.0f;

                bOversampling= false;

                init_point_xyz(&sPov, 3.0f, 0.6f, 2.1f);
                init_point_xyz(&sDst, 0.0f, 0.0f, 0.0f);
                init_vector_dxyz(&sTop, 0.0f, 0.0f, -1.0f);
                init_matrix_identity(&sWorld);
            }

            void launch(ws::timestamp_t deadline = 0)
            {
                nTaskId     = pWnd->display()->submit_task(deadline, execute_timer, this);
            }

            void stop()
            {
                if (nTaskId >= 0)
                    pWnd->display()->cancel_task(nTaskId);
            }

        public:
            void on_timer()
            {
                // Update mesh matrix
                nYaw            = (nYaw + nStep) % nPeriod;
                float yaw       = (2.0f * M_PI * nYaw) / float(nPeriod);

                init_matrix_rotate_z(&sWorld, -yaw);

                // Query area for redraw
                pWnd->invalidate();
            }

            void draw(ws::IR3DBackend *r3d)
            {
                ssize_t vx, vy, vw, vh;
                r3d::mat4_t mat;

                r3d->get_location(&vx, &vy, &vw, &vh);

                // Compute frustum matrix
                float aspect    = float(vw)/float(vh);
                float zNear     = 0.1f;
                float zFar      = 1000.0f;

                float fH        = tanf( fFov * M_PI / 360.0f) * zNear;
                float fW        = fH * aspect;
                init_matrix_frustum(&sProj, -fW, fW, -fH, fH, zNear, zFar);

                // Compute view matrix
                r3d::vec4_t dir;
                init_vector_p2(&dir, &sDst, &sPov);
                init_matrix_lookat_p1v2(&sView, &sPov, &dir, &sTop);

                // Update matrices
                r3d->set_matrix(r3d::MATRIX_WORLD, &sWorld);
                r3d->set_matrix(r3d::MATRIX_PROJECTION, &sProj);
                r3d->set_matrix(r3d::MATRIX_VIEW, &sView);

                // Now we can perform rendering
                // Set Light parameters
                r3d::light_t light;

                light.type          = r3d::LIGHT_SPOT;
                light.position      = sPov;
                light.direction.dx  = -dir.dx;
                light.direction.dy  = -dir.dy;
                light.direction.dz  = -dir.dz;
                light.direction.dw  = 0.0f;

                light.ambient.r     = 0.5f;
                light.ambient.g     = 0.5f;
                light.ambient.b     = 0.5f;
                light.ambient.a     = 1.0f;

                light.diffuse.r     = 0.5f;
                light.diffuse.g     = 0.5f;
                light.diffuse.b     = 0.5f;
                light.diffuse.a     = 1.0f;

                light.specular.r    = 0.5f;
                light.specular.g    = 0.5f;
                light.specular.b    = 0.5f;
                light.specular.a    = 1.0f;

                light.constant      = 1.0f;
                light.linear        = 0.0f;
                light.quadratic     = 0.0f;
                light.cutoff        = 180.0f;

                // Enable/disable lighting
                r3d->set_lights(&light, 1);

                r3d::buffer_t buf;

                // Draw axes
                r3d::init_buffer(&buf);

                buf.type            = r3d::PRIMITIVE_LINES;
                buf.width           = 2.0f * (1.0f + bOversampling);
                buf.count           = sizeof(axis_lines) / (sizeof(axis_point3d_t) * 2);
                buf.flags           = 0;

                buf.vertex.data     = &axis_lines[0].v;
                buf.vertex.stride   = sizeof(axis_point3d_t);

                buf.color.data      = &axis_lines[0].c;
                buf.color.stride    = sizeof(axis_point3d_t);

                r3d->draw_primitives(&buf);

                // Draw dot
                r3d::init_buffer(&buf);

                init_matrix_translate(&mat, 1.5f, 1.5f, 1.5f);

                buf.model           = mat;
                buf.type            = r3d::PRIMITIVE_POINTS;
                buf.width           = 8.0f * (1.0f + bOversampling);
                buf.count           = 1;
                buf.flags           = 0;

                buf.vertex.data     = &dot[0].v;
                buf.vertex.stride   = sizeof(axis_point3d_t);

                buf.color.data      = &dot[0].c;
                buf.color.stride    = sizeof(axis_point3d_t);

                r3d->draw_primitives(&buf);

                // Draw box
                r3d::init_buffer(&buf);

                buf.type            = r3d::PRIMITIVE_TRIANGLES;
                buf.width           = 1.0f;
                buf.count           = sizeof(box_vertex_idx) / (sizeof(uint32_t) * 3);
                buf.flags           = r3d::BUFFER_LIGHTING;

                buf.vertex.data     = box_vertex;
                buf.vertex.stride   = sizeof(r3d::vec4_t);
                buf.vertex.index    = box_vertex_idx;

                buf.normal.data     = box_normal;
                buf.normal.stride   = sizeof(r3d::vec4_t);
                buf.normal.index    = box_normal_idx;

                buf.color.data      = box_colors;
                buf.color.stride    = sizeof(r3d::color_t);
                buf.color.index     = box_vertex_idx;

                r3d->draw_primitives(&buf);
            }

            virtual status_t handle_event(const ws::event_t *ev) override
            {
                switch (ev->nType)
                {
                    case ws::UIE_MOUSE_SCROLL:
                    {
                        bOversampling   = ! bOversampling;
                        pTest->printf("Oversampling: %s", (bOversampling) ? "ON" : "OFF");
                        break;
                    }

                    case ws::UIE_REDRAW:
                    {
                        Color c(1.0f, 1.0f, 1.0f);
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        size_t ww   = pWnd->width() - 16;
                        size_t wh   = pWnd->height() - 16;
                        size_t rw   = ww * (1 + bOversampling);
                        size_t rh   = wh * (1 + bOversampling);

                        // Perform drawing
                        s->begin();
                        {
                            // Set background color for backend
                            if (pBackend != NULL)
                            {
                                r3d::color_t rc;
                                rc.r    = 0.0f;
                                rc.g    = 0.0f;
                                rc.b    = 0.0f;
                                rc.a    = 1.0f;
                                pBackend->set_bg_color(&rc);

                                // Locate the backend
                                pBackend->locate(8, 8, rw, rh);
                                pWnd->display()->sync();

                                // Allocate buffer for drawing
                                size_t count        = rw * rh;
                                size_t stride       = rw * sizeof(uint32_t);
                                uint8_t *buf        = static_cast<uint8_t *>(malloc(count * sizeof(uint32_t)));
                                lsp_finally{ free(buf); };

                                pBackend->begin_draw();
                                    draw(pBackend);
                                    pBackend->sync();
                                    pBackend->read_pixels(buf, stride, r3d::PIXEL_BGRA);
                                pBackend->end_draw();

                                // Draw the contents of the data read from the R3D content
                                abgr32_to_prgba32(buf, buf, count);

                                s->clear(c);
                                s->draw_raw(buf, rw, rh, stride,
                                    8, 8, float(ww) / float(rw), float(wh) / float(rh), 0.0f);
                            }
                            else
                                s->clear(c);
                        }
                        s->end();

                        return STATUS_OK;
                    }

                    case ws::UIE_CLOSE:
                    {
                        pWnd->hide();
                        pWnd->display()->quit_main();
                        break;
                    }

                    default:
                        return IEventHandler::handle_event(ev);
                }

                return STATUS_OK;
            }
    };

    MTEST_MAIN
    {
        ws::IDisplay *dpy = ws::lsp_ws_create_display(0, NULL);
        MTEST_ASSERT(dpy != NULL);
        lsp_finally { ws::lsp_ws_free_display(dpy); };

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally {
            wnd->destroy();
            delete wnd;
        };

        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test 3D rendering") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_ALL) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(320, 200, 640, 400) == STATUS_OK);

        ws::IR3DBackend *r3d = dpy->create_r3d_backend(wnd);
        MTEST_ASSERT(r3d != NULL);
        lsp_finally {
            r3d->destroy();
            delete r3d;
        };

        Handler h(this, wnd, r3d);
        h.launch();
        lsp_finally{ h.stop(); };

        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END





