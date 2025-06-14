#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

@interface CocoaCairoView : NSView

    @property (nonatomic, assign) float red;
    @property (nonatomic, assign) float green;
    @property (nonatomic, assign) float blue;

    - (void)triggerRedraw;
    - (void)drawCricle;
    - (void)drawTriangle;
    - (void)randomizeColor;
@end

#endif