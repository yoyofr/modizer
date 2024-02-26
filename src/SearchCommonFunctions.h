//
//  SearchCommonFunctions.h
//  modizer
//
//  Created by Yohann Magnien David on 25/02/2024.
//

#ifndef SearchCommonFunctions_h
#define SearchCommonFunctions_h

#include <regex.h>

static void sqlite_regexp(sqlite3_context* context, int argc, sqlite3_value** values) {
    int ret;
    regex_t regex;
    char* reg = (char*)sqlite3_value_text(values[0]);
    char* text = (char*)sqlite3_value_text(values[1]);

    if ( argc != 2 || reg == 0 || text == 0) {
        sqlite3_result_error(context, "SQL function regexp() called with invalid arguments.\n", -1);
        return;
    }

    ret = regcomp(&regex, reg, REG_EXTENDED | REG_NOSUB | REG_ICASE);
    if ( ret != 0 ) {
        sqlite3_result_error(context, "error compiling regular expression", -1);
        return;
    }

    ret = regexec(&regex, text , 0, NULL, 0);
    regfree(&regex);

    sqlite3_result_int(context, (ret != REG_NOMATCH));
}


-(bool) searchStringRegExp:(NSString*)searchPattern sourceString:(NSString*)sourceString {
    NSString *searchExpression;
    
    if (settings[GLOB_SearchRegExp].detail.mdz_boolswitch.switch_value) {
        //replace . by \. and * by .*
        searchExpression=[[searchPattern stringByReplacingOccurrencesOfString:@"." withString:@"\\."] stringByReplacingOccurrencesOfString:@"*" withString:@".*"];
    } else searchExpression=[NSString stringWithString:searchPattern];
    
    NSError *error = NULL;
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:searchExpression options:NSRegularExpressionCaseInsensitive error:&error];
    NSUInteger numberOfMatches = [regex numberOfMatchesInString:sourceString options:0 range:NSMakeRange(0, [sourceString length])];
    if (numberOfMatches) return true;
    else return false;
}

-(NSString *) convSearchRegExp:(NSString*)searchPattern {
    NSString *searchExpression;
    
    if (settings[GLOB_SearchRegExp].detail.mdz_boolswitch.switch_value) {
        //replace . by \. and * by .*
        searchExpression=[[searchPattern stringByReplacingOccurrencesOfString:@"." withString:@"\\."] stringByReplacingOccurrencesOfString:@"*" withString:@".*"];
    } else searchExpression=[NSString stringWithString:searchPattern];
    
    return searchExpression;
}


#endif /* SearchCommonFunctions_h */
