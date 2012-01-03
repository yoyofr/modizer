#include "OGLView.h"
#include "GlErrors.h"
#include <QuartzCore/QuartzCore.h>

#include <OpenGLES/EAGL.h>
#include <OpenGLES/EAGLDrawable.h>
#include <OpenGLES/ES1/glext.h>

@implementation OGLView

+ (Class)layerClass 
{
    return [CAEAGLLayer class];
}

- (id)initWithCoder:(NSCoder*)coder 
{
    if ((self = [super initWithCoder:coder])) 
	{
        CAEAGLLayer* eaglLayer = (CAEAGLLayer *)self.layer;
        eaglLayer.opaque = YES;
        //self.opaque=NO;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
											[NSNumber numberWithBool:NO], 
											kEAGLDrawablePropertyRetainedBacking, 
											kEAGLColorFormatRGBA8, 
											kEAGLDrawablePropertyColorFormat, 
											nil];  
		
		
    }
	m_touchcount=0;
	m_poptrigger=FALSE;
    return self;
}

- (void)initialize:(EAGLContext*)oglContext scaleFactor:(float)scaleFactor {
	if ([self respondsToSelector:@selector(contentScaleFactor)]) {
		self.contentScaleFactor=scaleFactor;
	}
	m_oglContext=oglContext;
	FrameBufferUtils::Create(m_frameBuffer, oglContext, (CAEAGLLayer*)self.layer);
}

- (void)layoutSubviews {
    //[EAGLContext setCurrentContext:m_oglContext];
    FrameBufferUtils::Destroy(m_frameBuffer);
	FrameBufferUtils::Create(m_frameBuffer, m_oglContext, (CAEAGLLayer*)self.layer);
}

- (void)dealloc 
{

	FrameBufferUtils::Destroy(m_frameBuffer);
    [super dealloc];
}


- (void)bind
{
	FrameBufferUtils::Set(m_frameBuffer);
}


- (bool)popTouch
{
	const bool touched = m_touched;
	m_touched = false;
	m_touchcount=0;
	m_poptrigger=TRUE;
	return touched;
}


- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch;
	m_1clicked = FALSE;
    m_hasmoved = FALSE;
	m_touched = true;
	m_poptrigger = FALSE;
	m_touchcount+=[touches count];
    touch = [touches anyObject];
    currentTouchLocation=[touch locationInView:touch.view];
	if([touches count] == 1) {
		touch = [touches anyObject];
		previousTouchLocation = [touch locationInView:touch.view];
		currentMove.x=currentMove.y=0.0f;
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	if ((m_hasmoved==FALSE)&&(m_touchcount==1)) m_1clicked=TRUE;
	m_touchcount-=[touches count];
}


- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch = [touches anyObject];
    currentTouchLocation=[touch locationInView:touch.view];
	
	//m_touchcount=[touches count];
	if([touches count] == 1) {
		UITouch *touch = [touches anyObject];
		CGPoint location = [touch locationInView:touch.view];
		currentMove.x=location.x-previousTouchLocation.x;
		currentMove.y=location.y-previousTouchLocation.y;
        m_hasmoved=TRUE;
	}
	
}


@end
