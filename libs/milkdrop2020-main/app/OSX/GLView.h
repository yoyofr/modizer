
#import <Cocoa/Cocoa.h>
#import "EventDelegate.h"

@class GLView;

@protocol GLViewDelegate <NSObject>

@required
- (void)drawInGLView:(GLView * _Nonnull)view;

@end



@interface GLView : NSOpenGLView


@property (weak, nonatomic) IBOutlet id<GLViewDelegate> _Nullable delegate;
@property (weak, nonatomic) IBOutlet id<EventDelegate> _Nullable eventDelegate;


- (void)registerDragDrop;

@end
