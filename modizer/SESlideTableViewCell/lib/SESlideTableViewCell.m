//
//  SESlideTableViewCell.m
//  slidetableviewcell
//
//  Created by letdown on 2014/07/09.
//  Copyright (c) 2014 Space Elephant. All rights reserved.
//

#import "SESlideTableViewCell.h"

#pragma mark -

#define SE_DEFAULT_BUTTON_WIDTH	((CGFloat)90.0)
#define SE_BUTTON_MARGIN	((CGFloat)20.0)
#define SE_ANIMATION_DURATION	0.4
#define SE_ANIMATION_DUMPING	0.75
#define SE_SECTION_INDEX_WIDTH 15

static UIImage* SECreateImageWithColor(UIColor* color, CGSize size) {
	CGRect rect = CGRectMake(0, 0, size.width, size.height);
	UIGraphicsBeginImageContext(rect.size);
	CGContextRef context = UIGraphicsGetCurrentContext();
	CGContextSetFillColorWithColor(context, color.CGColor);
	CGContextFillRect(context, rect);
	UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return image;
}

#pragma mark - SESlideView

@interface SESlideView : UIView

@property (nonatomic) UIColor* color;

@end

@interface SESlideView () {
	UIImageView* m_imageView;
}

@end

@implementation SESlideView

@synthesize color = m_color;

- (instancetype)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		m_color = [UIColor whiteColor];
		
		self.clipsToBounds = YES;
		UIImage* image = SECreateImageWithColor(m_color, CGSizeMake(1, 1));
		UIImageView* imageView = [[UIImageView alloc] initWithImage:image];
		[imageView setTranslatesAutoresizingMaskIntoConstraints:NO];
		imageView.frame = CGRectMake(0, 0, frame.size.width, frame.size.height);
		[self addSubview:imageView];
		m_imageView = imageView;
		
		NSArray* constraints = @[
			// H:|[imageView(==self)]
			[NSLayoutConstraint constraintWithItem:imageView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeLeft multiplier:1 constant:0],
			[NSLayoutConstraint constraintWithItem:imageView attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeWidth multiplier:1 constant:0],
			// V:|[imageView(==self)]
			[NSLayoutConstraint constraintWithItem:imageView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeTop multiplier:1 constant:0],
			[NSLayoutConstraint constraintWithItem:imageView attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeHeight multiplier:1 constant:0],
			];
		[self addConstraints:constraints];
	}
	return self;
}

- (void)setColor:(UIColor *)color {
	m_color = color;
	UIImage* image = SECreateImageWithColor(m_color, CGSizeMake(1, 1));
	m_imageView.image = image;
}

@end

#pragma mark - SEButtonView

@interface SEButtonView : UIView {
	UIView* m_colorView;
	UIView* m_buttonView;
	SESlideTableViewCellSide m_side;
}

- (instancetype)initWithFrame:(CGRect)frame color:(UIColor*)color;

@property (nonatomic) CGFloat displayWidth;
@property (nonatomic) NSInteger buttonIndex;

@end

@implementation SEButtonView

@synthesize displayWidth = m_displayWidth;
@synthesize buttonIndex = m_buttonIndex;

- (instancetype)initWithFrame:(CGRect)frame color:(UIColor*)color {
	self = [super initWithFrame:frame];
	if (self) {
		m_side = SESlideTableViewCellSideLeft;
		m_displayWidth = SE_DEFAULT_BUTTON_WIDTH;
		
		UIImage* image = SECreateImageWithColor(color, CGSizeMake(1, 1));
		m_colorView = [[UIImageView alloc] initWithImage:image];
		[self addSubview:m_colorView];
	}
	return self;
}

- (void)layoutSubviews {
	[super layoutSubviews];
	
	CGSize size = self.frame.size;
	m_colorView.frame = CGRectMake(0, 0, size.width, size.height);
	
	if (m_buttonView) {
		switch (m_side) {
			case SESlideTableViewCellSideLeft:
				m_buttonView.frame = CGRectMake(size.width - m_displayWidth, 0, m_displayWidth, size.height);
				break;
			case SESlideTableViewCellSideRight:
				m_buttonView.frame = CGRectMake(0, 0, m_displayWidth, size.height);
				break;
			default:
				break;
		}
	}
}

- (void)setButtonView:(UIView*)view side:(SESlideTableViewCellSide)side {
	m_buttonView = view;
	m_side = side;
	[self addSubview:m_buttonView];
	[self setNeedsLayout];
}

- (void)setDisplayWidth:(CGFloat)displayWidth {
	m_displayWidth = displayWidth;
	[self setNeedsLayout];
}

@end

#pragma mark - SEButtonGroupView

@interface SEButtonGroupView : UIView {
	NSMutableArray* m_buttonViews;
	NSMutableArray* m_constraints;
	SESlideTableViewCellSide m_side;
}

@property (nonatomic) NSMutableArray* buttonViews;
@property (nonatomic, readonly) CGFloat displayWidth;
@property (nonatomic) CGFloat sectionIndexWidth;

@end

@implementation SEButtonGroupView

@synthesize buttonViews = m_buttonViews;
@synthesize displayWidth = m_displayWidth;
@synthesize sectionIndexWidth = m_sectionIndexWidth;

- (instancetype)initWithFrame:(CGRect)frame side:(SESlideTableViewCellSide)side {
	self = [super initWithFrame:frame];
	if (self) {
		m_displayWidth = 0;
		m_buttonViews = [NSMutableArray array];
		m_constraints = [NSMutableArray array];
		m_side = side;
	}
	return self;
}

- (void)layoutSubviews {
	[super layoutSubviews];
	
	CGSize size = self.frame.size;
	if (m_displayWidth > 1.0e-5) {
		switch (m_side) {
			case SESlideTableViewCellSideLeft: {
				CGFloat scale = size.width / m_displayWidth;
				CGFloat originX = 0;
				NSInteger count = m_buttonViews.count;
				for (NSInteger i = 0; i < count; i++) {
					SEButtonView* buttonView = m_buttonViews[i];
					CGFloat width = buttonView.displayWidth;
					buttonView.frame = CGRectMake(0, 0, (originX + width) * scale, size.height);
					originX += width;
				}
			}	break;
			case SESlideTableViewCellSideRight: {
				CGFloat scale = size.width / (m_displayWidth + m_sectionIndexWidth);
				CGFloat originX = 0;
				NSInteger count = m_buttonViews.count;
				for (NSInteger i = 0; i < count; i++) {
					SEButtonView* buttonView = m_buttonViews[i];
					CGFloat width = buttonView.displayWidth;
					buttonView.frame = CGRectMake(originX * scale, 0, size.width - originX * scale, size.height);
					originX += width;
				}
			}	break;
			default:
				break;
		}
	}
}

- (NSInteger)buttonCount {
	return m_buttonViews.count;
}

- (void)addButtonView:(SEButtonView*)view {
	if (m_side == SESlideTableViewCellSideLeft) {
		if (m_buttonViews.count > 0) {
			[self insertSubview:view belowSubview:[m_buttonViews lastObject]];
		} else {
			[self addSubview:view];
		}
	} else {
		[self addSubview:view];
	}
	[m_buttonViews addObject:view];
	m_displayWidth += view.displayWidth;
}

- (void)clearButtonViews {
	[self removeConstraints:m_constraints];
	[m_constraints removeAllObjects];
	for (SEButtonView* buttonView in m_buttonViews) {
		[buttonView removeFromSuperview];
	}
	[m_buttonViews removeAllObjects];
	m_displayWidth = 0;
}

@end

#pragma mark - SESlideIndicator

#define INDICATOR_LINE_WIDTH ((CGFloat)2.0)
#define INDICATOR_MERGIN ((CGFloat)2.0)
#define INDICATOR_WIDTH (INDICATOR_MERGIN * 3 + INDICATOR_LINE_WIDTH * 4)
#define INDICATOR_LINE_HEIGHT0 ((CGFloat)3.0)
#define INDICATOR_LINE_HEIGHT1 ((CGFloat)7.0)
#define INDICATOR_HEIGHT (INDICATOR_LINE_HEIGHT1)
#define INDICATOR_OUT_MERGIN ((CGFloat)4.0)

typedef NS_OPTIONS(NSUInteger, SESlideIndicatorSideOption) {
	SESlideIndicatorSideOptionNone	= 0,
	SESlideIndicatorSideOptionLeft	= (1 << 0),
	SESlideIndicatorSideOptionRight	= (1 << 1),
	SESlideIndicatorSideOptionLeftAndRight = (SESlideIndicatorSideOptionLeft|SESlideIndicatorSideOptionRight)
};

@interface SESlideIndicator : UIView

@property (nonatomic) SESlideIndicatorSideOption sideOption;
@property (nonatomic) UIColor* color;

@end

@implementation SESlideIndicator

@synthesize sideOption = m_sideOption;
@synthesize color = m_color;

- (instancetype)init {
	self = [super initWithFrame:CGRectMake(0, 0, INDICATOR_WIDTH, INDICATOR_HEIGHT)];
	if (self) {
		self.backgroundColor = [UIColor clearColor];
		m_color = [UIColor colorWithWhite:210/255.0f alpha:1.0f];
	}
	return self;
}

- (void)addLinesAtCGContext:(CGContextRef)context line0X:(CGFloat)line0X line1X:(CGFloat)line1X {
	CGPathRef line0 = CGPathCreateWithRoundedRect(CGRectMake(line0X, (INDICATOR_HEIGHT - INDICATOR_LINE_HEIGHT0) * 0.5f, INDICATOR_LINE_WIDTH, INDICATOR_LINE_HEIGHT0), INDICATOR_LINE_WIDTH * 0.5f, INDICATOR_LINE_WIDTH * 0.5f, NULL);
	
	CGPathRef line1 = CGPathCreateWithRoundedRect(CGRectMake(line1X, (INDICATOR_HEIGHT - INDICATOR_LINE_HEIGHT1) * 0.5f, INDICATOR_LINE_WIDTH, INDICATOR_LINE_HEIGHT1), INDICATOR_LINE_WIDTH * 0.5f, INDICATOR_LINE_WIDTH * 0.5f, NULL);
	
	
	CGContextAddPath(context, line0);
	CGContextAddPath(context, line1);

	CGPathRelease(line0);
	CGPathRelease(line1);
}

- (void)drawRect:(CGRect)rect {
	CGContextRef context = UIGraphicsGetCurrentContext();
	
	CGContextSetFillColorWithColor(context, m_color.CGColor);
	
	switch (m_sideOption) {
		case SESlideIndicatorSideOptionNone:
			break;
		case SESlideIndicatorSideOptionLeft:
			[self addLinesAtCGContext:context line0X:INDICATOR_MERGIN * 3 + INDICATOR_LINE_WIDTH * 3 line1X:INDICATOR_MERGIN * 2 + INDICATOR_LINE_WIDTH * 2];
			CGContextFillPath(context);
			break;
		case SESlideIndicatorSideOptionRight:
			[self addLinesAtCGContext:context line0X:INDICATOR_MERGIN * 2 + INDICATOR_LINE_WIDTH * 2 line1X:INDICATOR_MERGIN * 3 + INDICATOR_LINE_WIDTH * 3];
			CGContextFillPath(context);
			break;
		case SESlideIndicatorSideOptionLeftAndRight:
			[self addLinesAtCGContext:context line0X:0 line1X:INDICATOR_MERGIN + INDICATOR_LINE_WIDTH];
			[self addLinesAtCGContext:context line0X:INDICATOR_MERGIN * 3 + INDICATOR_LINE_WIDTH * 3 line1X:INDICATOR_MERGIN * 2 + INDICATOR_LINE_WIDTH * 2];
			CGContextFillPath(context);
			break;
	}
}

- (void)setSideOption:(SESlideIndicatorSideOption)sideOption {
	if (m_sideOption == sideOption) {
		return;
	}
	m_sideOption = sideOption;
	[self setNeedsDisplay];
}

- (void)setColor:(UIColor *)color {
	m_color = color;
	[self setNeedsDisplay];
}

@end

#pragma mark - SESlideTableViewCell

typedef NS_OPTIONS(NSUInteger, SESlideStateOptions) {
	SESlideStateOptionNone	= 0,
	SESlideStateOptionLeft	= 1 << 0,
	SESlideStateOptionRight	= 1 << 1,
};

@interface SESlideTableViewCell () <UIGestureRecognizerDelegate> {
	SESlideStateOptions m_preparedSlideStates;
	UIPanGestureRecognizer* m_panGestureRecognizer;
	CGFloat m_panStartSnapshotViewOriginX;
	
	UIView* m_snapshotView;
	UIView* m_snapshotContainerView;
	SESlideView* m_slideView;
	SEButtonGroupView* m_leftButtonGroupView;
	SEButtonGroupView* m_rightButtonGroupView;
	NSMutableArray* m_constraints;
	
	SESlideIndicator* m_indicator;
	NSLayoutConstraint* m_indicatorRightConstraint;
	
	uint32_t m_slideAnimationId;
}

@end

@implementation SESlideTableViewCell

@synthesize delegate = m_delegate;
@synthesize slideState = m_slideState;
@synthesize showsLeftSlideIndicator = m_showsLeftSlideIndicator;
@synthesize showsRightSlideIndicator = m_showsRightSlideIndicator;
@synthesize indicatorColor = m_indicatorColor;
@synthesize slideElasticity = m_slideElasticity;

- (instancetype)initWithCoder:(NSCoder *)aDecoder{
    if (self = [super initWithCoder:aDecoder]) {
        [self setUp];
    }
    return self;
}

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
		[self setUp];
	}
    return self;
}

- (void)setUp {
	m_showsLeftSlideIndicator = YES;
	m_showsRightSlideIndicator = YES;
	m_indicatorColor = [UIColor colorWithWhite:210/255.0f alpha:1.0f];
	m_slideElasticity = SESlideTableViewCellSlideElasticitySmooth;
	m_constraints = [NSMutableArray array];
	
	m_panGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanGesture:)];
	m_panGestureRecognizer.delegate = self;
	[self addGestureRecognizer:m_panGestureRecognizer];
	
	m_slideView = [[SESlideView alloc] initWithFrame:self.frame];
	m_leftButtonGroupView = [[SEButtonGroupView alloc] initWithFrame:self.frame side:SESlideTableViewCellSideLeft];
	[m_leftButtonGroupView setTranslatesAutoresizingMaskIntoConstraints:NO];
	m_rightButtonGroupView = [[SEButtonGroupView alloc] initWithFrame:self.frame side:SESlideTableViewCellSideRight];
	[m_rightButtonGroupView setTranslatesAutoresizingMaskIntoConstraints:NO];
}

- (void)layoutSubviews {
	[super layoutSubviews];
	
	CGSize size = self.frame.size;
	m_slideView.frame = CGRectMake(0, 0, size.width, size.height);
	
	if (m_snapshotContainerView) {
		CGRect frame = m_snapshotContainerView.frame;
		frame.size = size;
		m_snapshotContainerView.frame = frame;
	}
}

- (void)prepareForReuse {
	[super prepareForReuse];
	
	m_slideState = SESlideTableViewCellSlideStateCenter;
	m_preparedSlideStates = SESlideStateOptionNone;
	[self cleanUpSlideView];
	
	if (m_indicatorRightConstraint) {
		m_indicatorRightConstraint.constant = -INDICATOR_OUT_MERGIN - self.sectionIndexWidth;
	}
}

#pragma mark - Public Properties

- (void)setShowsLeftSlideIndicator:(BOOL)showsLeftSlideIndicator {
	if (m_showsLeftSlideIndicator == showsLeftSlideIndicator) {
		return;
	}
	[self willChangeValueForKey:@"showsLeftSlideIndicator"];
	m_showsLeftSlideIndicator = showsLeftSlideIndicator;
	[self updateSlideIndicatorVisibility];
	[self didChangeValueForKey:@"showsLeftSlideIndicator"];
}

- (void)setShowsRightSlideIndicator:(BOOL)showsRightSlideIndicator {
	if (m_showsRightSlideIndicator == showsRightSlideIndicator) {
		return;
	}
	[self willChangeValueForKey:@"showsRightSlideIndicator"];
	m_showsRightSlideIndicator = showsRightSlideIndicator;
	[self updateSlideIndicatorVisibility];
	[self didChangeValueForKey:@"showsRightSlideIndicator"];
}

- (void)setIndicatorColor:(UIColor *)indicatorColor {
	m_indicatorColor = indicatorColor;
	if (m_indicator) {
		m_indicator.color = indicatorColor;
	}
}

- (void)setSlideBackgroundColor:(UIColor *)slideBackgroundColor {
	m_slideView.color = slideBackgroundColor;
}

- (UIColor*)slideBackgroundColor {
	return m_slideView.color;
}

#pragma mark - Public Interface

- (void)addButton:(UIView*)button buttonWidth:(CGFloat)buttonWidth backgroundColor:(UIColor*)backgroundColor side:(SESlideTableViewCellSide)side {
	SEButtonView* buttonView = [[SEButtonView alloc] initWithFrame:self.frame color:backgroundColor];
	buttonView.displayWidth = buttonWidth;
	[buttonView setButtonView:button side:side];
	switch (side) {
		case SESlideTableViewCellSideLeft:
			buttonView.buttonIndex = [m_leftButtonGroupView buttonCount];
			[buttonView addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(leftButtonTapped:)]];
			[m_leftButtonGroupView addButtonView:buttonView];
			break;
		case SESlideTableViewCellSideRight:
			buttonView.buttonIndex = [m_rightButtonGroupView buttonCount];
			[buttonView addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(rightButtonTapped:)]];
			[m_rightButtonGroupView addButtonView:buttonView];
			break;
	}
	
	// indicator
	if (m_indicator == nil) {
		SESlideIndicator* indicator = [SESlideIndicator new];
		indicator.color = m_indicatorColor;
		[indicator setTranslatesAutoresizingMaskIntoConstraints:NO];
		[self addSubview:indicator];
		NSLayoutConstraint* widthConstraint = [NSLayoutConstraint constraintWithItem:indicator attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1 constant:INDICATOR_WIDTH];
		NSLayoutConstraint* heightConstraint = [NSLayoutConstraint constraintWithItem:indicator attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1 constant:INDICATOR_HEIGHT];
		NSLayoutConstraint* bottomConstraint = [NSLayoutConstraint constraintWithItem:indicator attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeBottom multiplier:1 constant:-INDICATOR_OUT_MERGIN];
		NSLayoutConstraint* rightConstraint = [NSLayoutConstraint constraintWithItem:indicator attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeRight multiplier:1 constant:-INDICATOR_OUT_MERGIN - self.sectionIndexWidth];
		
		[self addConstraint:widthConstraint];
		[self addConstraint:heightConstraint];
		[self addConstraint:bottomConstraint];
		[self addConstraint:rightConstraint];
		m_indicator = indicator;
		m_indicatorRightConstraint = rightConstraint;
	}
	[self updateSlideIndicatorVisibility];
}

- (void)removeAllButtonsWithSide:(SESlideTableViewCellSide)side {
	switch (side) {
		case SESlideTableViewCellSideLeft:
			[m_leftButtonGroupView clearButtonViews];
			if (m_leftButtonGroupView.superview) {
				[m_leftButtonGroupView removeFromSuperview];
				if (m_preparedSlideStates) {
					m_slideState = SESlideTableViewCellSlideStateCenter;
					m_preparedSlideStates = SESlideStateOptionNone;
					[self cleanUpSlideView];
				}
			}
			break;
		case SESlideTableViewCellSideRight:
			[m_rightButtonGroupView clearButtonViews];
			if (m_rightButtonGroupView.superview) {
				[m_rightButtonGroupView removeFromSuperview];
				if (m_preparedSlideStates) {
					m_slideState = SESlideTableViewCellSlideStateCenter;
					m_preparedSlideStates = SESlideStateOptionNone;
					[self cleanUpSlideView];
				}
			}
			break;
	}
}

- (void)setSlideState:(SESlideTableViewCellSlideState)slideState animated:(BOOL)animated {
	if (! [self canSlideToState:slideState]) {
		return;
	}
	if (animated) {
		switch (slideState) {
			case SESlideTableViewCellSlideStateCenter:
				if (m_preparedSlideStates) {
					[self animateToSlideState:SESlideTableViewCellSlideStateCenter velocity:0];
				}
				break;
			case SESlideTableViewCellSlideStateLeft:
				[self prepareToSlideLeft];
				[self animateToSlideState:SESlideTableViewCellSlideStateLeft velocity:0];
				break;
			case SESlideTableViewCellSlideStateRight:
				[self prepareToSlideRight];
				[self animateToSlideState:SESlideTableViewCellSlideStateRight velocity:0];
				break;
		}
	} else {
		m_slideState = slideState;
		switch (slideState) {
			case SESlideTableViewCellSlideStateCenter:
				m_preparedSlideStates = SESlideStateOptionNone;
				[self cleanUpSlideView];
				break;
			case SESlideTableViewCellSlideStateLeft:
				if ((m_preparedSlideStates & SESlideStateOptionLeft) == 0) {
					m_preparedSlideStates |= SESlideStateOptionLeft;
					[self prepareToSlideLeft];
				}
				[self slideToLeft];
				break;
			case SESlideTableViewCellSlideStateRight:
				if ((m_preparedSlideStates & SESlideStateOptionRight) == 0) {
					m_preparedSlideStates |= SESlideStateOptionRight;
					[self prepareToSlideRight];
				}
				[self slideToRight];
				break;
		}
	}
}

- (void)updateContentViewSnapshot {
	if (m_snapshotView == nil) {
		return;
	}
	
	// take snapshot with hidding unnecessary content
	m_slideView.hidden = YES;
	UIView* newSnapshot = [self snapshotViewAfterScreenUpdates:YES];
	m_slideView.hidden = NO;
	
	// replace snapshotview
	[m_snapshotView removeFromSuperview];
	m_snapshotView = newSnapshot;
	[m_snapshotContainerView addSubview:m_snapshotView];
}

#pragma mark -

- (void)handlePanGesture:(UIPanGestureRecognizer*)gesture {
	switch (gesture.state) {
		case UIGestureRecognizerStateBegan: {
			[self cancelSlideAnimation];
			m_panStartSnapshotViewOriginX = 0;
			if (m_snapshotContainerView) {
				m_panStartSnapshotViewOriginX = m_snapshotContainerView.frame.origin.x;
			}
		}	break;
		case UIGestureRecognizerStateCancelled:
		case UIGestureRecognizerStateEnded: {
			if (!m_snapshotContainerView) {
				break;
			}
			const CGFloat VELOCITY_TH = 10;
			CGPoint velocity = [gesture velocityInView:self];
			CGFloat velocityX = velocity.x;
			if (fabs(velocity.y) > fabs(velocity.x)) {
				velocityX = 0;
			} else if (fabs(velocity.x) < VELOCITY_TH) {
				velocityX = 0;
			}
			SESlideTableViewCellSlideState targetSlideState = SESlideTableViewCellSlideStateCenter;
			if (m_preparedSlideStates & SESlideStateOptionLeft) {
				if (velocityX < 0) {
					targetSlideState = SESlideTableViewCellSlideStateCenter;
				} else if (velocityX > 0) {
					targetSlideState = SESlideTableViewCellSlideStateLeft;
				} else {
					if (m_snapshotContainerView.frame.origin.x > m_leftButtonGroupView.frame.size.width * 0.5) {
						targetSlideState = SESlideTableViewCellSlideStateLeft;
					} else {
						targetSlideState = SESlideTableViewCellSlideStateCenter;
					}
				}
			} else if (m_preparedSlideStates & SESlideStateOptionRight) {
				if (velocityX < 0) {
					targetSlideState = SESlideTableViewCellSlideStateRight;
				} else if (velocityX > 0) {
					targetSlideState = SESlideTableViewCellSlideStateCenter;
				} else {
					if (m_snapshotContainerView.frame.origin.x < - m_rightButtonGroupView.frame.size.width * 0.5) {
						targetSlideState = SESlideTableViewCellSlideStateRight;
					} else {
						targetSlideState = SESlideTableViewCellSlideStateCenter;
					}
				}
			}
			[self animateToSlideState:targetSlideState velocity:velocity.x];
		}	break;
		case UIGestureRecognizerStateChanged: {
			// decide which side should be prepared
			switch (m_slideState) {
				case SESlideTableViewCellSlideStateCenter: {
					CGPoint translation = [gesture translationInView:self];
					if (translation.x < 0 && [self canSlideToState:SESlideTableViewCellSlideStateRight]) {
						[self prepareToSlideRight];
					} else if (translation.x > 0 && [self canSlideToState:SESlideTableViewCellSlideStateLeft]) {
						[self prepareToSlideLeft];
					}
				}	break;
				default:
					break;
			}
			if (m_snapshotContainerView) {
				CGPoint translation = [gesture translationInView:self];
				CGRect frame = m_snapshotContainerView.frame;
				switch (m_slideElasticity) {
					case SESlideTableViewCellSlideElasticityHard: {
						CGFloat x = translation.x + m_panStartSnapshotViewOriginX;
						if (x >= 0) {
							// left
							CGFloat targetX = [self originXForSlideState:SESlideTableViewCellSlideStateLeft];
							if (x > targetX) {
								x = (x - targetX) * 0.5f + targetX;
							}
							frame.origin.x = x;
						} else {
							// right
							CGFloat targetX = [self originXForSlideState:SESlideTableViewCellSlideStateRight];
							if (x < targetX) {
								x = (x - targetX) * 0.5f + targetX;
							}
							frame.origin.x = x;
						}
					}	break;
					default: {
						frame.origin.x = translation.x + m_panStartSnapshotViewOriginX;
					}	break;
				}
				m_snapshotContainerView.frame = frame;
			}
		}	break;
		default:
			break;
	}
}

- (void)leftButtonTapped:(UITapGestureRecognizer*)gesture {
	SEButtonView* buttonView = (SEButtonView*)gesture.view;
	if ([m_delegate respondsToSelector:@selector(slideTableViewCell:didTriggerLeftButton:)]) {
		[m_delegate slideTableViewCell:self didTriggerLeftButton:buttonView.buttonIndex];
	}
}

- (void)rightButtonTapped:(UITapGestureRecognizer*)gesture {
	SEButtonView* buttonView = (SEButtonView*)gesture.view;
	if ([m_delegate respondsToSelector:@selector(slideTableViewCell:didTriggerRightButton:)]) {
		[m_delegate slideTableViewCell:self didTriggerRightButton:buttonView.buttonIndex];
	}
}

#pragma mark - UIView

- (void)didMoveToSuperview {
	[super didMoveToSuperview];
	
	if (m_indicatorRightConstraint) {
		m_indicatorRightConstraint.constant = -INDICATOR_OUT_MERGIN - self.sectionIndexWidth;
	}
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
	if (gestureRecognizer == m_panGestureRecognizer) {
		CGPoint velocity = [m_panGestureRecognizer velocityInView:self];
		if (fabs(velocity.x) < fabs(velocity.y)) {
			return NO;
		}
#if 0
		if (m_preparedSlideStates == SESlideStateOptionNone) {
			if ((velocity.x > 0) && [self canSlideToState:SESlideTableViewCellSlideStateLeft]) {
				return YES;
			}
			if ((velocity.x < 0) && [self canSlideToState:SESlideTableViewCellSlideStateRight]) {
				return YES;
			}
		} else {
			return YES;
		}
		return NO;
#else
		return YES;
#endif
	}
	return YES;
}

#pragma mark -

- (BOOL)canSlideToState:(SESlideTableViewCellSlideState)slideState {
	if (m_delegate && [m_delegate respondsToSelector:@selector(slideTableViewCell:canSlideToState:)]) {
		if (! [m_delegate slideTableViewCell:self canSlideToState:slideState]) {
			return NO;
		}
	}
	switch (slideState) {
		case SESlideTableViewCellSlideStateCenter:	return YES;
		case SESlideTableViewCellSlideStateLeft:
			return [m_leftButtonGroupView buttonCount] > 0;
		case SESlideTableViewCellSlideStateRight:
			return [m_rightButtonGroupView buttonCount] > 0;
	}
	return YES;
}
- (CGFloat)originXForSlideState:(SESlideTableViewCellSlideState)slideState {
	switch (slideState) {
		case SESlideTableViewCellSlideStateCenter:
			return 0;
		case SESlideTableViewCellSlideStateLeft:;
			return m_leftButtonGroupView.displayWidth;
		case SESlideTableViewCellSlideStateRight:
			return -(m_rightButtonGroupView.displayWidth + m_rightButtonGroupView.sectionIndexWidth);
	}
}

- (void)prepareToSlideLeft {
	if (m_preparedSlideStates & SESlideStateOptionLeft) {
		return;
	}
	if (m_delegate && [m_delegate respondsToSelector:@selector(slideTableViewCell:wilShowButtonsOfSide:)]) {
		[m_delegate slideTableViewCell:self wilShowButtonsOfSide:SESlideTableViewCellSideLeft];
	}
	m_preparedSlideStates |= SESlideStateOptionLeft;
	if (!m_snapshotContainerView) {
		UITableViewCellSelectionStyle presevedSelectionStyle = self.selectionStyle;
		self.selectionStyle = UITableViewCellSelectionStyleNone;
		m_snapshotView = [self snapshotViewAfterScreenUpdates:YES];
		m_snapshotContainerView = [[UIView alloc] initWithFrame:m_snapshotView.frame];
		m_snapshotContainerView.clipsToBounds = YES;
		[m_snapshotContainerView addSubview:m_snapshotView];
		self.selectionStyle = presevedSelectionStyle;

		[m_slideView addSubview:m_snapshotContainerView];
		[self addSubview:m_slideView];
	}
		
	CGSize size = m_snapshotContainerView.frame.size;
	m_leftButtonGroupView.frame = CGRectMake(size.width, 0, 0, size.height);
	m_leftButtonGroupView.sectionIndexWidth = [self sectionIndexWidth];
	
	[m_slideView addSubview:m_leftButtonGroupView];
	
	// constraints
	NSLayoutConstraint* constraintL = [NSLayoutConstraint constraintWithItem:m_leftButtonGroupView attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:m_snapshotContainerView attribute:NSLayoutAttributeLeft multiplier:1 constant:0];
	constraintL.priority = UILayoutPriorityDefaultHigh;
	[m_constraints addObject:constraintL];
	[m_constraints addObject:[NSLayoutConstraint constraintWithItem:m_leftButtonGroupView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:m_slideView attribute:NSLayoutAttributeLeft multiplier:1 constant:0]];
	[m_constraints addObject:[NSLayoutConstraint constraintWithItem:m_leftButtonGroupView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:m_slideView attribute:NSLayoutAttributeTop multiplier:1 constant:0]];
	[m_constraints addObject:[NSLayoutConstraint constraintWithItem:m_leftButtonGroupView attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:m_slideView attribute:NSLayoutAttributeBottom multiplier:1 constant:0]];
	[self addConstraints:m_constraints];
	
	[m_slideView layoutIfNeeded];
}
- (void)prepareToSlideRight {
	if (m_preparedSlideStates & SESlideStateOptionRight) {
		return;
	}
	if (m_delegate && [m_delegate respondsToSelector:@selector(slideTableViewCell:wilShowButtonsOfSide:)]) {
		[m_delegate slideTableViewCell:self wilShowButtonsOfSide:SESlideTableViewCellSideRight];
	}
	m_preparedSlideStates |= SESlideStateOptionRight;
	if (!m_snapshotContainerView) {
		UITableViewCellSelectionStyle presevedSelectionStyle = self.selectionStyle;
		self.selectionStyle = UITableViewCellSelectionStyleNone;
		m_snapshotView = [self snapshotViewAfterScreenUpdates:YES];
		m_snapshotContainerView = [[UIView alloc] initWithFrame:m_snapshotView.frame];
		m_snapshotContainerView.clipsToBounds = YES;
		[m_snapshotContainerView addSubview:m_snapshotView];
		self.selectionStyle = presevedSelectionStyle;
		
		[m_slideView addSubview:m_snapshotContainerView];
		[self addSubview:m_slideView];
	}
	
	CGSize size = m_snapshotContainerView.frame.size;
	m_rightButtonGroupView.frame = CGRectMake(size.width, 0, 0, size.height);
	m_rightButtonGroupView.sectionIndexWidth = [self sectionIndexWidth];
	
	[m_slideView addSubview:m_rightButtonGroupView];

	// constraints
	NSLayoutConstraint* constraintL = [NSLayoutConstraint constraintWithItem:m_rightButtonGroupView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:m_snapshotContainerView attribute:NSLayoutAttributeRight multiplier:1 constant:0];
	constraintL.priority = UILayoutPriorityDefaultHigh;
	[m_constraints addObject:constraintL];
	[m_constraints addObject:[NSLayoutConstraint constraintWithItem:m_rightButtonGroupView attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:m_slideView attribute:NSLayoutAttributeRight multiplier:1 constant:0]];
	[m_constraints addObject:[NSLayoutConstraint constraintWithItem:m_rightButtonGroupView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:m_slideView attribute:NSLayoutAttributeTop multiplier:1 constant:0]];
	[m_constraints addObject:[NSLayoutConstraint constraintWithItem:m_rightButtonGroupView attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:m_slideView attribute:NSLayoutAttributeBottom multiplier:1 constant:0]];
	[self addConstraints:m_constraints];
	
	[m_slideView layoutIfNeeded];
}

- (void)slideToLeft {
	CGRect frame = m_snapshotContainerView.frame;
	frame.origin.x = [self originXForSlideState:SESlideTableViewCellSlideStateLeft];
	m_snapshotContainerView.frame = frame;
}

- (void)slideToRight {
	CGRect frame = m_snapshotContainerView.frame;
	frame.origin.x = [self originXForSlideState:SESlideTableViewCellSlideStateRight];
	m_snapshotContainerView.frame = frame;
}

- (void)cleanUpSlideView {
	[m_slideView removeFromSuperview];
	[m_snapshotContainerView removeFromSuperview];
	[m_leftButtonGroupView removeFromSuperview];
	[m_rightButtonGroupView removeFromSuperview];
	m_snapshotContainerView = nil;
	m_snapshotView = nil;
	[m_slideView removeConstraints:m_constraints];
	[m_constraints removeAllObjects];
}

- (void)animateToSlideState:(SESlideTableViewCellSlideState)slideState velocity:(CGFloat)velocity {
	UIView* snapshotView = m_snapshotContainerView;
	
	if (m_delegate && [m_delegate respondsToSelector:@selector(slideTableViewCell:willSlideToState:)]) {
		[m_delegate slideTableViewCell:self willSlideToState:slideState];
	}
	
	CGFloat targetPositionX = [self originXForSlideState:slideState];
	CGFloat currentPositionX = snapshotView.frame.origin.x;
	CGFloat distance = targetPositionX - currentPositionX;
	CGFloat initialVelocity = (fabs(distance) > 1.0e-5) ? (velocity / distance) : 0;
	
	m_slideAnimationId++;
	uint32_t animationId = m_slideAnimationId;
	
	[UIView animateWithDuration:SE_ANIMATION_DURATION delay:0 usingSpringWithDamping:SE_ANIMATION_DUMPING initialSpringVelocity:initialVelocity options:UIViewAnimationOptionAllowUserInteraction|UIViewAnimationOptionBeginFromCurrentState animations:^(){
		CGRect frame = snapshotView.frame;
		frame.origin.x = targetPositionX;
		snapshotView.frame = frame;
		[m_slideView layoutIfNeeded];
	} completion:^(BOOL finished){
		if (finished) {
			if (animationId != m_slideAnimationId) {
				return;
			}
			m_slideState = slideState;
			if (slideState == SESlideTableViewCellSlideStateCenter) {
				m_preparedSlideStates = SESlideStateOptionNone;
				[self cleanUpSlideView];
			}
			if (m_delegate && [m_delegate respondsToSelector:@selector(slideTableViewCell:didSlideToState:)]) {
				[m_delegate slideTableViewCell:self didSlideToState:slideState];
			}
		}
	}];
}

- (void)cancelSlideAnimation {
	// cancel previous animation
	[m_snapshotContainerView.layer removeAllAnimations];
	
	m_slideAnimationId++;
}

- (UITableView*)tableViewInSuperviews {
	for (UIView* view = self.superview; view; view = view.superview) {
		if ([view isKindOfClass:[UITableView class]]) {
			return (UITableView*)view;
		}
	}
	return nil;
}

- (CGFloat)sectionIndexWidth {
	UITableView* tableView = [self tableViewInSuperviews];
	if (tableView && tableView.dataSource && [tableView.dataSource respondsToSelector:@selector(sectionIndexTitlesForTableView:)]) {
		if ([tableView.dataSource sectionIndexTitlesForTableView:tableView]) {
			return SE_SECTION_INDEX_WIDTH;
		}
	}
	return 0;
}

- (void)updateSlideIndicatorVisibility {
	if (m_indicator) {
		SESlideIndicatorSideOption sides = SESlideIndicatorSideOptionNone;
		if (m_showsRightSlideIndicator && [m_rightButtonGroupView buttonCount] > 0) {
			sides |= SESlideStateOptionRight;
		}
		if (m_showsLeftSlideIndicator && [m_leftButtonGroupView buttonCount] > 0) {
			sides |= SESlideStateOptionLeft;
		}
		m_indicator.sideOption = sides;
	}
}

@end

#pragma mark - 

#define BUTTON_MARGIN	20

@implementation SESlideTableViewCell (ButtonUtility)

- (void)addLeftButton:(UIView*)button buttonWidth:(CGFloat)buttonWidth backgroundColor:(UIColor*)backgroundColor {
	[self addButton:button buttonWidth:buttonWidth backgroundColor:backgroundColor side:SESlideTableViewCellSideLeft];
}
- (void)addRightButton:(UIView*)button buttonWidth:(CGFloat)buttonWidth backgroundColor:(UIColor*)backgroundColor {
	[self addButton:button buttonWidth:buttonWidth backgroundColor:backgroundColor side:SESlideTableViewCellSideRight];
}
- (void)removeAllLeftButtons {
	[self removeAllButtonsWithSide:SESlideTableViewCellSideLeft];
}
- (void)removeAllRightButtons {
	[self removeAllButtonsWithSide:SESlideTableViewCellSideRight];
}

- (void)addButtonWithText:(NSString*)text textColor:(UIColor*)textColor backgroundColor:(UIColor*)backgroundColor side:(SESlideTableViewCellSide)side {
	UILabel* label = [UILabel new];
	label.text = text;
	label.textAlignment = NSTextAlignmentCenter;
	label.textColor = textColor;
	CGSize size = [label sizeThatFits:CGSizeMake(FLT_MAX, FLT_MAX)];
	label.frame = CGRectMake(0, 0, size.width, size.height);
	[self addButton:label buttonWidth:size.width + BUTTON_MARGIN backgroundColor:backgroundColor side:side];
}

- (void)addButtonWithText:(NSString*)text textColor:(UIColor*)textColor backgroundColor:(UIColor*)backgroundColor font:(UIFont*)font side:(SESlideTableViewCellSide)side {
	UILabel* label = [UILabel new];
	label.text = text;
	label.textAlignment = NSTextAlignmentCenter;
	label.textColor = textColor;
	label.font = font;
	CGSize size = [label sizeThatFits:CGSizeMake(FLT_MAX, FLT_MAX)];
	label.frame = CGRectMake(0, 0, size.width, size.height);
	[self addButton:label buttonWidth:size.width + BUTTON_MARGIN backgroundColor:backgroundColor side:side];
}

- (void)addButtonWithImage:(UIImage*)image backgroundColor:(UIColor*)backgroundColor side:(SESlideTableViewCellSide)side {
	UIImageView* imageView = [[UIImageView alloc] initWithImage:image];
	UIView* view = [[UIView alloc] initWithFrame:imageView.frame];
#if 1
	imageView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleTopMargin|UIViewAutoresizingFlexibleBottomMargin;
#else
	[imageView setTranslatesAutoresizingMaskIntoConstraints:NO];
	[view addConstraint:[NSLayoutConstraint constraintWithItem:imageView attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:view attribute:NSLayoutAttributeCenterX multiplier:1 constant:0]];
	[view addConstraint:[NSLayoutConstraint constraintWithItem:imageView attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:view attribute:NSLayoutAttributeCenterY multiplier:1 constant:0]];
#endif
	[view addSubview:imageView];
	[self addButton:view buttonWidth:image.size.width + BUTTON_MARGIN backgroundColor:backgroundColor side:side];
}

- (void)addButtonWithImage:(UIImage*)image text:(NSString*)text textColor:(UIColor*)textColor font:(UIFont*)font backgroundColor:(UIColor*)backgroundColor side:(SESlideTableViewCellSide)side {
	UIImageView* imageView = [[UIImageView alloc] initWithImage:image];
	UILabel* label = [UILabel new];
	label.text = text;
	label.textAlignment = NSTextAlignmentCenter;
	if (font) {
		label.font = font;
	}
	label.textColor = textColor;
	CGSize textSize = [label sizeThatFits:CGSizeMake(FLT_MAX, FLT_MAX)];
	CGFloat width = MAX(textSize.width, image.size.width);
	imageView.frame = CGRectMake((width - image.size.width) * 0.5, 0, image.size.width, image.size.height);
	label.frame = CGRectMake((width - textSize.width) * 0.5, image.size.height, textSize.width, textSize.height);
	UIView* subView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, image.size.height + textSize.height)];

	[subView addSubview:imageView];
	[subView addSubview:label];
	
	UIView* view = [[UIView alloc] initWithFrame:subView.frame];
	subView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleTopMargin|UIViewAutoresizingFlexibleBottomMargin;
	[view addSubview:subView];
	
	[self addButton:view buttonWidth:width + BUTTON_MARGIN backgroundColor:backgroundColor side:side];
}


- (void)addLeftButtonWithText:(NSString*)text textColor:(UIColor*)textColor backgroundColor:(UIColor*)backgroundColor {
	[self addButtonWithText:text textColor:textColor backgroundColor:backgroundColor side:SESlideTableViewCellSideLeft];
}
- (void)addLeftButtonWithText:(NSString*)text textColor:(UIColor*)textColor backgroundColor:(UIColor*)backgroundColor font:(UIFont*)font {
	[self addButtonWithText:text textColor:textColor backgroundColor:backgroundColor font:font side:SESlideTableViewCellSideLeft];
}
- (void)addRightButtonWithText:(NSString*)text textColor:(UIColor*)textColor backgroundColor:(UIColor*)backgroundColor {
	[self addButtonWithText:text textColor:textColor backgroundColor:backgroundColor side:SESlideTableViewCellSideRight];
}
- (void)addRightButtonWithText:(NSString*)text textColor:(UIColor*)textColor backgroundColor:(UIColor*)backgroundColor font:(UIFont*)font {
	[self addButtonWithText:text textColor:textColor backgroundColor:backgroundColor font:font side:SESlideTableViewCellSideRight];
}
- (void)addLeftButtonWithImage:(UIImage*)image backgroundColor:(UIColor*)backgroundColor{
	[self addButtonWithImage:image backgroundColor:backgroundColor side:SESlideTableViewCellSideLeft];
}
- (void)addRightButtonWithImage:(UIImage*)image backgroundColor:(UIColor*)backgroundColor {
	[self addButtonWithImage:image backgroundColor:backgroundColor side:SESlideTableViewCellSideRight];
}
- (void)addLeftButtonWithImage:(UIImage*)image text:(NSString*)text textColor:(UIColor*)textColor font:(UIFont*)font backgroundColor:(UIColor*)backgroundColor {
	[self addButtonWithImage:image text:text textColor:textColor font:font backgroundColor:backgroundColor side:SESlideTableViewCellSideLeft];
}
- (void)addRightButtonWithImage:(UIImage*)image text:(NSString*)text textColor:(UIColor*)textColor font:(UIFont*)font backgroundColor:(UIColor*)backgroundColor {
	[self addButtonWithImage:image text:text textColor:textColor font:font backgroundColor:backgroundColor side:SESlideTableViewCellSideRight];
}



@end


