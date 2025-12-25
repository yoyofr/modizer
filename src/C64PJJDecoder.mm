//
//  C64PJJDecoder.mm
//  modizer
//
//  Created by Yohann Magnien David on 25/12/2025.
//

#import "C64PJJDecoder.h"

static const uint32_t C64Palette[16] = {
    0x000000FF, 0xFFFFFFFF, 0x813338FF, 0x75CEC8FF,
    0x8E3C97FF, 0x56AC4DFF, 0x2E2C9BFF, 0xEDF171FF,
    0x8E5029FF, 0x553800FF, 0xC46C71FF, 0x4A4A4AFF,
    0x7B7B7BFF, 0xA9FF9FFF, 0x706DEBFF, 0xB2B2B2FF
};

@implementation C64PJJDecoder

+ (NSData *)decompressPJJData:(NSData *)compressedData
{
    if (compressedData.length < 3) return nil;

    const uint8_t *input = (const uint8_t *)compressedData.bytes;

    // Skip 2-byte header (load address $005C)
    NSUInteger inputPos = 2;

    // PJJ files decompress to 9216 bytes
    NSMutableData *output = [NSMutableData dataWithCapacity:9216];

    // Same RLE algorithm as PGG: FE XX YY = repeat byte XX YY times
    while (inputPos < compressedData.length && output.length < 9216) {
        uint8_t byte = input[inputPos];

        if (byte == 0xFE && inputPos + 2 < compressedData.length) {
            // RLE marker: FE <value> <count>
            uint8_t value = input[inputPos + 1];
            uint8_t count = input[inputPos + 2];

            for (int i = 0; i < count && output.length < 9216; i++) {
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

+ (UIImage *)imageFromPJJData:(NSData *)data
{
    // Decompress the PJJ data first
    NSData *decompressed = [self decompressPJJData:data];

    if (!decompressed || decompressed.length < 9024) {
        return nil;
    }

    const uint8_t *bytes = (const uint8_t *)decompressed.bytes;

    // PJJ format is HIRES (1 bit/pixel)
    // Structure: screen(1024) + bitmap(8000) + padding(192)
    const uint8_t *screen = bytes;
    const uint8_t *bitmap = bytes + 1024;

    const int width  = 320;
    const int height = 200;

    uint32_t *rgba = (uint32_t *)calloc(width * height, sizeof(uint32_t));

    // HIRES mode: 1 bit per pixel, 8 pixels per byte
    // Screen RAM contains color info: upper nibble = foreground, lower nibble = background
    // Standard character organization (8 bytes per character)
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            // Calculate character position (8x8 chars)
            int charCol = x >> 3;  // x / 8
            int charRow = y >> 3;  // y / 8
            int rowInChar = y & 7;
            int colInChar = x & 7;

            // Character index in screen RAM
            int charIndex = charRow * 40 + charCol;

            // Byte index in bitmap (standard character organization)
            int byteIndex = charIndex * 8 + rowInChar;

            // Get the bit for this pixel (MSB = leftmost pixel)
            int bitPos = 7 - colInChar;
            uint8_t bit = (bitmap[byteIndex] >> bitPos) & 0x01;

            // Get colors from screen RAM
            uint8_t screenByte = screen[charIndex];
            uint8_t fgColor = (screenByte >> 4) & 0x0F;
            uint8_t bgColor = screenByte & 0x0F;

            // Select color based on bit value
            uint8_t colorIndex = bit ? fgColor : bgColor;
            uint32_t pixel = C64Palette[colorIndex];

            rgba[y * width + x] = pixel;
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
