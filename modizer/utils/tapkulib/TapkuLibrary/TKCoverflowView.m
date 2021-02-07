//
//  TKCoverflowView.m
//  Created by Devin Ross on 1/3/10.
//
/*
 
 tapku.com || http://github.com/devinross/tapkulibrary
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 
*/

//TODO: determine max scrollage area at once and bufferize only when still
//

#define COVER_BUFF_EXTENSION 10  //tradeoff between ram usage & smooth scrolling...

#import "TKCoverflowView.h"
#import "TKCoverflowCoverView.h"
#import "TKGlobal.h"

#define COVER_SPACING 70.0
#define CENTER_COVER_OFFSET 70
#define SIDE_COVER_ANGLE 1.4
#define SIDE_COVER_ZPOSITION -80
#define COVER_SCROLL_PADDING 4


@interface TKCoverflowView (hidden)

- (void) animateToIndex:(int)index  animated:(BOOL)animated;
- (void) load;
- (void) setup;
- (void) setup:(int)position;
- (void) newrange:(BOOL)extended;
- (void) setupTransforms;
- (void) adjustViewHeirarchy;
- (void) deplaceAlbumsFrom:(int)start to:(int)end;
- (void) deplaceAlbumsAtIndex:(int)cnt;
- (BOOL) placeAlbumsFrom:(int)start to:(int)end fast_mode:(BOOL)fmode;
- (void) placeAlbumAtIndex:(int)cnt fast_mode:(BOOL)fmode;
- (void) snapToAlbum:(BOOL)animated;

@end
@implementation TKCoverflowView (hidden)

- (void) setupTransforms{

	leftTransform = CATransform3DMakeRotation(coverAngle, 0, 1, 0);
	leftTransform = CATransform3DConcat(leftTransform,CATransform3DMakeTranslation(-spaceFromCurrent, 0, -300));
	
	rightTransform = CATransform3DMakeRotation(-coverAngle, 0, 1, 0);
	rightTransform = CATransform3DConcat(rightTransform,CATransform3DMakeTranslation(spaceFromCurrent, 0, -300));
	
}
- (void) load{
	self.backgroundColor = [UIColor blackColor];
	numberOfCovers = 0;
	coverSpacing = COVER_SPACING;
	coverAngle = SIDE_COVER_ANGLE;
	self.showsHorizontalScrollIndicator = YES;
    self.showsVerticalScrollIndicator=YES;
	super.delegate = self;
	origin = self.contentOffset.x;
	
	yard = [[NSMutableArray alloc] init];
	views = [[NSMutableArray alloc] init];
	
	coverSize = CGSizeMake(224, 224);
	spaceFromCurrent = coverSize.width/2.4;
	[self setupTransforms];


	CATransform3D sublayerTransform = CATransform3DIdentity;
	sublayerTransform.m34 = -0.001;
	[self.layer setSublayerTransform:sublayerTransform];
	
	currentIndex = -1;
	currentSize = self.frame.size;
    
    self.decelerationRate=0.01f;//UIScrollViewDecelerationRateFast;
	
}
- (void) setup{

	currentIndex = -1;
	for(UIView *v in views) [v removeFromSuperview];
	[yard removeAllObjects];
	[views removeAllObjects];
	[coverViews release];
	coverViews = nil;
	
	if(numberOfCovers < 1){
		self.contentOffset = CGPointZero;
		return;
	} 
	
	
	coverViews = [[NSMutableArray alloc] initWithCapacity:numberOfCovers];
	for (unsigned i = 0; i < numberOfCovers; i++) [coverViews addObject:[NSNull null]];
	deck = NSMakeRange(0, 0);
	
	currentSize = self.frame.size;
	margin = (self.frame.size.width / 2);
	self.contentSize = CGSizeMake( (coverSpacing) * (numberOfCovers-1) + (margin*2) , currentSize.height);
	coverBuffer = (int) ((currentSize.width - coverSize.width) / coverSpacing) + 3;
	

    initRange = YES;
	movingRight = YES;
	currentSize = self.frame.size;
	currentIndex = 0;
	self.contentOffset = CGPointZero;
    

//    self.contentInset=UIEdgeInsetsMake(0,0,0,0);    
//    self.scrollIndicatorInsets=UIEdgeInsetsMake(0, 64, 0, 0); 

//    NSLog(@"setup, newrange");
    
//        NSLog(@"setup nr");
	[self newrange:YES];
//        NSLog(@"setup done");
	[self animateToIndex:currentIndex animated:NO];
	
}

- (void) setup:(int)position{
    
	currentIndex = -1;
	for(UIView *v in views) [v removeFromSuperview];
	[yard removeAllObjects];
	[views removeAllObjects];
	[coverViews release];
	coverViews = nil;
	
	if(numberOfCovers < 1){
		self.contentOffset = CGPointZero;
		return;
	} 
	
	
	coverViews = [[NSMutableArray alloc] initWithCapacity:numberOfCovers];
	for (unsigned i = 0; i < numberOfCovers; i++) [coverViews addObject:[NSNull null]];
	deck = NSMakeRange(0, 0);
	
	currentSize = self.frame.size;
	margin = (self.frame.size.width / 2);
	self.contentSize = CGSizeMake( (coverSpacing) * (numberOfCovers-1) + (margin*2) , currentSize.height);
	coverBuffer = (int) ((currentSize.width - coverSize.width) / coverSpacing) + 3;
	
    
    initRange = YES;
	movingRight = YES;
	currentSize = self.frame.size;
	currentIndex = position;
	self.contentOffset = CGPointMake(coverSpacing*currentIndex, 0);
    
    //        NSLog(@"setup nr");
	[self newrange:YES];
    //        NSLog(@"setup done");
	[self animateToIndex:currentIndex animated:NO];
	
}


- (void) deplaceAlbumsFrom:(int)start to:(int)end{
//    NSLog(@"deplace %d %d %d",start,end,movingRight);
	if(start >= end) return;
	
	for(int cnt=start;cnt<end;cnt++)
		[self deplaceAlbumsAtIndex:cnt];

}
- (void) deplaceAlbumsAtIndex:(int)cnt{    
	if(cnt >= [coverViews count]) return;
	
	if([coverViews objectAtIndex:cnt] != [NSNull null]){
		
		UIView *v = [coverViews objectAtIndex:cnt];
		[v removeFromSuperview];
		[views removeObject:v];
		[yard addObject:v];
		[coverViews replaceObjectAtIndex:cnt withObject:[NSNull null]];
		
	}
}
- (BOOL) placeAlbumsFrom:(int)start to:(int)end  fast_mode:(BOOL)fmode{
//    NSLog(@"place %d %d %d",start,end,movingRight);    
	if(start >= end) return NO;
	for(int cnt=start;cnt<= end;cnt++) 
		[self placeAlbumAtIndex:cnt fast_mode:fmode];
	return YES;
}
- (void) placeAlbumAtIndex:(int)cnt fast_mode:(BOOL)fmode{
	if(cnt >= [coverViews count]) return;
	
	if([coverViews objectAtIndex:cnt] == [NSNull null]){
//		NSLog(@"cv %d",cnt);
		TKCoverflowCoverView *cover = [dataSource coverflowView:self coverAtIndex:cnt];
		[coverViews replaceObjectAtIndex:cnt withObject:cover];
		
		CGRect r = cover.frame;
		r.origin.y = currentSize.height / 2 - (coverSize.height/2) - (coverSize.height/16);
		r.origin.x = (currentSize.width/2 - (coverSize.width/ 2)) + (coverSpacing) * cnt;
		cover.frame = r;
		
		[self addSubview:cover];
		if(cnt > currentIndex){
			cover.layer.transform = rightTransform;
			[self sendSubviewToBack:cover];
		}
		else 
			cover.layer.transform = leftTransform;
		
		[views addObject:cover];
		
	}
}

- (void) newrange:(BOOL)extended{
	
	int loc = deck.location, len = deck.length, buff = coverBuffer;
	int newLocation = currentIndex - buff < 0 ? 0 : currentIndex-buff;
	int newLength = currentIndex + buff > numberOfCovers ? numberOfCovers - newLocation : currentIndex + buff - newLocation;
    buff+=COVER_BUFF_EXTENSION;
    int newLocationE = currentIndex - buff < 0 ? 0 : currentIndex-buff;
	int newLengthE = currentIndex + buff > numberOfCovers ? numberOfCovers - newLocationE : currentIndex + buff - newLocationE;
    
	
//    NSLog(@"nr : %d ci %d / old %d-%d / new %d-%d / %d-%d",extended,currentIndex,loc,len,newLocation,newLength,newLocationE,newLengthE);
    
	if ((loc == newLocation && newLength == len)&&!extended) return;
	
	if(movingRight||initRange){
        initRange=NO;
        if (extended) {
            [self deplaceAlbumsFrom:loc to:MIN(newLocationE,loc+len)];        
            [self placeAlbumsFrom:MAX(loc+len,newLocationE) to:newLocationE+newLengthE fast_mode:extended];
        } else {
            [self deplaceAlbumsFrom:loc to:MIN(newLocationE,loc+len)];        
            [self placeAlbumsFrom:MAX(loc+len,newLocation) to:newLocation+newLength fast_mode:extended];
        }
		
	}else{
        if (extended) {
            [self deplaceAlbumsFrom:MAX(newLengthE+newLocationE,loc) to:loc+len];
            [self placeAlbumsFrom:newLocationE to:MIN(newLocationE+newLengthE,loc) fast_mode:extended];
        } else {
            [self deplaceAlbumsFrom:MAX(newLengthE+newLocationE,loc) to:loc+len];
            [self placeAlbumsFrom:newLocation to:MIN(newLocation+newLength,loc) fast_mode:extended];
        }
	}
	
	NSRange spectrum = NSMakeRange(0, numberOfCovers);
    if (extended) deck = NSIntersectionRange(spectrum, NSMakeRange(newLocationE, newLengthE));
    else deck = NSIntersectionRange(spectrum, NSMakeRange(newLocation, newLength));
}


- (void) adjustViewHeirarchy{

	int i = currentIndex-1;
	if (i >= 0) 
		for(;i > deck.location;i--) 
			[self sendSubviewToBack:[coverViews objectAtIndex:i]];
	
	i = currentIndex+1;
	if(i<numberOfCovers-1) 
		for(;i < deck.location+deck.length;i++) 
			[self sendSubviewToBack:[coverViews objectAtIndex:i]];
	
	UIView *v = [coverViews objectAtIndex:currentIndex];
	if((NSObject*)v != [NSNull null])
		[self bringSubviewToFront:[coverViews objectAtIndex:currentIndex]];
	
	
}

- (void) snapToAlbum:(BOOL)animated{
	
	UIView *v = [coverViews objectAtIndex:currentIndex];
	
	if((NSObject*)v!=[NSNull null]){
		[self setContentOffset:CGPointMake(v.center.x - (currentSize.width/2), 0) animated:animated];
	}else{		
		[self setContentOffset:CGPointMake(coverSpacing*currentIndex, 0) animated:animated];
	}
	
}
- (void) animateToIndex:(int)index animated:(BOOL)animated{
	
	NSString *string = [NSString stringWithFormat:@"%d",currentIndex];
	if(velocity> 200) animated = NO;
	
	if(animated){
		float speed = 0.2;
		if(velocity>80)speed=0.05;
		[UIView beginAnimations:string context:nil];
		[UIView setAnimationDuration:speed];
		[UIView setAnimationCurve:UIViewAnimationCurveLinear];
		[UIView setAnimationBeginsFromCurrentState:YES];
		[UIView setAnimationDelegate:self];
		[UIView setAnimationDidStopSelector:@selector(animationDidStop:finished:context:)]; 
	}

	for(UIView *v in views){
		int i = [coverViews indexOfObject:v];
		if(i < index) v.layer.transform = leftTransform;
		else if(i > index) v.layer.transform = rightTransform;
		else v.layer.transform = CATransform3DIdentity;
	}
	
	if(animated) [UIView commitAnimations];
	else [coverflowDelegate coverflowView:self coverAtIndexWasBroughtToFront:currentIndex];

}
- (void) animationDidStop:(NSString *)animationID finished:(NSNumber *)finished context:(void *)context{

	if([finished boolValue]) [self adjustViewHeirarchy];
	
	if([finished boolValue] && [animationID intValue] == currentIndex) [coverflowDelegate coverflowView:self coverAtIndexWasBroughtToFront:currentIndex];
	
}

@end

@implementation TKCoverflowView
@synthesize coverflowDelegate, dataSource, coverSize, numberOfCovers, coverSpacing, coverAngle;

- (id) initWithFrame:(CGRect)frame {
	if(![super initWithFrame:frame]) return nil;
	[self load];
	currentSize = frame.size;
    return self;
}

- (void) layoutSubviews{
	
	if(self.frame.size.width == currentSize.width && self.frame.size.height == currentSize.height) return;
	currentSize = self.frame.size;

	margin = (self.frame.size.width / 2);
	self.contentSize = CGSizeMake( (coverSpacing) * (numberOfCovers-1) + (margin*2) , self.frame.size.height);
	coverBuffer = (int)((currentSize.width - coverSize.width) / coverSpacing) + 3;
	
	

	for(UIView *v in views){
		v.layer.transform = CATransform3DIdentity;
		CGRect r = v.frame;
		r.origin.y = currentSize.height / 2 - (coverSize.height/2) - (coverSize.height/16);
		v.frame = r;

	}

	for(int i= deck.location; i < deck.location + deck.length; i++){
		
		if([coverViews objectAtIndex:i] != [NSNull null]){
			UIView *cover = [coverViews objectAtIndex:i];
			CGRect r = cover.frame;
			r.origin.x = (currentSize.width/2 - (coverSize.width/ 2)) + (coverSpacing) * i;
			cover.frame = r;
		}
	}
	

//    NSLog(@"layout nr");
	[self newrange:YES];
//    NSLog(@"layout done");    
	[self animateToIndex:currentIndex animated:NO];
	

}

- (void) setNumberOfCovers:(int)cov{
	numberOfCovers = cov;
	[self setup];
}

- (void) setNumberOfCovers:(int)cov pos:(int)_pos{
	numberOfCovers = cov;
	[self setup:_pos];
}


- (void) setCoverSpacing:(float)space{
	coverSpacing = space;
	[self setupTransforms];
	[self setup];
	[self layoutSubviews];
}
- (void) setCoverAngle:(float)f{
	coverAngle = f;
	[self setupTransforms];
	[self setup];
}
- (void) setCoverSize:(CGSize)s{
	coverSize = s;
	spaceFromCurrent = coverSize.width/2.4;
	[self setupTransforms];
	[self setup];
}


- (TKCoverflowCoverView *) coverAtIndex:(int)index{
	if([coverViews objectAtIndex:index] != [NSNull null]) return [coverViews objectAtIndex:index];
	return nil;
}

- (NSInteger) currentIndex{
	return currentIndex;
}
- (NSInteger) indexOfFrontCoverView{
	return currentIndex;

}
- (void) setCurrentIndex:(NSInteger)index{
	[self bringCoverAtIndexToFront:index animated:NO];
}


- (void) bringCoverAtIndexToFront:(int)index animated:(BOOL)animated{
	
	if(index == currentIndex) return;
	
    currentIndex = index;
    [self snapToAlbum:animated];
//        NSLog(@"bringto");
	[self newrange:YES];
//        NSLog(@"bringto done");
	[self animateToIndex:index animated:animated];
}

- (TKCoverflowCoverView*) dequeueReusableCoverView{
	
	if([yard count] < 1)  return nil;
	
	TKCoverflowCoverView *v = [[[yard lastObject] retain] autorelease];
	v.layer.transform = CATransform3DIdentity;
    
	[yard removeLastObject];

	return v;
}


- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches anyObject];
	
	if(touch.view != self &&  [touch locationInView:touch.view].y < coverSize.height){
		currentTouch = touch.view;
	}

}
- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {

	UITouch *touch = [touches anyObject];
	
	if(touch.view == currentTouch){
		if(touch.tapCount > 1 && currentIndex == [coverViews indexOfObject:currentTouch]){

			if([coverflowDelegate respondsToSelector:@selector(coverflowView:coverAtIndexWasDoubleTapped:)])
				[coverflowDelegate coverflowView:self coverAtIndexWasDoubleTapped:currentIndex];
			
		}else if(touch.tapCount == 1 && currentIndex == [coverViews indexOfObject:currentTouch]){
            
			if([coverflowDelegate respondsToSelector:@selector(coverflowView:coverAtIndexWasSingleTapped:)])
				[coverflowDelegate coverflowView:self coverAtIndexWasSingleTapped:currentIndex];
			
		}else{
			int index = [coverViews indexOfObject:currentTouch];
			[self setContentOffset:CGPointMake(coverSpacing*index, 0) animated:YES];
		}
		

	}
	

	
	currentTouch = nil;
}
- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event{
	if(currentTouch!= nil) currentTouch = nil;
}

- (void) scrollViewDidScroll:(UIScrollView *)scrollView{
    
    
	
	velocity = abs(pos - scrollView.contentOffset.x);
	pos = scrollView.contentOffset.x;
	movingRight = self.contentOffset.x - origin > 0 ? YES : NO;
	origin = self.contentOffset.x;

	CGFloat num = numberOfCovers;
	CGFloat per = scrollView.contentOffset.x / (self.contentSize.width - currentSize.width);
	CGFloat ind = num * per;
	CGFloat mi = ind / (numberOfCovers/2);
	mi = 1 - mi;
	mi = mi / 2;
	int index = (int)(ind+mi);
	index = MIN(MAX(0,index),numberOfCovers-1);
	

	if(index == currentIndex) return;
	
	currentIndex = index;
//        NSLog(@"didscroll");
	[self newrange:NO];
//        NSLog(@"didscroll done");
	
	
	if(velocity < 180 || currentIndex < 15 || currentIndex > (numberOfCovers - 16))
		[self animateToIndex:index animated:YES];
	
}
- (void) scrollViewDidEndDecelerating:(UIScrollView *)scrollView{
	if(!scrollView.tracking && !scrollView.decelerating){

		[self snapToAlbum:YES];
		[self adjustViewHeirarchy];

//        NSLog(@"end dec");
        [self newrange:YES];
//        NSLog(@"end dec yo");
	} 
}
- (void) scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate{
	if(!self.decelerating && !decelerate){
		[self snapToAlbum:YES];
		[self adjustViewHeirarchy];
        
//        NSLog(@"end dec");
        [self newrange:YES];
//        NSLog(@"end dec yo");
	}
}

- (void) dealloc {	
	
	[yard release],yard = nil;
	[views release],views = nil;
	[coverViews release],coverViews = nil;

	currentTouch = nil;
	coverflowDelegate = nil;
	dataSource = nil;
	
    [super dealloc];
}

@end