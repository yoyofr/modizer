/*
 * Copyright (c) 2018, 2020 Taner Sener
 *
 * This file is part of MobileFFmpeg.
 *
 * MobileFFmpeg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MobileFFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MediaInformationParser.h"

@implementation MediaInformationParser

+ (MediaInformation*)from:(NSString*)ffprobeJsonOutput {
    NSError *error;

    MediaInformation* mediaInformation = [self from: ffprobeJsonOutput with: error];

    if (error != nil) {
        NSLog(@"MediaInformation parsing failed: %@.\n", error);
    }

    return mediaInformation;
}

+ (MediaInformation*)from:(NSString*)ffprobeJsonOutput with:(NSError*)error {
    NSData *jsonData = [ffprobeJsonOutput dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *jsonDictionary = [NSJSONSerialization JSONObjectWithData:jsonData options:kNilOptions error:&error];
    if (error != nil || jsonDictionary == nil) {
        return nil;
    }

    NSArray* array = [jsonDictionary objectForKey:@"streams"];
    NSMutableArray *streamArray = [[NSMutableArray alloc] init];
    for(int i = 0; i<array.count; i++) {
        NSDictionary *streamDictionary = [array objectAtIndex:i];
        [streamArray addObject:[[StreamInformation alloc] init: streamDictionary]];
    }

    return [[MediaInformation alloc] init:jsonDictionary withStreams:streamArray];
}

@end
