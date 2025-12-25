//
//  C64PGGDecoder.mm
//  modizer
//
//  Created by Yohann Magnien David on 25/12/2025.
//

#import "C64PGGDecoder.h"

static const uint32_t C64Palette[16] = {
    0x000000FF, 0xFFFFFFFF, 0x813338FF, 0x75CEC8FF,
    0x8E3C97FF, 0x56AC4DFF, 0x2E2C9BFF, 0xEDF171FF,
    0x8E5029FF, 0x553800FF, 0xC46C71FF, 0x4A4A4AFF,
    0x7B7B7BFF, 0xA9FF9FFF, 0x706DEBFF, 0xB2B2B2FF
};

@implementation C64PGGDecoder

+ (NSData *)decompressPGGData:(NSData *)compressedData
{
    if (compressedData.length < 3) return nil;

    const uint8_t *input = (const uint8_t *)compressedData.bytes;

    // Skip 2-byte header (load address $0060)
    // Note: Original C64 disk files have destination address at bytes 2-3,
    // but standalone .pgg files appear to start RLE data directly at byte 2
    NSUInteger inputPos = 2;

    // Allocate buffer for decompressed data (max size for .pic format)
    NSMutableData *output = [NSMutableData dataWithCapacity:10001];

    // Decompression: FE XX YY = repeat byte XX YY times
    // Based on actual Stereo SID Player code analysis
    // Other bytes are literal
    while (inputPos < compressedData.length && output.length < 10001) {
        uint8_t byte = input[inputPos];

        if (byte == 0xFE && inputPos + 2 < compressedData.length) {
            // RLE marker: FE <value> <count>
            uint8_t value = input[inputPos + 1];
            uint8_t count = input[inputPos + 2];

            for (int i = 0; i < count && output.length < 10001; i++) {
                [output appendBytes:&value length:1];
            }

            inputPos += 3;
        } else {
            // Literal byte
            [output appendBytes:&byte length:1];
            inputPos++;
        }
    }

    return output;
}

+ (UIImage *)imageFromPGGData:(NSData *)data
{
    // Decompress the PGG data first
    NSData *decompressed = [self decompressPGGData:data];

    if (!decompressed || decompressed.length < 10001) {
        return nil;
    }

    const uint8_t *bytes = (const uint8_t *)decompressed.bytes;

    // PGG format is MULTICOLOR (2 bits/pixel), same as .pic format!
    // Structure: bitmap(8000) + screen(1000) + color(1000) + bgcolor(1)
    const uint8_t *bitmap = bytes;
    const uint8_t *screen = bytes + 8000;
    const uint8_t *color  = bytes + 9000;

    uint8_t bgColor = bytes[10000] & 0x0F;

    const int width  = 320;
    const int height = 200;

    uint32_t *rgba = (uint32_t *)calloc(width * height, sizeof(uint32_t));

    // MULTICOLOR mode: 2 bits per pixel, 4 pixels per byte
    // Standard character organization (not scan-line)
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 160; x++) {

            int cellX = x / 4;
            int charRow = y >> 3;
            int rowInChar = y & 7;

            // Standard character organization
            int charIndex = charRow * 40 + cellX;
            int byteIndex = charIndex * 8 + rowInChar;

            int shift = (3 - (x & 3)) * 2;
            uint8_t bits = (bitmap[byteIndex] >> shift) & 0x03;

            int cellY = y / 8;
            int cell  = cellY * 40 + cellX;

            uint8_t screenByte = screen[cell];
            uint8_t colorByte  = color[cell];

            uint8_t colorIndex;
            switch (bits) {
                case 0: colorIndex = bgColor; break;
                case 1: colorIndex = (screenByte >> 4) & 0x0F; break;
                case 2: colorIndex = screenByte & 0x0F; break;
                case 3: colorIndex = colorByte & 0x0F; break;
            }

            uint32_t pixel = C64Palette[colorIndex];

            int outX = x * 2;
            rgba[y * width + outX]     = pixel;
            rgba[y * width + outX + 1] = pixel;
        }
    }

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();

    CGContextRef ctx = CGBitmapContextCreate(
        rgba,
        width,
        height,
        8,
        width * 4,
        cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Little
    );

    CGImageRef cgImage = CGBitmapContextCreateImage(ctx);
    UIImage *image = [UIImage imageWithCGImage:cgImage
                                         scale:1.0
                                   orientation:UIImageOrientationUp];

    CGImageRelease(cgImage);
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
    free(rgba);

    return image;
}

@end
