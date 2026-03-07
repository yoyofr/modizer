//
//  RadioSource.h
//  modizer
//
//  Created by Yohann Magnien David on 10/01/2026.
//

#import <Foundation/Foundation.h>
#import "DetailViewControllerIphone.h"

NS_ASSUME_NONNULL_BEGIN

@interface RadioSource : NSObject <NSURLSessionDelegate> {
    NSData *joshw_urlData[27+4];
    int joshw_data_cnt;
}

enum t_radioSource {
    RS_NONE=0,
    RS_COLLECTION_AMP,
    RS_COLLECTION_MODLAND,
    RS_COLLECTION_ASMA,
    RS_COLLECTION_HVSC,
    RS_COLLECTION_CGSC,
    RS_COLLECTION_SNES,
    RS_COLLECTION_SMSP,
    RS_COLLECTION_VGMR,
    RS_COLLECTION_ZXART,
    RS_COLLECTION_JOSHW,
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
@property (nonatomic, strong) NSString *mCurrentPath;
@property (nonatomic, assign) int mCurrentPlayingSlot;

@property (nonatomic, strong) NSURLSessionConfiguration *mURLSessionConfig;
@property (nonatomic, strong) NSOperationQueue *mURLSessionQueue;
@property (nonatomic, strong) NSURLSession *mURLSsession;

-(void) moveNext:(bool)removeCurrentEntry;
-(void) movePrev:(int)idx;
-(void) startCurrent;

-(void) activate;
-(void) stop;
-(bool) isActive;
-(int) queueSize;
-(NSString *) getQueueLabel:(int)slot;
-(NSString *) getHistoryLabel:(int)idx;
-(int) getHistorySize;
-(NSString *) radioSourceName;
-(bool) saveFileToLibrary:(NSString* __nullable)suggestedName;
-(bool) isInLibrary:(int)slot;
-(void) cleanFiles;

@end

NS_ASSUME_NONNULL_END
