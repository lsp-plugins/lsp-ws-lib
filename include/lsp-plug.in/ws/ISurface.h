/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 25 окт. 2016 г.
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

#ifndef LSP_PLUG_IN_WS_ISURFACE_H_
#define LSP_PLUG_IN_WS_ISURFACE_H_

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/ws/Font.h>
#include <lsp-plug.in/ws/IGradient.h>

#define SURFMASK_NONE           0x00
#define SURFMASK_NO_CORNER      0x00
#define SURFMASK_LT_CORNER      0x01
#define SURFMASK_RT_CORNER      0x02
#define SURFMASK_RB_CORNER      0x04
#define SURFMASK_LB_CORNER      0x08
#define SURFMASK_ALL_CORNER     0x0f
#define SURFMASK_T_CORNER       (SURFMASK_LT_CORNER | SURFMASK_RT_CORNER)
#define SURFMASK_B_CORNER       (SURFMASK_LB_CORNER | SURFMASK_RB_CORNER)
#define SURFMASK_L_CORNER       (SURFMASK_LT_CORNER | SURFMASK_LB_CORNER)
#define SURFMASK_R_CORNER       (SURFMASK_RT_CORNER | SURFMASK_RB_CORNER)

namespace lsp
{
    namespace ws
    {
        class IDisplay;

        /**
         * Common drawing surface interface
         */
        class LSP_WS_LIB_PUBLIC ISurface
        {
            protected:
                size_t          nWidth;
                size_t          nHeight;
                surface_type_t  nType;

            protected:
                explicit ISurface(size_t width, size_t height, surface_type_t type);

            public:
                explicit ISurface();
                ISurface(const ISurface &) = delete;
                ISurface(ISurface &&) = delete;
                virtual ~ISurface();

                ISurface & operator = (const ISurface &) = delete;
                ISurface & operator = (ISurface &&) = delete;

            public:
                /** Get surface width
                 *
                 * @return surface width
                 */
                inline size_t width() const { return nWidth; }

                /** Get surface height
                 *
                 * @return surface height
                 */
                inline size_t height() const { return nHeight; }

                /** Get type of surface
                 *
                 * @return type of surface
                 */
                inline surface_type_t type()  const { return nType; }

            public:
                /**
                 * Return pointer to the owner's display
                 * @return pointer to the owner's display
                 */
                virtual IDisplay *display();

                /** Create child surface for drawing
                 * @param width surface width
                 * @param height surface height
                 * @return created surface or NULL
                 */
                virtual ISurface *create(size_t width, size_t height);

                /**
                 * Resize the surface. There is no guarantee about the image content
                 * stored inside of the surface.
                 *
                 * @param width new width of the surface
                 * @param height new height of the surface
                 * @return status of operation
                 */
                virtual status_t resize(size_t width, size_t height);

                /** Create linear gradient
                 *
                 * @param x0 x coordinate of the first point
                 * @param y0 y coordinate of the first point
                 * @param x1 x coordinate of the second point
                 * @param y1 y coordinate of the second point
                 * @return pointer to the gradient object or NULL on error
                 */
                virtual IGradient *linear_gradient(float x0, float y0, float x1, float y1);

                /** Create radial gradient
                 *
                 * @param cx0 x coordinate of the center of the blink
                 * @param cy0 y coordinate of the center of the blink
                 * @param cx1 x coordinate of the center of the radial gradient
                 * @param cy1 y coordinate of the center of the radial gradient
                 * @param r
                 * @return pointer to the gradient object or NULL on error
                 */
                virtual IGradient *radial_gradient
                (
                    float cx0, float cy0,
                    float cx1, float cy1,
                    float r
                );

                /** Destroy surface
                 *
                 */
                virtual void destroy();

                /** Start drawing on the surface
                 *
                 */
                virtual void begin();

                /** Complete drawing, synchronize surface with underlying device
                 *
                 */
                virtual void end();

                /**
                 * Check that surface is valid
                 * @return true if surface is valid
                 */
                virtual bool valid() const;

            public:
                /** Draw surface with alpha blending
                 *
                 * @param s surface to draw
                 * @param x offset from left
                 * @param y offset from top
                 * @param sx surface scale x
                 * @param sy surface scale y
                 * @param a alpha
                 */
                virtual void draw(ISurface *s, float x, float y, float sx, float sy, float a);

                /** Draw surface with alpha blending and rotating
                 *
                 * @param s surface to draw
                 * @param x offset from left
                 * @param y offset from top
                 * @param sx surface scale x                 * @param sy surface scale y
                 * @param ra rotation angle in radians
                 * @param a alpha
                 */
                virtual void draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a);

                /** Draw clipped surface
                 *
                 * @param s surface to draw
                 * @param x position to draw at
                 * @param y position to draw at
                 * @param sx source surface starting position
                 * @param sy source surface starting position
                 * @param sw source surface width
                 * @param sh source surface height
                 * @param a alpha
                 */
                virtual void draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a);

                /** Draw surface from BGRA32 memory chunk where alpha is premultiplied.
                 * That means that alpha of 0xff defines fully opaque color and 0x00
                 * defines fully transient color. That's why 50% transparent red is 0x80800000,
                 * not 0x80ff0000
                 *
                 * @param data pointer to data array
                 * @param width the width of the image
                 * @param height the height of the image
                 * @param stride the size of the row in bytes
                 * @param x offset from left
                 * @param y offset from top
                 * @param sx surface scale x
                 * @param sy surface scale y
                 * @param a alpha
                 */
                virtual void draw_raw(
                    const void *data, size_t width, size_t height, size_t stride,
                    float x, float y, float sx, float sy, float a);

                /** Wire rectangle with rounded corners that fits inside the specified area
                 *
                 * @param color rectangle color
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param radius the corner radius
                 * @param left left-top corner x coordinate
                 * @param top left-top corner y coordinate
                 * @param width width of rectangle
                 * @param height height of rectangle
                 * @param line_width width of line
                 */
                virtual void wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width);

                /** Wire rectangle with rounded corners that fits inside the specified area
                 *
                 * @param color rectangle color
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param radius the corner radius
                 * @param rect rectangle parameters
                 * @param line_width width of line
                 */
                virtual void wire_rect(const Color &c, size_t mask, float radius, const rectangle_t *rect, float line_width);

                /** Wire rectangle with rounded corners that fits inside the specified area
                 *
                 * @param g gradient to use
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param radius the corner radius
                 * @param left left-top corner x coordinate
                 * @param top left-top corner y coordinate
                 * @param width width of rectangle
                 * @param height height of rectangle
                 * @param line_width width of line
                 */
                virtual void wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width);

                /** Wire rectangle with rounded corners that fits inside the specified area
                 *
                 * @param g gradient to use
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param radius the corner radius
                 * @param rect rectangle parameters
                 * @param line_width width of line
                 */
                virtual void wire_rect(IGradient *g, size_t mask, float radius, const rectangle_t *rect, float line_width);

                /** Fill rectangle with rounded corners
                 *
                 * @param color rectangle color
                 * @param radius the corner radius
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param left left-top corner x coordinate
                 * @param top left-top corner y coordinate
                 * @param width width of rectangle
                 * @param height height of rectangle
                 */
                virtual void fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height);

                /** Fill rectangle with rounded corners
                 *
                 * @param g gradient to use
                 * @param radius the corner radius
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param r rectangle descriptor
                 */
                virtual void fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r);

                /** Fill rectangle with rounded corners
                 *
                 * @param g gradient to use
                 * @param radius the corner radius
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param left left-top corner x coordinate
                 * @param top left-top corner y coordinate
                 * @param width width of rectangle
                 * @param height height of rectangle
                 */
                virtual void fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height);

                /** Fill rectangle with rounded corners
                 *
                 * @param g gradient to use
                 * @param radius the corner radius
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param r rectangle descriptor
                 */
                virtual void fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r);

                /**
                 * Fill rectangle with rounded corners using contents of other surface.
                 * If rectangle size does not match surface size, the surface will be scaled
                 * using linear interpolation.
                 *
                 * @param s source surface to use
                 * @param alpha alpha blending factor (0.0 - opaque, 1.0 - fully transient)
                 * @param radius the corner radius
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param left left-top corner x coordinate
                 * @param top left-top corner y coordinate
                 * @param width width of rectangle
                 * @param height height of rectangle
                 */
                virtual void fill_rect(ISurface *s, float alpha, size_t mask, float radius, float left, float top, float width, float height);

                /**
                 * Fill rectangle with rounded corners using contents of other surface.
                 * If rectangle size does not match surface size, the surface will be scaled
                 * using linear interpolation.
                 *
                 * @param s source surface to use
                 * @param alpha alpha blending factor (0.0 - opaque, 1.0 - fully transient)
                 * @param radius the corner radius
                 * @param mask the corner mask:
                 *      0x01 - left-top corner is rounded
                 *      0x02 - right-top corner is rounded
                 *      0x04 - right-bottom corner is rounded
                 *      0x08 - left-bottom corner is rounded
                 * @param r rectangle descriptor
                 */
                virtual void fill_rect(ISurface *s, float alpha, size_t mask, float radius, const ws::rectangle_t *r);

                /** Fill sector of the round
                 *
                 * @param c color
                 * @param cx center of the round x coordinate
                 * @param cy center of the round y coordinate
                 * @param radius the radius of the round
                 * @param angle1 starting angle of the sector
                 * @param angle2 end angle of the sector
                 */
                virtual void fill_sector(const Color &c, float cx, float cy, float radius, float angle1, float angle2);

                /** Draw arc line
                 *
                 * @param c line color
                 * @param x center x
                 * @param y center y
                 * @param r radius
                 * @param a1 angle 1
                 * @param a2 angle 2
                 * @param width line width
                 */
                virtual void wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width);

                /** Fill triangle
                 *
                 * @param g gradient
                 * @param x0 vertex 0 x-coordinate
                 * @param y0 vertex 0 y-coordinate
                 * @param x1 vertex 1 x-coordinate
                 * @param y1 vertex 1 y-coordinate
                 * @param x2 vertex 2 x-coordinate
                 * @param y2 vertex 2 y-coordinate
                 */
                virtual void fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2);

                /** Fill triangle
                 *
                 * @param c color
                 * @param x0 vertex 0 x-coordinate
                 * @param y0 vertex 0 y-coordinate
                 * @param x1 vertex 1 x-coordinate
                 * @param y1 vertex 1 y-coordinate
                 * @param x2 vertex 2 x-coordinate
                 * @param y2 vertex 2 y-coordinate
                 */
                virtual void fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2);

                /** Fill circle
                 *
                 * @param c color
                 * @param x center x
                 * @param y center y
                 * @param r radius
                 */
                virtual void fill_circle(const Color &c, float x, float y, float r);

                /** Fill circle
                 *
                 * @param g gradient
                 * @param x center x
                 * @param y center y
                 * @param r radius
                 */
                virtual void fill_circle(IGradient *g, float x, float y, float r);

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

                /** Clear surface with specified color
                 *
                 * @param color color to use for clearing
                 */
                virtual void clear(const Color &color);

                /** Clear surface with specified color
                 *
                 * @param color RGB color to use for clear
                 */
                virtual void clear_rgb(uint32_t color);

                /** Clear surface with specified color
                 *
                 * @param color RGBA color to use for clear
                 */
                virtual void clear_rgba(uint32_t color);

                /** Ouput single-line text
                 *
                 * @param f font to use
                 * @param color text color
                 * @param x left position
                 * @param y top position
                 * @param text text to output
                 */
                virtual void out_text(const Font &f, const Color &color, float x, float y, const char *text);

                /**
                 * Output single-line text
                 * @param f font
                 * @param color color
                 * @param x left position
                 * @param y top position
                 * @param text text
                 */
                virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text);

                /**
                 * Output single-line text
                 * @param f font
                 * @param color color
                 * @param x left position
                 * @param y top position
                 * @param text text
                 * @param first index of first character
                 */
                virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first);

                /**
                 * Output single-line text
                 * @param f font
                 * @param color color
                 * @param x left position
                 * @param y top position
                 * @param text text
                 * @param first index of first character
                 * @param last index of last character
                 */
                virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last);

                /** Ouput single-line text relative to the specified position
                 *
                 * @param f font to use
                 * @param color text color
                 * @param x position left
                 * @param y position top
                 * @param dx relative offset by the x position. -1 = leftmost, +1 = rightmost, 0 = middle
                 * @param dy relative offset by the y position. -1 = topmost, +1 = bottommost, 0 = middle
                 * @param text text to output
                 */
                virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text);

                /** Ouput single-line text relative to the specified position
                 *
                 * @param f font to use
                 * @param color text color
                 * @param x position left
                 * @param y position top
                 * @param dx relative offset by the x position. -1 = leftmost, +1 = rightmost, 0 = middle
                 * @param dy relative offset by the y position. -1 = topmost, +1 = bottommost, 0 = middle
                 * @param text text to output
                 */
                virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text);

                /** Ouput single-line text relative to the specified position
                 *
                 * @param f font to use
                 * @param color text color
                 * @param x position left
                 * @param y position top
                 * @param dx relative offset by the x position. -1 = leftmost, +1 = rightmost, 0 = middle
                 * @param dy relative offset by the y position. -1 = topmost, +1 = bottommost, 0 = middle
                 * @param text text to output
                 * @param first first character of string
                 */
                virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first);

                /** Ouput single-line text relative to the specified position
                 *
                 * @param f font to use
                 * @param color text color
                 * @param x position left
                 * @param y position top
                 * @param dx relative offset by the x position. -1 = leftmost, +1 = rightmost, 0 = middle
                 * @param dy relative offset by the y position. -1 = topmost, +1 = bottommost, 0 = middle
                 * @param text text to output
                 * @param first first character of string
                 * @param last last character of string
                 */
                virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last);

                /** Draw line
                 *
                 * @param c line color
                 * @param x0 first point x coordinate
                 * @param y0 first point y coordinate
                 * @param x1 second point x coordinate
                 * @param y1 second point y coordinate
                 * @param width line width
                 */
                virtual void line(const Color &c, float x0, float y0, float x1, float y1, float width);

                /** Draw line
                 *
                 * @param g gradient
                 * @param x0 first point x coordinate
                 * @param y0 first point y coordinate
                 * @param x1 second point x coordinate
                 * @param y1 second point y coordinate
                 * @param width line width
                 */
                virtual void line(IGradient *g, float x0, float y0, float x1, float y1, float width);

                /** Draw parametric line defined by equation a*x + b*y + c = 0
                 *
                 * @param color line color
                 * @param a the x multiplier
                 * @param b the y multiplier
                 * @param c the shift
                 * @param width line width
                 */
                virtual void parametric_line(const Color &color, float a, float b, float c, float width);

                /** Draw parameteric line defined by equation a*x + b*y + c = 0 and cull it by two of
                 * specified boundaries which are selected depending on the slope of the line.
                 *
                 * @param color line color
                 * @param a the x multiplier
                 * @param b the y multiplier
                 * @param c the shift
                 * @param left coordinates of left culling boundary
                 * @param right coordinates of right culling boundary
                 * @param top corrdinates of top culling boundary
                 * @param bottom coordinates of bottom culling boundary
                 * @param width line width
                 */
                virtual void parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width);

                /** Draw parametric bar defined by two line equations
                 *
                 * @param gr gradient to fill bar
                 * @param a1 the x multiplier 1
                 * @param b1 the y multiplier 1
                 * @param c1 the shift 1
                 * @param a2 the x multiplier 2
                 * @param b2 the y multiplier 2
                 * @param c2 the shift 2
                 * @param left
                 * @param right
                 * @param top
                 * @param bottom
                 */
                virtual void parametric_bar(
                    IGradient *gr,
                    float a1, float b1, float c1, float a2, float b2, float c2,
                    float left, float right, float top, float bottom);

                /**
                 * Draw the rectangle with the rectangle hole inside. The rectangle hole
                 * additionally allows to round corners.
                 *
                 * @param color color of the rectangle
                 * @param flags flags that indicate the rounding of the corresponding corner
                 * @param radius the radius of the rounding for all corners
                 * @param fx the left coordinate of the outer rectangle
                 * @param fy the top coordinate of the outer rectangle
                 * @param fw the width of the outer rectangle
                 * @param fh the height of the outer rectangle
                 * @param ix the left coordinate of the inner rectangle
                 * @param iy the top coordinate of the inner rectangle
                 * @param iw the width of the inner rectangle
                 * @param ih the height of the inner rectangle
                 */
                virtual void fill_frame(
                    const Color &color,
                    size_t flags, float radius,
                    float fx, float fy, float fw, float fh,
                    float ix, float iy, float iw, float ih);

                /**
                 * Draw the rectangle with the rectangle hole inside. The rectangle hole
                 * additionally allows to round corners.
                 *
                 * @param color color of the rectangle
                 * @param flags flags that indicate the rounding of the corresponding corner
                 * @param radius the radius of the rounding for all corners
                 * @param out the parameters of the outer rectangle
                 * @param in the parameters of the inner rectangle
                 */
                virtual void fill_frame(
                    const Color &color,
                    size_t flags, float radius,
                    const ws::rectangle_t *out, const ws::rectangle_t *in);

                /** Draw polygon
                 *
                 * @param color polygon color
                 * @param x array of x point coordinates
                 * @param y array of y point coordinates
                 * @param n number of elements in each array
                 */
                virtual void fill_poly(const Color & color, const float *x, const float *y, size_t n);

                /** Draw polygon
                 *
                 * @param gr gradient to fill
                 * @param x array of x point coordinates
                 * @param y array of y point coordinates
                 * @param n number of elements in each array
                 */
                virtual void fill_poly(IGradient *gr, const float *x, const float *y, size_t n);

                /** Wire polygon
                 *
                 * @param color polygon line color
                 * @param x array of x point coordinates
                 * @param y array of y point coordinates
                 * @param n number of elements in each array
                 * @param width line width
                 */
                virtual void wire_poly(const Color & color, float width, const float *x, const float *y, size_t n);

                /** Draw filled and wired polygon
                 *
                 * @param fill polygon fill color
                 * @param wire polygon wire color
                 * @param x array of x point coordinates
                 * @param y array of y point coordinates
                 * @param n number of elements in each array
                 * @param width line width
                 */
                virtual void draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n);

                /**
                 * Begin clipping of the rectangle area
                 * @param x left-top corner X coordinate
                 * @param y left-top corner Y coordinate
                 * @param w width
                 * @param h height
                 */
                virtual void clip_begin(float x, float y, float w, float h);

                /**
                 * Begin clipping of the rectangle area
                 * @param area the area to clip
                 */
                virtual void clip_begin(const ws::rectangle_t *area);

                /**
                 * End clipping
                 */
                virtual void clip_end();

                /** Get anti-aliasing
                 *
                 * @return anti-aliasing state
                 */
                virtual bool get_antialiasing();

                /** Set anti-aliasing
                 *
                 * @param set new anti-aliasing state
                 * @return previous anti-aliasing state
                 */
                virtual bool set_antialiasing(bool set);

                /**
                 * Set up drawing origin
                 * @param origin drawing origin (left and top coordinates)
                 * @return old drawing origin value
                 */
                virtual ws::point_t set_origin(const ws::point_t & origin);

                /**
                 * Set up drawing origin
                 * @param left left coordinate of drawing origin
                 * @param top top coordinate of drawing origin
                 * @return
                 */
                virtual ws::point_t set_origin(ssize_t left, ssize_t top);
        };

    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_ISURFACE_H_ */
