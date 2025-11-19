//
//  StoreManager.h
//  modizer
//
//  Created by Yohann Magnien David on 17/11/2025.
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol StoreManagerDelegate <NSObject>
@optional
- (void)productsLoaded:(NSArray<SKProduct *> *)products;
- (void)productPurchased:(NSString *)productIdentifier;
- (void)purchaseFailed:(NSError *)error;
- (void)purchaseRestored;
@end

@interface StoreManager : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>

@property (nonatomic, weak) id<StoreManagerDelegate> delegate;
@property (nonatomic, strong, readonly) NSArray<SKProduct *> *products;
@property (nonatomic, readonly) BOOL isLoadingProducts;

+ (instancetype)sharedManager;
- (void)loadProducts;
- (void)refreshProducts;  // Force refresh
- (void)purchaseProduct:(SKProduct *)product;
- (BOOL)canMakePurchases;
- (void)clearCache;  // Clear cached products
- (NSString *)formattedTipsTotalWithLocale:(NSLocale *)locale;

@end

NS_ASSUME_NONNULL_END
