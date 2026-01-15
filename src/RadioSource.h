//
//  RadioSource.h
//  modizer
//
//  Created by Yohann Magnien David on 10/01/2026.
//

#import <Foundation/Foundation.h>
#import "DetailViewControllerIphone.h"

NS_ASSUME_NONNULL_BEGIN

@interface RadioSource : NSObject <NSURLSessionDelegate>

enum t_radioSource {
    RS_NONE=0,
    RS_COLLECTION_AMP,
    RS_COLLECTION_MODLAND,
    RS_COLLECTION_ASMA,
    RS_COLLECTION_HVSC,
    RS_COLLECTION_CGSC,
};

@property (nonatomic, assign) t_radioSource mRadioSource;
@property (nonatomic, assign) int mRadioSource_mode;
@property (nonatomic, assign) int mPendingNewFileToPlay;
@property (nonatomic, assign) int mRetryCount,mRetryDuplCount;
@property (nonatomic, assign) bool mActive;
@property (nonatomic, strong) DetailViewControllerIphone *detailVC;
@property (nonatomic, strong) NSMutableArray *mFilesList;
@property (nonatomic, strong) NSMutableArray *mFilesExistInLibrary;
@property (nonatomic, strong) NSMutableArray *mSourceData;
@property (nonatomic, strong) NSMutableArray *mHistory,*mHistoryComp;
@property (nonatomic, strong) NSTimer *fetchDebounceTimer;

-(void) moveNext;
-(void) activate;
-(void) stop;
-(bool) isActive;
-(int) queueSize;
-(NSString *) getQueueLabel:(int)slot;
-(NSString *) getHistoryLabel:(int)depth;
-(NSString *) radioSourceName;
-(bool) saveFileToLibrary:(int)slot;
-(bool) isInLibrary:(int)slot;

@end

NS_ASSUME_NONNULL_END
