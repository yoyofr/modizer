#ifndef st_oglview_h_
#define st_oglview_h_


#include "FrameBuffer.h"
#import <UIKit/UIKit.h>
#import <UIKit/UIView.h>

@class EAGLContext;

@interface OGLView : UIView 
{
	FrameBuffer m_frameBuffer;
	EAGLContext* m_oglContext;

	@public int m_touchcount;
	@public bool m_touched,m_hasmoved;
	@public bool m_poptrigger;
	@public CGPoint previousTouchLocation;
    @public CGPoint currentTouchLocation;
	@public CGPoint currentMove;
@public bool m_1clicked;
}

- (void)initialize:(EAGLContext*)oglContext scaleFactor:(float)scaleFactor;
- (void)bind;
- (bool)popTouch;


@end


#endif
