//
//  StoreManager.mm
//  modizer
//
//  Created by Yohann Magnien David on 17/11/2025.
//

#import "StoreManager.h"

@interface StoreManager ()
@property (nonatomic, strong) NSArray<SKProduct *> *products;
@property (nonatomic, strong) SKProductsRequest *productsRequest;
@property (nonatomic, strong) NSSet<NSString *> *productIdentifiers;
@property (nonatomic, assign) BOOL isLoadingProducts;
@property (nonatomic, strong) NSDate *lastLoadTime;
@property (nonatomic, assign) NSTimeInterval cacheTimeout; // Default cache timeout

@property (nonatomic, strong) NSDecimalNumber *cachedTipsTotal;

@end

@implementation StoreManager

+ (instancetype)sharedManager {
    static StoreManager *sharedManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedManager = [[StoreManager alloc] init];
    });
    return sharedManager;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
        
        // Set default cache timeout (e.g., 5 minutes)
        self.cacheTimeout = 300; // 5 minutes in seconds
        
        // Define your product identifiers
        self.productIdentifiers = [NSSet setWithObjects:
                                   @"com.yoyofr.modizer.tip.small",
                                   @"com.yoyofr.modizer.tip.medium",
                                   @"com.yoyofr.modizer.tip.large",
                                   nil];
        self.cachedTipsTotal = [self tipsTotal];
    }
    return self;
}

- (void)dealloc {
    [[SKPaymentQueue defaultQueue] removeTransactionObserver:self];
    [self.productsRequest cancel];
}

- (BOOL)isCacheValid {
    if (!self.products || !self.lastLoadTime) {
        return NO;
    }
    
    NSTimeInterval timeSinceLastLoad = [[NSDate date] timeIntervalSinceDate:self.lastLoadTime];
    return timeSinceLastLoad < self.cacheTimeout;
}

- (void)loadProducts {
    // If products are cached and still valid, return them immediately
    if ([self isCacheValid]) {
        NSLog(@"Using cached products (loaded %.0f seconds ago)", [[NSDate date] timeIntervalSinceDate:self.lastLoadTime]);
        dispatch_async(dispatch_get_main_queue(), ^{
            if ([self.delegate respondsToSelector:@selector(productsLoaded:)]) {
                [self.delegate productsLoaded:self.products];
            }
        });
        return;
    }
    
    // Otherwise, fetch fresh products
    [self fetchProductsFromAppStore];
}

- (void)refreshProducts {
    NSLog(@"Force refreshing products...");
    [self clearCache];
    [self fetchProductsFromAppStore];
}

- (void)clearCache {
    self.products = nil;
    self.lastLoadTime = nil;
}

- (void)fetchProductsFromAppStore {
    // Prevent multiple simultaneous requests
    if (self.isLoadingProducts) {
        NSLog(@"Already loading products, skipping duplicate request");
        return;
    }
    
    // Cancel any existing request
    if (self.productsRequest) {
        [self.productsRequest cancel];
        self.productsRequest = nil;
    }
    
    self.isLoadingProducts = YES;
    
    NSLog(@"Fetching products from App Store...");
    self.productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:self.productIdentifiers];
    self.productsRequest.delegate = self;
    [self.productsRequest start];
}

- (BOOL)canMakePurchases {
    return [SKPaymentQueue canMakePayments];
}

- (void)purchaseProduct:(SKProduct *)product {
    if (![self canMakePurchases]) {
        NSError *error = [NSError errorWithDomain:@"StoreManager"
                                            code:0
                                        userInfo:@{NSLocalizedDescriptionKey: @"In-app purchases are disabled"}];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if ([self.delegate respondsToSelector:@selector(purchaseFailed:)]) {
                [self.delegate purchaseFailed:error];
            }
        });
        return;
    }
    
    SKPayment *payment = [SKPayment paymentWithProduct:product];
    [[SKPaymentQueue defaultQueue] addPayment:payment];
}

#pragma mark - Tips Total Management

- (NSDecimalNumber *)tipsTotal {
    NSString *stored = [NSUserDefaults.standardUserDefaults stringForKey:@"consumable.tips.total"] ?: @"0";
    NSDecimalNumber *value = [NSDecimalNumber decimalNumberWithString:stored];
    if ([value isEqualToNumber:[NSDecimalNumber notANumber]]) {
        value = [NSDecimalNumber zero];
    }
    return value;
}

- (void)resetTipsTotal {
    [NSUserDefaults.standardUserDefaults removeObjectForKey:@"consumable.tips.total"]; 
    [NSUserDefaults.standardUserDefaults synchronize];
    self.cachedTipsTotal = [NSDecimalNumber zero];
}

- (void)incrementTipsTotalByAmount:(NSDecimalNumber *)amount inCurrencyCode:(NSString *)currencyCode {
    if (!amount || [amount compare:[NSDecimalNumber zero]] != NSOrderedDescending) {
        return;
    }
    // For simplicity, we accumulate the numeric value regardless of currency.
    // If you sell in multiple currencies and need a single-currency total, convert here before adding.
    NSDecimalNumber *current = self.cachedTipsTotal ?: [self tipsTotal];
    NSDecimalNumber *newTotal = [current decimalNumberByAdding:amount];
    self.cachedTipsTotal = newTotal;
    NSString *storeString = newTotal.stringValue;
    [NSUserDefaults.standardUserDefaults setObject:storeString forKey:@"consumable.tips.total"]; 
    [NSUserDefaults.standardUserDefaults synchronize];
}

- (NSString *)formattedTipsTotalWithLocale:(NSLocale *)locale {
    NSDecimalNumber *total = self.cachedTipsTotal ?: [self tipsTotal];
    NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
    formatter.numberStyle = NSNumberFormatterCurrencyStyle;
    if (locale) { formatter.locale = locale; }
    return [formatter stringFromNumber:total] ?: total.stringValue;
}

#pragma mark - SKProductsRequestDelegate

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response {
    NSLog(@"Received %lu products from App Store", (unsigned long)response.products.count);
    
    self.products = response.products;
    self.lastLoadTime = [NSDate date];
    self.isLoadingProducts = NO;
    self.productsRequest = nil;
    
    // Sort products by price
    self.products = [self.products sortedArrayUsingComparator:^NSComparisonResult(SKProduct *obj1, SKProduct *obj2) {
        return [obj1.price compare:obj2.price];
    }];
    
    // Log product details for debugging
    #ifdef DEBUG
    for (SKProduct *product in self.products) {
        NSLog(@"Product loaded: %@ - %@ %@",
              product.productIdentifier,
              product.priceLocale.currencySymbol,
              product.price);
    }
    #endif
    
    // Notify delegate on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(productsLoaded:)]) {
            [self.delegate productsLoaded:self.products];
        }
    });
    
    // Log invalid product identifiers
    for (NSString *invalidIdentifier in response.invalidProductIdentifiers) {
        NSLog(@"⚠️ Invalid product identifier: %@", invalidIdentifier);
    }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error {
    NSLog(@"Failed to load products: %@", error.localizedDescription);
    
    self.isLoadingProducts = NO;
    self.productsRequest = nil;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(purchaseFailed:)]) {
            [self.delegate purchaseFailed:error];
        }
    });
}

- (void)requestDidFinish:(SKRequest *)request {
    NSLog(@"Product request finished");
    self.isLoadingProducts = NO;
    self.productsRequest = nil;
}

#pragma mark - SKPaymentTransactionObserver

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray<SKPaymentTransaction *> *)transactions {
    for (SKPaymentTransaction *transaction in transactions) {
        switch (transaction.transactionState) {
            case SKPaymentTransactionStatePurchased: {
                NSLog(@"✅ Transaction purchased: %@", transaction.payment.productIdentifier);
                
                // Find the SKProduct for this identifier to get the price
                NSString *productID = transaction.payment.productIdentifier;
                SKProduct *matched = nil;
                for (SKProduct *p in self.products) {
                    if ([p.productIdentifier isEqualToString:productID]) { matched = p; break; }
                }
                if (matched) {
                    // Price is an NSDecimalNumber in the product's currency
                    NSDecimalNumber *price = matched.price ?: [NSDecimalNumber zero];
                    NSString *currencyCode = matched.priceLocale.currencyCode;
                    [self incrementTipsTotalByAmount:price inCurrencyCode:currencyCode];
                }
                
                // Finish the transaction
                [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                
                // Notify delegate on main thread
                dispatch_async(dispatch_get_main_queue(), ^{
                    if ([self.delegate respondsToSelector:@selector(productPurchased:)]) {
                        [self.delegate productPurchased:productID];
                    }
                });
                break;
            }
            case SKPaymentTransactionStateFailed: {
                NSLog(@"❌ Transaction failed: %@", transaction.error.localizedDescription);
                
                NSError *error = transaction.error;
                [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                
                // Notify delegate on main thread
                dispatch_async(dispatch_get_main_queue(), ^{
                    if ([self.delegate respondsToSelector:@selector(purchaseFailed:)]) {
                        [self.delegate purchaseFailed:error];
                    }
                });
                break;
            }
            case SKPaymentTransactionStateRestored: {
                NSLog(@"🔄 Transaction restored: %@", transaction.payment.productIdentifier);
                
                // Consumables are not restorable; finish immediately.
                [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                
                // Notify delegate on main thread
                dispatch_async(dispatch_get_main_queue(), ^{
                    if ([self.delegate respondsToSelector:@selector(purchaseRestored)]) {
                        [self.delegate purchaseRestored];
                    }
                });
                break;
            }
            case SKPaymentTransactionStatePurchasing:
                NSLog(@"⏳ Transaction purchasing: %@", transaction.payment.productIdentifier);
                break;
            case SKPaymentTransactionStateDeferred:
                NSLog(@"⏸️ Transaction deferred: %@", transaction.payment.productIdentifier);
                break;
        }
    }
}

@end

