#import "GLView.h"
#import <AppKit/AppKit.h>
#import <GLKit/GLKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <Carbon/Carbon.h>
#import <CoreVideo/CVDisplayLink.h>



@implementation GLView
{
    CVDisplayLinkRef _displayLink;
}


-(BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)keyDown:(NSEvent *)event
{
    [self.eventDelegate keyDown:event];
}

- (void)keyUp:(NSEvent *)event
{
    [self.eventDelegate keyUp:event];
}



- (void)prepareOpenGL
{
    [super prepareOpenGL];
    
    [self setupView];
}


-(void)setupView
{
    //[self setWantsBestResolutionOpenGLSurface:YES];
    [self.openGLContext makeCurrentContext];
    
    // Synchronize buffer swaps with vertical refresh rate
    GLint swapInt = 0;
    [self.openGLContext setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];
    
    [self startDisplayLink];
}

- (void)dealloc
{
    // Release the display link
    CVDisplayLinkStop(_displayLink);
    CVDisplayLinkRelease(_displayLink);
}



// This is the renderer output callback function
static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
	GLView *view =  (__bridge GLView *)displayLinkContext;
    
    [view performSelectorOnMainThread:@selector(onDisplayFrame) withObject:view waitUntilDone:FALSE];
    
	//[view setNeedsDisplay:YES];
	return kCVReturnSuccess;
}


-(void)onDisplayFrame
{
    {
        [self setNeedsDisplay:YES];
    }
}



-(void)startDisplayLink
{
	// Create a display link capable of being used with all active displays
	CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
	
	// Set the renderer output callback function
	CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, (__bridge void *)self);
	
	// Set the display link for the current renderer
	CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
    
//    CGLPixelFormatObj cglPixelFormat = CGLGetPixelFormat(cglContext);
    CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj)self.pixelFormat.CGLPixelFormatObj;
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(_displayLink, cglContext, cglPixelFormat);
	
	// Activate the display link
	CVDisplayLinkStart(_displayLink);
    
    
    // Register to be notified when the window closes so we can stop the displaylink
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(windowWillClose:)
                                                 name:NSWindowWillCloseNotification
                                               object:[self window]];
    

}



- (void) windowWillClose:(NSNotification*)notification
{
    CVDisplayLinkStop(_displayLink);
    
}

- (void)drawRect:(NSRect)dirtyRect
{
    [self drawView];
}
    
-(void) drawView
{
    @autoreleasepool {
    
//    CGLLockContext([[self openGLContext] CGLContextObj]);
    
    [self.openGLContext makeCurrentContext];
    
    [self.delegate drawInGLView:self];

 
	[self.openGLContext flushBuffer];
       
//    CGLFlushDrawable([[self openGLContext] CGLContextObj]);
//    CGLUnlockContext([[self openGLContext] CGLContextObj]);
          
    }
}


- (void)registerDragDrop
{
    [self registerForDraggedTypes:[NSArray arrayWithObjects:
                                   NSFilenamesPboardType, nil]];
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];
    NSPasteboard *pboard = [sender draggingPasteboard];

    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        if (sourceDragMask & NSDragOperationLink) {
            return NSDragOperationLink;
        } else if (sourceDragMask & NSDragOperationCopy) {
            return NSDragOperationCopy;
        }
    }
    return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard = [sender draggingPasteboard];

    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
        [self.eventDelegate onDragDrop:files];
    }
    return YES;
}



@end
