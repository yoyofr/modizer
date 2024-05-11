//
//  GoogleAppHelper.m
//  modizer
//
//  Created by yoyofr on 23/10/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "GoogleAppHelper.h"
#import "DBHelper.h"

typedef struct  {
    NSString *url;
    NSString *path_prefix;
} t_joshw_path;

@implementation GoogleAppHelper

+(void) SendStatistics:(NSString*)fileName path:(NSString*)filePath rating:(int)rating playcount:(int)playcount {
    int collectionType=0;
    NSRange r;
    NSString *strPath=nil;
    
    if ([filePath length]>2) {
        if (([filePath characterAtIndex:0]=='/')&&([filePath characterAtIndex:1]=='/')) {
            strPath=@"";
        }
    }
    
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/MODLAND" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            strPath=DBHelper::getFullPathFromLocalPath([filePath substringFromIndex:18]);
            if (strPath!=nil) collectionType=1;
            else { //Not found but keep path
                strPath=[filePath substringFromIndex:18];
            }
        }
    }
        
    //MODLAND?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/MODLAND" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            strPath=DBHelper::getFullPathFromLocalPath([filePath substringFromIndex:18]);
            if (strPath!=nil) collectionType=1;
            else { //Not found but keep path
                strPath=[filePath substringFromIndex:strlen("Documents/MODLAND")+1];
            }
        }
    }
    //HVSC?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/HVSC" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            collectionType=2;
            strPath=[filePath substringFromIndex:strlen("Documents/HVSC")+1];
        }
    }
    //ASMA?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/ASMA" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            collectionType=3;
            strPath=[filePath substringFromIndex:strlen("Documents/ASMA")+1];
        }
    }
    //SMSP?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/SMSP" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            collectionType=4;
            strPath=[[[[filePath substringFromIndex:strlen("Documents/SMSP")+1] lastPathComponent] stringByDeletingPathExtension] stringByReplacingOccurrencesOfString:@" " withString:@""];
        }
    }
    //SNESMusic?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/SNESM" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            collectionType=5;
            strPath=[[[filePath substringFromIndex:strlen("Documents/SNESM")+1] lastPathComponent] stringByDeletingPathExtension];
        }
    }
    //P2612?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/P2612" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            collectionType=6;
            strPath=[[[filePath substringFromIndex:strlen("Documents/P2612")+1] lastPathComponent] stringByDeletingPathExtension];
        }
    }
    //VGMRips?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/VGMRips" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            collectionType=7;
            strPath=[[[filePath substringFromIndex:strlen("Documents/VGMRips")+1] lastPathComponent] stringByDeletingPathExtension];
        }
    }
    //JoshW?
    if (strPath==nil) {
        r=[filePath rangeOfString:@"Documents/JoshW" options:NSCaseInsensitiveSearch];
        if (r.location!=NSNotFound) {
            collectionType=8;
            strPath=[filePath substringFromIndex:strlen("Documents/JoshW")+1];
            
            
            t_joshw_path josh_path[]=
            {
                //computers
                {@"pc.joshw.info",@"PC"},
                {@"cdi.joshw.info/amiga",@"Amiga"},
                {@"fmtowns.joshw.info",@"FMT"},
                {@"hoot.joshw.info",@"Hoot"},
                {@"s98.joshw.info",@"S98"},
                {@"kss.joshw.info/MSX",@"MSX"},
                //consoles
                {@"nsf.joshw.info",@"NES"},
                {@"spc.joshw.info",@"SNES"},
                {@"usf.joshw.info",@"N64"},
                {@"gcn.joshw.info",@"GC"},
                {@"wii.joshw.info",@"Wii"},
                {@"wiiu.joshw.info",@"WiiU"},
                {@"kss.joshw.info/Master%20System",@"SMS"},
                {@"smd.joshw.info",@"SMD"},
                {@"ssf.joshw.info",@"Saturn"},
                {@"dsf.joshw.info",@"DC"},
                {@"hes.joshw.info",@"PCE"},
                {@"ncd.joshw.info",@"NEOCD"},
                {@"psf.joshw.info",@"PS1"},
                {@"psf2.joshw.info",@"PS2"},
                {@"psf3.joshw.info",@"PS3"},
                {@"xbox.joshw.info",@"Xbox"},
                {@"x360.joshw.info",@"X360"},
                {@"3do.joshw.info",@"3DO"},
                {@"switch.joshw.info",@"Switch"},
                {@"cdi.joshw.info/cdi",@"CD-i"},
                {@"psf4.joshw.info",@"PS4"},
                {@"psf5.joshw.info",@"PS5"},
                {@"cdi.joshw.info/pgm",@"PGM"},
                //portables
                {@"gbs.joshw.info",@"GB"},
                {@"gsf.joshw.info",@"GBA"},
                {@"2sf.joshw.info",@"NDS"},
                {@"3sf.joshw.info",@"3DS"},
                {@"kss.joshw.info/Game%20Gear",@"SGG"},
                {@"wsr.joshw.info",@"WS"},
                {@"psp.joshw.info",@"PSP"},
                {@"vita.joshw.info",@"PSVita"},
                {@"mobile.joshw.info",@"Mobile"},
                {NULL,NULL}
            };
            NSMutableArray *arr=[NSMutableArray arrayWithArray:[strPath componentsSeparatedByString:@"/"]];
            int idx=0;
            NSString *url=NULL;
            while (josh_path[idx].url!=NULL) {
                if ([josh_path[idx].path_prefix isEqualToString:(NSString*)[arr firstObject]]) {
                    url=josh_path[idx].url;
                    break;
                }
                idx++;
            }
            
            if (url) {
                [arr removeObjectAtIndex:0];
                char letter=tolower([(NSString *)[arr lastObject] characterAtIndex:0]);
                strPath=[NSString stringWithFormat:@"%@/%c/%@",url,letter,[arr componentsJoinedByString:@"/"]];
            }
        }
    }
    
    if (strPath==nil) {
        //Remove the Documents/ and keep the rest
        strPath=[filePath substringFromIndex:10];
    }
    NSString *identifier;
    if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)]) {
        identifier=[[[UIDevice currentDevice] identifierForVendor] UUIDString];
    } else {
        identifier=@"unknwown";//[[UIDevice currentDevice] uniqueIdentifier];
    }
    NSString *urlString=[NSString stringWithFormat:@"%@/Stats?Name=%@&Path=%@&Rating=%d&Played=%d&UID=%@&Type=%d&VersionMajor=%s&VersionMinor=%s",
                         STATISTICS_URL,
                         [[fileName stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] stringByReplacingOccurrencesOfString:@"&" withString:@"$$"],
                         [[[strPath stringByAddingPercentEscapesUsingEncoding:NSISOLatin1StringEncoding] stringByReplacingOccurrencesOfString:@"%" withString:@"//"] stringByReplacingOccurrencesOfString:@"&" withString:@"$$"],
                         rating,playcount,identifier,
                         collectionType,
                         VERSION_MAJOR_STR,
                         VERSION_MINOR_STR
    ];
    NSURL *url=[NSURL URLWithString:urlString];
    //NSLog(@"%@",[url absoluteString]);
    
    AFHTTPSessionManager *manager = [AFHTTPSessionManager manager];
    manager.responseSerializer.acceptableContentTypes=[NSSet setWithObject:@"application/json"];
    [manager GET:urlString parameters:nil headers:nil progress:nil success:^(NSURLSessionDataTask * _Nonnull task, id  _Nullable responseObject) {
        //NSLog(@"JSON: %@", responseObject);
    } failure:^(NSURLSessionDataTask * _Nullable task, NSError * _Nonnull error) {
        NSLog(@"Error: %@", error);
    }];
    [manager invalidateSessionCancelingTasks:NO resetSession:NO];
    
}


@end
