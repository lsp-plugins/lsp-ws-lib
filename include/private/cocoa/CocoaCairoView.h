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

#ifndef PRIVATE_COCOA_COCOACAIROVIEW_H_
#define PRIVATE_COCOA_COCOACAIROVIEW_H_

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_MACOSX

#include <private/cocoa/CocoaCairoSurface.h>

#import <Cocoa/Cocoa.h>

@interface CocoaCairoView : NSView 

    @property (assign) cairo_surface_t *imageSurface;
    @property (strong) NSTimer *redrawTimer;
    @property (strong) NSCursor *nextCursor;
    @property (assign) bool needsRedrawing;

    - (CGImageRef)renderCairoImage;
    - (void)triggerRedraw;
    - (void)setCursor:(NSCursor *)cursor;
    - (void)setImage:(cairo_surface_t *)image;
    - (void)startRedrawLoop;
    - (void)stopRedrawLoop;
@end

#endif

#endif