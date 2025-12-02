//
//  SPCTagParser.mm
//  modizer
//
//  SPC file tag parser implementation
//

#import "SPCTagParser.h"
#include <string.h>

// SPC file format constants
#define SPC_HEADER_SIZE 33
#define SPC_ID666_OFFSET 0x2E
#define SPC_MAGIC "SNES-SPC700 Sound File Data v0.30"
#define SPC_MIN_SIZE 0x10200

// ID666 field offsets (relative to 0x2E)
#define ID666_SONG_TITLE    0
#define ID666_GAME_TITLE    32
#define ID666_DUMPER_NAME   64
#define ID666_COMMENTS      80
#define ID666_DATE          112
#define ID666_SECONDS       123
#define ID666_FADE          126  // Text: 126 (5 bytes), Binary: 126 (4 bytes)
#define ID666_ARTIST        131
#define ID666_CHANNEL       163
#define ID666_EMULATOR      164

// xid6 constants
#define XID6_MAGIC "xid6"
#define TICKS_PER_SECOND 64000

@implementation SPCTagParser

#pragma mark - Helper Functions

static char* copyString(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    if (len == 0) return NULL;
    char *dst = (char *)malloc(len + 1);
    if (dst) strcpy(dst, src);
    return dst;
}

static void trimString(char *str, size_t maxLen) {
    for (int i = (int)maxLen - 1; i >= 0; i--) {
        if (str[i] == ' ' || str[i] == '\0') {
            str[i] = '\0';
        } else {
            break;
        }
    }
}

static void extractString(char *dest, const unsigned char *src, size_t len) {
    memcpy(dest, src, len);
    dest[len] = '\0';
    trimString(dest, len);
}

static int parseNumericString(const unsigned char *data, size_t maxLen) {
    char temp[16] = {0};
    size_t len = (maxLen < sizeof(temp) - 1) ? maxLen : sizeof(temp) - 1;
    memcpy(temp, data, len);
    return atoi(temp);
}

static BOOL detectBinaryFormat(const unsigned char *id666Data) {
    // Text format has '/' at positions 2 and 5 in date field
    if (id666Data[ID666_DATE + 2] == '/' && id666Data[ID666_DATE + 5] == '/') {
        return NO;
    }
    // Binary format has 7 unused zero bytes after the 4-byte date
    for (int i = 0; i < 7; i++) {
        if (id666Data[ID666_DATE + 4 + i] != 0) {
            return YES; // Non-zero in unused area suggests text format
        }
    }
    return YES;
}

static void parseTextDate(const unsigned char *data, int *month, int *day, int *year) {
    char temp[12] = {0};
    memcpy(temp, data, 11);

    if (strlen(temp) >= 10 && temp[2] == '/' && temp[5] == '/') {
        *month = (temp[0] - '0') * 10 + (temp[1] - '0');
        *day = (temp[3] - '0') * 10 + (temp[4] - '0');
        *year = (temp[6] - '0') * 1000 + (temp[7] - '0') * 100 +
                (temp[8] - '0') * 10 + (temp[9] - '0');
    } else {
        *month = *day = *year = 0;
    }
}

static void parseBinaryDate(const unsigned char *data, int *month, int *day, int *year) {
    uint32_t date;
    memcpy(&date, data, 4);

    if (date > 0) {
        *year = date / 10000;
        *month = (date / 100) % 100;
        *day = date % 100;
    } else {
        *month = *day = *year = 0;
    }
}

#pragma mark - ID666 Parsing

static BOOL parseID666(const unsigned char *bytes, SPCTag *tag) {
    const unsigned char *id666 = bytes + SPC_ID666_OFFSET;

    // Detect format
    tag->isBinaryID666 = detectBinaryFormat(id666);

    // Extract strings
    char temp[256];

    extractString(temp, &id666[ID666_SONG_TITLE], 32);
    if (strlen(temp) > 0) tag->songName = copyString(temp);

    extractString(temp, &id666[ID666_GAME_TITLE], 32);
    if (strlen(temp) > 0) tag->gameName = copyString(temp);

    extractString(temp, &id666[ID666_DUMPER_NAME], 16);
    if (strlen(temp) > 0) tag->dumperName = copyString(temp);

    extractString(temp, &id666[ID666_COMMENTS], 32);
    if (strlen(temp) > 0) tag->comments = copyString(temp);

    extractString(temp, &id666[ID666_ARTIST], 32);
    if (strlen(temp) > 0) tag->artistName = copyString(temp);

    // Parse date
    if (tag->isBinaryID666) {
        parseBinaryDate(&id666[ID666_DATE], &tag->month, &tag->day, &tag->year);
    } else {
        parseTextDate(&id666[ID666_DATE], &tag->month, &tag->day, &tag->year);
    }

    // Parse playback time (convert to ticks)
    int seconds = parseNumericString(&id666[ID666_SECONDS], 3);
    tag->loopLength = seconds * TICKS_PER_SECOND;

    int fadeMs = parseNumericString(&id666[ID666_FADE], tag->isBinaryID666 ? 4 : 5);
    tag->fadeLength = (fadeMs * TICKS_PER_SECOND) / 1000;

    // Channel/emulator info
    tag->mutedVoices = id666[ID666_CHANNEL];
    tag->emulatorID = id666[ID666_EMULATOR];

    return YES;
}

#pragma mark - xid6 Parsing

static const unsigned char* findXID6Chunk(NSData *data) {
    const unsigned char *bytes = (const unsigned char *)[data bytes];
    NSUInteger len = [data length];

    if (len < SPC_MIN_SIZE + 8) return NULL;

    // xid6 chunk is at the end of file
    // Search backwards for "xid6" signature
    for (NSInteger i = len - 8; i >= SPC_MIN_SIZE; i--) {
        if (memcmp(&bytes[i], XID6_MAGIC, 4) == 0) {
            return &bytes[i];
        }
    }

    return NULL;
}

static void parseXID6Chunk(const unsigned char *xid6Data, NSUInteger maxLen, SPCTag *tag) {
    if (maxLen < 8) return;

    // Read chunk size
    uint32_t chunkSize;
    memcpy(&chunkSize, xid6Data + 4, 4);

    if (chunkSize > maxLen - 8) return;

    // Parse sub-chunks
    const unsigned char *data = xid6Data + 8;
    const unsigned char *end = data + chunkSize;

    while (data + 4 <= end) {
        uint8_t id = data[0];
        uint8_t type = data[1];
        uint16_t value16;
        memcpy(&value16, data + 2, 2);

        data += 4;

        if (type == 0x00) {
            // Data type - value is in the header
            switch (id) {
                case 0x11: tag->ostDisc = value16; break;
                case 0x12: tag->ostTrack = value16; break;
                case 0x14: tag->copyrightYear = value16; break;
                case 0x35: tag->loopCount = (uint8_t)value16; break;
            }
        }
        else if (type == 0x01) {
            // String type - value16 is length
            uint16_t strLen = value16;
            if (data + strLen > end) break;

            char *str = (char *)malloc(strLen + 1);
            if (str) {
                memcpy(str, data, strLen);
                str[strLen] = '\0';

                switch (id) {
                    case 0x01: free(tag->songName); tag->songName = str; break;
                    case 0x02: free(tag->gameName); tag->gameName = str; break;
                    case 0x03: free(tag->artistName); tag->artistName = str; break;
                    case 0x04: free(tag->dumperName); tag->dumperName = str; break;
                    case 0x07: free(tag->comments); tag->comments = str; break;
                    case 0x10: tag->ostTitle = str; break;
                    case 0x13: tag->publisherName = str; break;
                    default: free(str); break;
                }
            }

            // Advance and align to 4-byte boundary
            data += strLen;
            if (strLen % 4) data += 4 - (strLen % 4);
        }
        else if (type == 0x04) {
            // Integer type - value16 is length (should be 4)
            if (value16 == 4 && data + 4 <= end) {
                uint32_t val32;
                memcpy(&val32, data, 4);

                switch (id) {
                    case 0x05: { // Date dumped (YYYYMMDD)
                        tag->year = val32 / 10000;
                        tag->month = (val32 / 100) % 100;
                        tag->day = val32 % 100;
                        break;
                    }
                    case 0x30: tag->introLength = val32; break;
                    case 0x31: tag->loopLength = val32; break;
                    case 0x32: tag->endLength = (int32_t)val32; break;
                    case 0x33: tag->fadeLength = val32; break;
                    case 0x36: tag->ampValue = val32; break;
                }

                data += 4;
            }
        }
    }
}

#pragma mark - Public Methods

+ (BOOL)parseTagsFromFile:(NSString *)filePath tag:(SPCTag *)tag {
    if (!filePath || !tag) return NO;

    NSData *data = [NSData dataWithContentsOfFile:filePath];
    if (!data) return NO;

    return [self parseTagsFromData:data tag:tag];
}

+ (BOOL)parseTagsFromData:(NSData *)data tag:(SPCTag *)tag {
    if (!data || !tag) return NO;

    NSUInteger len = [data length];
    if (len < SPC_ID666_OFFSET + 165) return NO;

    const unsigned char *bytes = (const unsigned char *)[data bytes];

    // Verify SPC header
    if (memcmp(bytes, SPC_MAGIC, strlen(SPC_MAGIC)) != 0) {
        return NO;
    }

    // Clear tag structure
    memset(tag, 0, sizeof(SPCTag));

    // Parse basic ID666 tags
    parseID666(bytes, tag);

    // Look for Extended ID666 (xid6)
    const unsigned char *xid6 = findXID6Chunk(data);
    if (xid6) {
        tag->hasXID6 = YES;
        NSUInteger remaining = len - (xid6 - bytes);
        parseXID6Chunk(xid6, remaining, tag);
    }

    return YES;
}

+ (void)freeTag:(SPCTag *)tag {
    if (!tag) return;

    free(tag->songName);
    free(tag->gameName);
    free(tag->artistName);
    free(tag->dumperName);
    free(tag->comments);
    free(tag->ostTitle);
    free(tag->publisherName);

    memset(tag, 0, sizeof(SPCTag));
}

+ (NSString *)getEmulatorName:(unsigned char)emulatorID {
    switch (emulatorID) {
        case 1: return @"ZSNES";
        case 2: return @"Snes9x";
        default: return @"Unknown";
    }
}

+ (BOOL)isVoiceMuted:(SPCTag *)tag voice:(int)voice {
    if (!tag || voice < 0 || voice >= 8) return NO;
    return (tag->mutedVoices & (1 << voice)) != 0;
}

+ (double)ticksToSeconds:(uint32_t)ticks {
    return (double)ticks / TICKS_PER_SECOND;
}

+ (uint32_t)secondsToTicks:(double)seconds {
    return (uint32_t)(seconds * TICKS_PER_SECOND);
}

+ (void)parseOSTTrack:(uint16_t)value track:(int *)track subtrack:(int *)subtrack {
    if (track) *track = (value >> 8) & 0xFF;
    if (subtrack) *subtrack = value & 0xFF;
}

@end
