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
#include <private/cocoa/CocoaDisplay.h>
#include <lsp-plug.in/ws/ISurface.h>
#include <lsp-plug.in/common/debug.h>

#include <cairo.h>
#include <cairo-quartz.h>

@implementation CocoaCairoView

cairo_surface_t *imageSurface;

// We need an overloaded objective c - NSView Class for Rendering, drawRect is triggered on create/update
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    if (imageSurface != NULL) {
        // Get current CGContext
        CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
        CGImageRef image = [self renderCairoImage];

        CGContextSaveGState(context);
        // Flip vertically to match Cairo's coordinate system
        CGContextTranslateCTM(context, 0, self.bounds.size.height);
        CGContextScaleCTM(context, 1, -1);

        CGRect rect = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
        CGContextDrawImage(context, rect, image);

        CGContextRestoreGState(context);

        CGImageRelease(image);
    } 

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center postNotificationName:@"RedrawRequest"
                        object:[self window]];
    
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
        [center addObserverForName:@"ForceExpose"
                        object:[self window]
                        queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification * _Nonnull note) {
                            imageSurface = (cairo_surface_t *)[note.userInfo[@"Surface"] pointerValue];
                        }
        ];
    }
    return self;
} 

- (CGImageRef)renderCairoImage {
    cairo_surface_flush(imageSurface);

    unsigned char *data = cairo_image_surface_get_data(imageSurface);
    int width = cairo_image_surface_get_width(imageSurface);
    int height = cairo_image_surface_get_height(imageSurface);
    int stride = cairo_image_surface_get_stride(imageSurface);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst;

    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, data, stride * height, NULL);

    CGImageRef image = CGImageCreate(width,  height, 8, 32, stride,
        colorSpace, bitmapInfo, provider, NULL, false, kCGRenderingIntentDefault);

    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    return image;
}

- (void)startRedrawLoop {
    [NSTimer scheduledTimerWithTimeInterval:(1.0/60.0)
            target:self
            selector:@selector(triggerRedraw)
            userInfo:nil
            repeats:YES];
}

// Updates the view
- (void)triggerRedraw {
    [self setNeedsDisplay:YES];
}

// Sets the cairo image
- (void)setImage:(cairo_surface_t *)image {
    imageSurface = image;
}

// Sets the cursor in window
- (void)setCursor:(NSCursor *)cursor {
    [self addCursorRect:[self bounds] cursor: cursor];
}

@end

#endif