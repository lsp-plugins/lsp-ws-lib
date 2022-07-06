/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#define SURFMASK_T_CORNER       0x03
#define SURFMASK_B_CORNER       0x0c
#define SURFMASK_L_CORNER       0x09
#define SURFMASK_R_CORNER       0x06

namespace lsp
{
    namespace ws
    {
        enum surf_line_cap_t
        {
            SURFLCAP_BUTT,
            SURFLCAP_ROUND,
            SURFLCAP_SQUARE
        };

        /** Common drawing surface interface
         *
         */
        class ISurface
        {
            private:
                ISurface & operator = (const ISurface &);
                ISurface(const ISurface &);

            protected:
                size_t          nWidth;
                size_t          nHeight;
                size_t          nStride;
                uint8_t        *pData;
                surface_type_t  nType;

            protected:
                explicit ISurface(size_t width, size_t height, surface_type_t type);

            public:
                explicit ISurface();
                virtual ~ISurface();

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
                /** Create child surface for drawing
                 * @param width surface width
                 * @param height surface height
                 * @return created surface or NULL
                 */
                virtual ISurface *create(size_t width, size_t height);

                /**
                 * Create copy of current surface
                 * @return copy of current surface
                 */
                virtual ISurface *create_copy();

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

            public:
                /** Draw surface
                 *
                 * @param s surface to draw
                 * @param x offset from left
                 * @param y offset from top
                 */
                virtual void draw(ISurface *s, float x, float y);

                /** Draw surface
                 *
                 * @param s surface to draw
                 * @param x offset from left
                 * @param y offset from top
                 * @param sx surface scale x
                 * @param sy surface scale y
                 */
                virtual void draw(ISurface *s, float x, float y, float sx, float sy);

                /**
                 * Draw surface
                 * @param s surface to draw
                 * @param r the rectangle to place the surface
                 */
                virtual void draw(ISurface *s, const ws::rectangle_t *r);

                /** Draw surface with alpha blending
                 *
                 * @param s surface to draw
                 * @param x offset from left
                 * @param y offset from top
                 * @param sx surface scale x
                 * @param sy surface scale y
                 * @param a alpha
                 */
                virtual void draw_alpha(ISurface *s, float x, float y, float sx, float sy, float a);

                /** Draw surface with alpha blending and rotating
                 *
                 * @param s surface to draw
                 * @param x offset from left
                 * @param y offset from top
                 * @param sx surface scale x
                 * @param sy surface scale y
                 * @param ra rotation angle in radians
                 * @param a alpha
                 */
                virtual void draw_rotate_alpha(ISurface *s, float x, float y, float sx, float sy, float ra, float a);

                /** Draw clipped surface
                 *
                 * @param s surface to draw
                 * @param x position to draw at
                 * @param y position to draw at
                 * @param sx source surface starting position
                 * @param sy source surface starting position
                 * @param sw source surface width
                 * @param sh source surface height
                 */
                virtual void draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh);

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

                /** Fill sector of the round
                 *
                 * @param cx center of the round x coordinate
                 * @param cy center of the round y coordinate
                 * @param radius the radius of the round
                 * @param angle1 starting angle of the sector
                 * @param angle2 end angle of the sector
                 * @param color color
                 */
                virtual void fill_sector(float cx, float cy, float radius, float angle1, float angle2, const Color &color);

                /** Fill rectangle
                 *
                 * @param x0 vertex 0 x-coordinate
                 * @param y0 vertex 0 y-coordinate
                 * @param x1 vertex 1 x-coordinate
                 * @param y1 vertex 1 y-coordinate
                 * @param x2 vertex 2 x-coordinate
                 * @param y2 vertex 2 y-coordinate
                 * @param g gradient
                 */
                virtual void fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, IGradient *g);

                /** Fill rectangle
                 *
                 * @param x0 vertex 0 x-coordinate
                 * @param y0 vertex 0 y-coordinate
                 * @param x1 vertex 1 x-coordinate
                 * @param y1 vertex 1 y-coordinate
                 * @param x2 vertex 2 x-coordinate
                 * @param y2 vertex 2 y-coordinate
                 * @param c color
                 */
                virtual void fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, const Color &color);

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
                 * @param x0 first point x coordinate
                 * @param y0 first point y coordinate
                 * @param x1 second point x coordinate
                 * @param y1 second point y coordinate
                 * @param width line width
                 * @param color line color
                 */
                virtual void line(float x0, float y0, float x1, float y1, float width, const Color &color);

                /** Draw line
                 *
                 * @param x0 first point x coordinate
                 * @param y0 first point y coordinate
                 * @param x1 second point x coordinate
                 * @param y1 second point y coordinate
                 * @param width line width
                 * @param g gradient
                 */
                virtual void line(float x0, float y0, float x1, float y1, float width, IGradient *g);

                /** Draw parametric line defined by equation a*x + b*y + c = 0
                 *
                 * @param a the x multiplier
                 * @param b the y multiplier
                 * @param c the shift
                 * @param width line width
                 * @param color line color
                 */
                virtual void parametric_line(float a, float b, float c, float width, const Color &color);

                /** Draw parameteric line defined by equation a*x + b*y + c = 0 and cull it by specified boundaries
                 *
                 * @param a the x multiplier
                 * @param b the y multiplier
                 * @param c the shift
                 * @param left
                 * @param right
                 * @param top
                 * @param bottom
                 * @param width line width
                 * @param color line color
                 */
                virtual void parametric_line(float a, float b, float c, float left, float right, float top, float bottom, float width, const Color &color);

                /** Draw parametric bar defined by two line equations
                 *
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
                 * @param gr gradient to fill bar
                 */
                virtual void parametric_bar(float a1, float b1, float c1, float a2, float b2, float c2,
                        float left, float right, float top, float bottom, IGradient *gr);

                /** Draw arc
                 *
                 * @param x center x
                 * @param y center y
                 * @param r radius
                 * @param a1 angle 1
                 * @param a2 angle 2
                 * @param width line width
                 * @param color line color
                 */
                virtual void wire_arc(float x, float y, float r, float a1, float a2, float width, const Color &color);

                virtual void fill_frame(const Color &color,
                    float fx, float fy, float fw, float fh,
                    float ix, float iy, float iw, float ih);

                virtual void fill_frame(const Color &color, const ws::rectangle_t *out, const ws::rectangle_t *in);

                virtual void fill_round_frame(
                    const Color &color,
                    float radius, size_t flags,
                    float fx, float fy, float fw, float fh,
                    float ix, float iy, float iw, float ih);

                virtual void fill_round_frame(
                    const Color &color, float radius, size_t flags,
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

                /** Fill circle
                 *
                 * @param x center x
                 * @param y center y
                 * @param r radius
                 * @param color color
                 */
                virtual void fill_circle(float x, float y, float r, const Color & color);

                /** Fill circle
                 *
                 * @param x center x
                 * @param y center y
                 * @param r radius
                 * @param g gradient
                 */
                virtual void fill_circle(float x, float y, float r, IGradient *g);

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

                /** Get line cap
                 *
                 * @return line cap
                 */
                virtual surf_line_cap_t get_line_cap();

                /** Set line cap
                 *
                 * @param lc line cap
                 * @return line cap
                 */
                virtual surf_line_cap_t set_line_cap(surf_line_cap_t lc);

                /** Return difference (in bytes) between two sequential rows
                 *
                 * @return stride between rows
                 */
                virtual     size_t stride();

                /**
                 * Return raw buffer data
                 *
                 * @return raw buffer data
                 */
                virtual     void *data();

                /**
                 * Return pointer to the beginning of the specified row
                 * @param row row number
                 */
                virtual     void *row(size_t row);

                /**
                 * Start direct access to the surface
                 * @return pointer to surface buffer or NULL if error/not possible
                 */
                virtual     void *start_direct();

                /**
                 * End direct access to the surface
                 */
                virtual     void end_direct();
        };
    }

} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_ISURFACE_H_ */
