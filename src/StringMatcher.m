//
//  StringMatcher.m
//  modizer
//
//  Created by Yohann Magnien David on 28/01/2026.
//

#define DEBUG

#import "StringMatcher.h"

@implementation StringMatcher

#pragma mark - Public Methods

- (NSInteger)bestMatchIndexFor:(NSString *)search 
                       inArray:(NSArray<NSString *> *)array 
                  minimumScore:(CGFloat)minimumScore {
    
    if (!search || [search length] == 0 || !array || [array count] == 0) {
        return NSNotFound;
    }
    
    CGFloat bestScore = 0.0;
    NSInteger bestIndex = NSNotFound;
    
    for (NSInteger i = 0; i < [array count]; i++) {
        NSString *candidate = array[i];
        CGFloat score = [self scoreForMatch:candidate withSearch:search];
        
        #ifdef DEBUG
        NSLog(@"Score for '%@' vs '%@': %.4f", search, candidate, score);
        #endif
        
        if (score > bestScore && score >= minimumScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    
    #ifdef DEBUG
    if (bestIndex!=NSNotFound) {
        NSLog(@"✅ Best match: '%@' with score %.4f", array[bestIndex], bestScore);
    } else {
        NSLog(@"❌ No match found with minimum score %.2f", minimumScore);
    }
    #endif
    
    return bestIndex;
}

- (CGFloat)scoreForMatch:(NSString *)candidate withSearch:(NSString *)search {
    if (!candidate || !search) return 0.0;
    
    NSString *candidateLower = [[self normalizeRomanNumerals:candidate] lowercaseString];
    NSString *searchLower = [[self normalizeRomanNumerals:search] lowercaseString];
    
    // Exact match
    if ([candidateLower isEqualToString:searchLower]) {
        return 1.0;
    }
    
    // Exact match après normalisation des numéros romains
    NSString *candidateNormalized = [self normalizeRomanNumerals:candidate];
    NSString *searchNormalized = [self normalizeRomanNumerals:search];
    if ([[candidateNormalized lowercaseString] isEqualToString:[searchNormalized lowercaseString]]) {
        return 0.98;
    }
    
    NSArray *searchTokens = [self tokenize:searchLower];
    NSArray *candidateTokens = [self tokenize:candidateLower];
    
    // Extraire les numéros
    NSSet *searchNumbers = [self extractNumbers:searchLower];
    NSSet *candidateNumbers = [self extractNumbers:candidateLower];
    
    // Calculer le score de base
    CGFloat baseScore = [self calculateBaseScore:candidateLower
                                      searchTokens:searchTokens
                                   candidateTokens:candidateTokens
                                            search:searchLower];
    
    // Appliquer les pénalités/bonus en fonction des numéros
    
    // CAS 1: Recherche SANS numéro, candidat AVEC numéro → PÉNALITÉ
    if ([searchNumbers count] == 0 && [candidateNumbers count] > 0) {
        CGFloat penalty = 0.15 * [candidateNumbers count];
        return MAX(0.0, baseScore - penalty);
    }
    
    // CAS 2: Recherche AVEC numéro, candidat SANS numéro → OK (pas de pénalité)
    if ([searchNumbers count] > 0 && [candidateNumbers count] == 0) {
        return baseScore;
    }
    
    // CAS 3: Les deux ont des numéros
    if ([searchNumbers count] > 0 && [candidateNumbers count] > 0) {
        if ([searchNumbers isEqualToSet:candidateNumbers]) {
            // Numéros identiques → pas de pénalité
            return baseScore;
        } else {
            // Numéros différents → pénalité forte
            return MAX(0.0, baseScore - 0.20);
        }
    }
    
    // CAS 4: Aucun des deux n'a de numéro → score normal
    return baseScore;
}

#pragma mark - Private Methods - Score Calculation

- (CGFloat)calculateBaseScore:(NSString *)candidate
                  searchTokens:(NSArray *)searchTokens
               candidateTokens:(NSArray *)candidateTokens
                        search:(NSString *)search {
    
    // Check for initials match (acronym)
    BOOL hasStrongIndicator = [candidate containsString:@"&"] || [candidate containsString:@"."];
    if (hasStrongIndicator && [self matchesInitials:search withCandidate:candidate]) {
        CGFloat wordBonus = [self exactWordMatchBonus:search withCandidate:candidate];
        return 0.92 + wordBonus;
    }
    
    // Same series, same number check
    NSString *searchSeriesNumber = [self extractSeriesNumber:search];
    NSString *candidateSeriesNumber = [self extractSeriesNumber:candidate];
    
    if (searchSeriesNumber && candidateSeriesNumber &&
        [searchSeriesNumber isEqualToString:candidateSeriesNumber]) {
        return 0.95;
    }
    
    // Substring containment
    if ([candidate containsString:search]) {
        return 0.85;
    }
    
    // Token matching
    NSUInteger matchedTokens = 0;
    for (NSString *searchToken in searchTokens) {
        for (NSString *candidateToken in candidateTokens) {
            if ([candidateToken containsString:searchToken] ||
                [searchToken containsString:candidateToken]) {
                matchedTokens++;
                break;
            }
        }
    }
    
    CGFloat tokenMatchRatio = (CGFloat)matchedTokens / [searchTokens count];
    
    if (tokenMatchRatio < 0.7) {
        return 0.0; // Not enough token overlap
    }
    
    // Proximity bonus for series
    if (searchSeriesNumber && candidateSeriesNumber) {
        NSInteger searchNum = [searchSeriesNumber integerValue];
        NSInteger candidateNum = [candidateSeriesNumber integerValue];
        NSInteger distance = labs(searchNum - candidateNum);
        
        if (distance <= 2) {
            return 0.70 + (0.05 * (3 - distance));
        }
    }
    
    // Word match bonus
    CGFloat wordBonus = [self exactWordMatchBonus:search withCandidate:candidate];
    if (wordBonus > 0) {
        return 0.70 + wordBonus;
    }
    
    // Levenshtein distance fallback
    NSUInteger distance = [self levenshteinDistanceBetween:search and:candidate];
    NSUInteger maxLength = MAX([search length], [candidate length]);
    CGFloat similarity = 1.0 - ((CGFloat)distance / maxLength);
    
    return similarity * 0.65;
}

- (CGFloat)exactWordMatchBonus:(NSString *)search withCandidate:(NSString *)candidate {
    NSArray *searchWords = [self tokenize:search];
    NSArray *candidateWords = [self tokenize:candidate];
    
    CGFloat bonus = 0.0;
    
    for (NSString *searchWord in searchWords) {
        if ([searchWord length] < 6) continue; // Only long words
        
        for (NSString *candidateWord in candidateWords) {
            // Exact match
            if ([searchWord isEqualToString:candidateWord]) {
                bonus += 0.10;
                continue;
            }
            
            // Quasi-exact (1-2 char difference)
            NSUInteger distance = [self levenshteinDistanceBetween:searchWord and:candidateWord];
            if (distance <= 2) {
                bonus += 0.05;
            }
        }
    }
    
    return MIN(bonus, 0.15); // Cap bonus
}

- (BOOL)matchesInitials:(NSString *)search withCandidate:(NSString *)candidate {
    NSArray *searchTokens = [self tokenize:search];
    NSArray *candidateTokens = [self tokenize:candidate];
    
    // Filter out symbols from candidate tokens
    NSMutableArray *filteredCandidateTokens = [NSMutableArray array];
    for (NSString *token in candidateTokens) {
        if ([self isSymbolOnlyToken:token]) continue;
        [filteredCandidateTokens addObject:token];
    }
    
    if ([searchTokens count] != [filteredCandidateTokens count]) {
        return NO;
    }
    
    for (NSInteger i = 0; i < [searchTokens count]; i++) {
        NSString *searchToken = searchTokens[i];
        NSString *candidateToken = filteredCandidateTokens[i];
        
        if ([searchToken length] == 0 || [candidateToken length] == 0) {
            return NO;
        }
        
        unichar searchFirst = [searchToken characterAtIndex:0];
        unichar candidateFirst = [candidateToken characterAtIndex:0];
        
        if (searchFirst != candidateFirst) {
            return NO;
        }
    }
    
    return YES;
}

- (BOOL)isSymbolOnlyToken:(NSString *)token {
    NSCharacterSet *symbolSet = [NSCharacterSet characterSetWithCharactersInString:@"&:.-_"];
    NSCharacterSet *tokenSet = [NSCharacterSet characterSetWithCharactersInString:token];
    return [symbolSet isSupersetOfSet:tokenSet];
}

#pragma mark - Private Methods - String Processing

- (NSString *)normalizeRomanNumerals:(NSString *)text {
    NSDictionary *romanToArabic = @{
        @"\\bI\\b": @"1",
        @"\\bII\\b": @"2",
        @"\\bIII\\b": @"3",
        @"\\bIV\\b": @"4",
        @"\\bV\\b": @"5",
        @"\\bVI\\b": @"6",
        @"\\bVII\\b": @"7",
        @"\\bVIII\\b": @"8",
        @"\\bIX\\b": @"9",
        @"\\bX\\b": @"10",
        @"\\bXI\\b": @"11",
        @"\\bXII\\b": @"12",
        @"\\bXIII\\b": @"13",
        @"\\bXIV\\b": @"14",
        @"\\bXV\\b": @"15",
        @"\\bXVI\\b": @"16",
        @"\\bXVII\\b": @"17",
        @"\\bXVIII\\b": @"18",
        @"\\bXIX\\b": @"19",
        @"\\bXX\\b": @"20"
    };
    
    NSString *result = text;
    
    for (NSString *pattern in romanToArabic) {
        NSString *replacement = romanToArabic[pattern];
        NSRegularExpression *regex = [NSRegularExpression
            regularExpressionWithPattern:pattern
            options:NSRegularExpressionCaseInsensitive
            error:nil];
        
        result = [regex stringByReplacingMatchesInString:result
                                                 options:0
                                                   range:NSMakeRange(0, [result length])
                                            withTemplate:replacement];
    }
    
    return result;
}

- (NSArray *)tokenize:(NSString *)text {
    NSMutableArray *tokens = [NSMutableArray array];
    
    NSRegularExpression *regex = [NSRegularExpression
        regularExpressionWithPattern:@"[\\w']+"
        options:0
        error:nil];
    
    NSArray *matches = [regex matchesInString:text
                                      options:0
                                        range:NSMakeRange(0, [text length])];
    
    for (NSTextCheckingResult *match in matches) {
        NSString *token = [text substringWithRange:match.range];
        [tokens addObject:[token lowercaseString]];
    }
    
    return tokens;
}

- (NSSet *)extractNumbers:(NSString *)text {
    NSMutableSet *numbers = [NSMutableSet set];
    
    NSRegularExpression *regex = [NSRegularExpression
        regularExpressionWithPattern:@"\\b(\\d+)\\b"
        options:0
        error:nil];
    
    NSArray *matches = [regex matchesInString:text
                                      options:0
                                        range:NSMakeRange(0, [text length])];
    
    for (NSTextCheckingResult *match in matches) {
        NSString *number = [text substringWithRange:[match rangeAtIndex:1]];
        [numbers addObject:number];
    }
    
    return numbers;
}

- (NSString *)extractSeriesNumber:(NSString *)text {
    NSRegularExpression *regex = [NSRegularExpression
        regularExpressionWithPattern:@"\\b(\\d+)\\b"
        options:0
        error:nil];
    
    NSTextCheckingResult *match = [regex firstMatchInString:text
                                                    options:0
                                                      range:NSMakeRange(0, [text length])];
    
    if (match && match.numberOfRanges > 1) {
        return [text substringWithRange:[match rangeAtIndex:1]];
    }
    
    return nil;
}

- (NSUInteger)levenshteinDistanceBetween:(NSString *)source and:(NSString *)target {
    NSUInteger sourceLength = [source length];
    NSUInteger targetLength = [target length];
    
    if (sourceLength == 0) return targetLength;
    if (targetLength == 0) return sourceLength;
    
    NSUInteger *previousRow = (NSUInteger *)malloc((targetLength + 1) * sizeof(NSUInteger));
    NSUInteger *currentRow = (NSUInteger *)malloc((targetLength + 1) * sizeof(NSUInteger));
    
    for (NSUInteger i = 0; i <= targetLength; i++) {
        previousRow[i] = i;
    }
    
    for (NSUInteger i = 0; i < sourceLength; i++) {
        currentRow[0] = i + 1;
        unichar c1 = [source characterAtIndex:i];
        
        for (NSUInteger j = 0; j < targetLength; j++) {
            unichar c2 = [target characterAtIndex:j];
            
            NSUInteger insertions = previousRow[j + 1] + 1;
            NSUInteger deletions = currentRow[j] + 1;
            NSUInteger substitutions = previousRow[j] + (c1 == c2 ? 0 : 1);
            
            currentRow[j + 1] = MIN(MIN(insertions, deletions), substitutions);
        }
        
        NSUInteger *temp = previousRow;
        previousRow = currentRow;
        currentRow = temp;
    }
    
    NSUInteger result = previousRow[targetLength];
    
    free(previousRow);
    free(currentRow);
    
    return result;
}

@end
