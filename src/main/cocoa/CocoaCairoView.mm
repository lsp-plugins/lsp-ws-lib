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

// We need an overloaded objective c - NSView Class for Rendering, drawRect is triggered on create/update
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSLog(@"Content view: %@", self);

    if (self->_nextCursor != NULL) {
        //[self discardCursorRects];
        [self addCursorRect:[self bounds] cursor: self->_nextCursor];
    }

    self->_needsRedrawing = false;
    if (self->_imageSurface != NULL) {
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

        cairo_surface_destroy(self->_imageSurface);
        self->_imageSurface = NULL;
    } 
    
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center postNotificationName:@"RedrawRequest"
            object:self];
    
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        lsp_trace("Register event for view: %p", self);
        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
        [center addObserverForName:@"ForceExpose"
                object:self
                queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification * _Nonnull note) {
                    if ([note.userInfo[@"Surface"] pointerValue] != nil) {
                        lsp_trace("Update Surface!");
                        if (self->_imageSurface) {
                            cairo_surface_destroy(self->_imageSurface);
                        }
                        self->_imageSurface = (cairo_surface_t *)[note.userInfo[@"Surface"] pointerValue];
                        self->_needsRedrawing = true;
                    }
                }
        ];
    }
    return self;
} 

- (CGImageRef)renderCairoImage {
    cairo_surface_flush(self->_imageSurface);

    unsigned char *data = cairo_image_surface_get_data(self->_imageSurface);
    int width = cairo_image_surface_get_width(self->_imageSurface);
    int height = cairo_image_surface_get_height(self->_imageSurface);
    int stride = cairo_image_surface_get_stride(self->_imageSurface);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst;

    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, data, stride * height, NULL);

    CGImageRef image = CGImageCreate(width,  height, 8, 32, stride,
        colorSpace, bitmapInfo, provider, NULL, false, kCGRenderingIntentDefault);

    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    return image;
}

// Starts the redraw loop
- (void)startRedrawLoop {
    lsp_trace("Trigger timer start!");

    if (self->_redrawTimer == nil) {
        self->_redrawTimer = [NSTimer   scheduledTimerWithTimeInterval:(1.0/60.0)
                                        target:self
                                        selector:@selector(triggerRedraw)
                                        userInfo:nil
                                        repeats:YES];
    }
}

// Stops the redraw loop
- (void)stopRedrawLoop {
    if (self->_redrawTimer != nil) {
        [self->_redrawTimer invalidate]; 
        self->_redrawTimer = nil;
    }

}

// Destructor
- (void)dealloc {
    [self stopRedrawLoop];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    if (self.trackingArea) {
        [self removeTrackingArea:self.trackingArea];
        self.trackingArea = nil;
    }
    self.display = nullptr;
    [super dealloc];
}

// Updates the view
- (void)triggerRedraw {
    if (self->_needsRedrawing)
        [self setNeedsDisplay:YES];
}

// Sets the cairo image
- (void)setImage:(cairo_surface_t *)image {
    self->_imageSurface = image;
}

// Sets the cursor in window
- (void)setCursor:(NSCursor *)cursor {
    self->_nextCursor = cursor;
}

//TODO: Finish drag and drop, only a draft
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center postNotificationName:@"DragEnter"
            object:[self window]];

    return NSDragOperationCopy; 
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center postNotificationName:@"DragExit"
            object:[self window]];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    NSPasteboard *pboard = [sender draggingPasteboard];

    if ([[pboard types] containsObject:NSPasteboardTypeFileURL]) {
        NSArray<NSURL *> *files = [pboard   readObjectsForClasses:@[[NSURL class]]
                                            options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
        for (NSURL *url in files) {
            NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
            [center postNotificationName:@"DragDroped"
                    object:[self window]
                    userInfo:@{
                        @"URL": url.path
                    }
            ];
        }
        return YES;
    }
    return NO;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (self.trackingArea) {
        [self removeTrackingArea:self.trackingArea];
    }
    NSTrackingArea *area = [[NSTrackingArea alloc]  initWithRect:self.bounds
                                                    options:
                                                        NSTrackingMouseEnteredAndExited |
                                                        NSTrackingActiveInActiveApp |
                                                        NSTrackingInVisibleRect |
                                                        NSTrackingMouseMoved |
                                                        NSTrackingEnabledDuringMouseDrag
                                                    owner:self
                                                    userInfo:nil];
    [self addTrackingArea:area];
    self.trackingArea = area;
}

- (void)mouseDown:(NSEvent *)event {
    lsp_trace("Mouse down event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)rightMouseDown:(NSEvent *)event {
    lsp_trace("Mouse (right) down event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)otherMouseDown:(NSEvent *)event {
    lsp_trace("Mouse (other) down event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)mouseUp:(NSEvent *)event {
    lsp_trace("Mouse up event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)rightMouseUp:(NSEvent *)event {
    lsp_trace("Mouse (right) up event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)otherMouseUp:(NSEvent *)event {
    lsp_trace("Mouse (other) up event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)mouseMoved:(NSEvent *)event {
    lsp_trace("Mouse moved event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)mouseDragged:(NSEvent *)event {
    lsp_trace("Mouse dragged event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)mouseEntered:(NSEvent *)event {
    lsp_trace("Mouse enterd event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)mouseExited:(NSEvent *)event {
    lsp_trace("Mouse exited event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)scrollWheel:(NSEvent *)event {
    lsp_trace("Mouse scroll event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)keyDown:(NSEvent *)event {
    lsp_trace("Key down event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

- (void)keyUp:(NSEvent *)event {
    lsp_trace("Key up event in CocoaCairoView: %p", self);
    if (self.display)
        self.display->handle_event(event);
}

@end

#endif