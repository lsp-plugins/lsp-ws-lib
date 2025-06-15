/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *           (C) 2025 Marvin Edeler <marvin.edeler@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 12 June 2025
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

#ifdef PLATFORM_MACOSX

#import <private/cocoa/CocoaCairoView.h>
#include <private/cocoa/CocoaCairoSurface.h>

#include <cairo.h>
#include <cairo-quartz.h>

@implementation CocoaCairoView

// We need an overloaded objective c - NSView Class for Rendering, drawRect is triggered on create/update
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    // Randomize color to see updates - just for testing
    [self randomizeColor];
    // Switch random bewteen circle and triange draw - just for testing
    BOOL randomBool = arc4random_uniform(2);
    if (randomBool) {
        [self drawTriangle];
    } else {
        [self drawCricle];
    }
}

// Draw a triangle for testing...
- (void)drawTriangle {
    CGContextRef cg = [[NSGraphicsContext currentContext] CGContext];
    
    cairo_surface_t* surface = cairo_quartz_surface_create_for_cg_context(cg, self.bounds.size.width, self.bounds.size.height);
    cairo_t* cr = cairo_create(surface);

    // Clear background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);  // Dark gray
    cairo_paint(cr);

    // Define triangle points
    double cx = self.bounds.size.width / 2.0;
    double cy = self.bounds.size.height / 2.0;
    double size =  100;

    cairo_move_to(cr, cx, cy - size);          // Top point
    cairo_line_to(cr, cx - size, cy + size);   // Bottom left
    cairo_line_to(cr, cx + size, cy + size);   // Bottom right
    cairo_close_path(cr);

    // Fill with color
    float red = (float)(arc4random() % 256) / 255.0f;
    float green = (float)(arc4random() % 256) / 255.0f;
    float blue = (float)(arc4random() % 256) / 255.0f;
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_fill(cr);

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

// Draw a cricle for testing...
- (void)drawCricle {
    CGContextRef cg = [[NSGraphicsContext currentContext] CGContext];

    cairo_surface_t *surface = cairo_quartz_surface_create_for_cg_context(
        cg,
        self.bounds.size.width,
        self.bounds.size.height);
    cairo_t *cr = cairo_create(surface);

    // Background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_paint(cr);

    // Draw circle
    double cx = self.bounds.size.width * 0.5;
    double cy = self.bounds.size.height * 0.5;
    double r = MIN(cx, cy) - 20.0;
    cairo_arc(cr, cx, cy, r, 0, 2*M_PI);

    float red = (float)(arc4random() % 256) / 255.0f;
    float green = (float)(arc4random() % 256) / 255.0f;
    float blue = (float)(arc4random() % 256) / 255.0f;
    cairo_set_source_rgb(cr, red, green, blue);

    cairo_fill(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface); 
}

- (void)randomizeColor {
    self.red = (float)(arc4random() % 256) / 255.0f;
    self.green = (float)(arc4random() % 256) / 255.0f;
    self.blue = (float)(arc4random() % 256) / 255.0f;
}

// Updates the view
- (void)triggerRedraw {
    [self setNeedsDisplay:YES];
}

- (void)setCursor:(NSCursor *)cursor {
    [self addCursorRect:[self bounds] cursor: cursor];
}

@end

#endif