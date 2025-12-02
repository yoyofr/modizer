//
//  SPCTagParser.h
//  modizer
//
//  SPC file ID666 and Extended ID666 (xid6) tag parser
//  https://wiki.superfamicom.org/spc-and-rsn-file-format
//

#ifndef SPCTagParser_h
#define SPCTagParser_h

#import <Foundation/Foundation.h>

// SPC tag structure (combines ID666 and Extended ID666)
// Extended tags replace basic ID666 fields when present
typedef struct {
    // Basic metadata (from ID666 or xid6 0x01-0x07)
    char *songName;            // Song title
    char *gameName;            // Game title
    char *artistName;          // Artist/composer name
    char *dumperName;          // Person who dumped the file
    char *comments;            // Comments

    // Date dumped
    int year;
    int month;
    int day;

    // Emulator info
    unsigned char emulatorID;  // 0=unknown, 1=ZSNES, 2=Snes9x

    // Extended metadata (xid6 0x10-0x1F)
    char *ostTitle;            // Official soundtrack title
    uint16_t ostDisc;          // OST disc number
    uint16_t ostTrack;         // OST track (high byte=track, low byte=subtrack)
    char *publisherName;       // Publisher name
    uint16_t copyrightYear;    // Copyright year

    // Playback info (from ID666 or xid6 0x30-0x36)
    // Note: xid6 uses ticks (1/64000th second), ID666 uses seconds/milliseconds
    uint32_t introLength;      // Introduction length (ticks)
    uint32_t loopLength;       // Loop length (ticks)
    int32_t endLength;         // End length (ticks, can be negative)
    uint32_t fadeLength;       // Fade length (ticks)

    // Voice/channel control
    uint8_t mutedVoices;       // Bit mask for muted voices
    uint8_t loopCount;         // Number of times to loop
    uint32_t ampValue;         // Amplification (65536 = 100%)

    // Format flags
    BOOL hasXID6;              // Extended tags present
    BOOL isBinaryID666;        // ID666 is binary format
} SPCTag;

@interface SPCTagParser : NSObject

// Parse SPC file tags (ID666 + xid6 if present)
+ (BOOL)parseTagsFromFile:(NSString *)filePath tag:(SPCTag *)tag;
+ (BOOL)parseTagsFromData:(NSData *)data tag:(SPCTag *)tag;

// Free dynamically allocated memory in tag
+ (void)freeTag:(SPCTag *)tag;

// Helper methods
+ (NSString *)getEmulatorName:(unsigned char)emulatorID;
+ (BOOL)isVoiceMuted:(SPCTag *)tag voice:(int)voice;

// Time conversion (xid6 uses 1/64000th second ticks)
+ (double)ticksToSeconds:(uint32_t)ticks;
+ (uint32_t)secondsToTicks:(double)seconds;

// OST track parsing (format: high byte = track, low byte = subtrack)
+ (void)parseOSTTrack:(uint16_t)value track:(int *)track subtrack:(int *)subtrack;

@end

#endif /* SPCTagParser_h */
