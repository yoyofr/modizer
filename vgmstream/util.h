/*
 * util.h - utility functions
 */

#include "streamtypes.h"

#ifndef _UTIL_H
#define _UTIL_H

/* very common functions, so static (inline) in .h as compiler can optimize to avoid some call overhead */

/* host endian independent multi-byte integer reading */

static inline int16_t get_16bitBE(const uint8_t* p) {
    return (p[0]<<8) | (p[1]);
}

static inline int16_t get_16bitLE(const uint8_t* p) {
    return (p[0]) | (p[1]<<8);
}

static inline int32_t get_32bitBE(const uint8_t* p) {
    return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | (p[3]);
}

static inline int32_t get_32bitLE(const uint8_t* p) {
    return (p[0]) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

static inline int64_t get_64bitBE(const uint8_t* p) {
    return (uint64_t)(((uint64_t)p[0]<<56) | ((uint64_t)p[1]<<48) | ((uint64_t)p[2]<<40) | ((uint64_t)p[3]<<32) | ((uint64_t)p[4]<<24) | ((uint64_t)p[5]<<16) | ((uint64_t)p[6]<<8) | ((uint64_t)p[7]));
}

static inline int64_t get_64bitLE(const uint8_t* p) {
    return (uint64_t)(((uint64_t)p[0]) | ((uint64_t)p[1]<<8) | ((uint64_t)p[2]<<16) | ((uint64_t)p[3]<<24) | ((uint64_t)p[4]<<32) | ((uint64_t)p[5]<<40) | ((uint64_t)p[6]<<48) | ((uint64_t)p[7]<<56));
}

/* alias of the above */
static inline  int8_t  get_s8   (const uint8_t* p) { return ( int8_t)p[0]; }
static inline uint8_t  get_u8   (const uint8_t* p) { return (uint8_t)p[0]; }
static inline  int16_t get_s16le(const uint8_t* p) { return ( int16_t)get_16bitLE(p); }
static inline uint16_t get_u16le(const uint8_t* p) { return (uint16_t)get_16bitLE(p); }
static inline  int16_t get_s16be(const uint8_t* p) { return ( int16_t)get_16bitBE(p); }
static inline uint16_t get_u16be(const uint8_t* p) { return (uint16_t)get_16bitBE(p); }
static inline  int32_t get_s32le(const uint8_t* p) { return ( int32_t)get_32bitLE(p); }
static inline uint32_t get_u32le(const uint8_t* p) { return (uint32_t)get_32bitLE(p); }
static inline  int32_t get_s32be(const uint8_t* p) { return ( int32_t)get_32bitBE(p); }
static inline uint32_t get_u32be(const uint8_t* p) { return (uint32_t)get_32bitBE(p); }
static inline  int64_t get_s64le(const uint8_t* p) { return ( int64_t)get_64bitLE(p); }
static inline uint64_t get_u64le(const uint8_t* p) { return (uint64_t)get_64bitLE(p); }
static inline  int64_t get_s64be(const uint8_t* p) { return ( int64_t)get_64bitBE(p); }
static inline uint64_t get_u64be(const uint8_t* p) { return (uint64_t)get_64bitBE(p); }

void put_8bit(uint8_t* buf, int8_t i);

void put_16bitLE(uint8_t* buf, int16_t i);

void put_32bitLE(uint8_t* buf, int32_t i);

void put_16bitBE(uint8_t* buf, int16_t i);

void put_32bitBE(uint8_t* buf, int32_t i);

/* alias of the above */ //TODO: improve
#define put_u8 put_8bit
#define put_u16le put_16bitLE
#define put_u32le put_32bitLE
#define put_u16be put_16bitBE
#define put_u32be put_32bitBE
#define put_s8 put_8bit
#define put_s16le put_16bitLE
#define put_s32le put_32bitLE
#define put_s16be put_16bitBE
#define put_s32be put_32bitBE


/* signed nibbles come up a lot */
static int nibble_to_int[16] = {0,1,2,3,4,5,6,7,-8,-7,-6,-5,-4,-3,-2,-1};

static inline int get_nibble_signed(uint8_t n, int upper) {
    /*return ((n&0x70)-(n&0x80))>>4;*/
    return nibble_to_int[(n >> (upper?4:0)) & 0x0f];
}

static inline int get_high_nibble_signed(uint8_t n) {
    /*return ((n&0x70)-(n&0x80))>>4;*/
    return nibble_to_int[n>>4];
}

static inline int get_low_nibble_signed(uint8_t n) {
    /*return (n&7)-(n&8);*/
    return nibble_to_int[n&0xf];
}

static inline int clamp16(int32_t val) {
    if (val > 32767) return 32767;
    else if (val < -32768) return -32768;
    else return val;
}


/* transforms a string to uint32 (for comparison), but if this is static + all goes well
 * compiler should pre-calculate and use uint32 directly */
static inline /*const*/ uint32_t get_id32be(const char* s) {
    return (uint32_t)(s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3] << 0);
}

//static inline /*const*/ uint32_t get_id32le(const char* s) {
//    return (uint32_t)(s[0] << 0) | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
//}

static inline /*const*/ uint64_t get_id64be(const char* s) {
    return (uint64_t)(
            ((uint64_t)s[0] << 56) |
            ((uint64_t)s[1] << 48) |
            ((uint64_t)s[2] << 40) |
            ((uint64_t)s[3] << 32) |
            ((uint64_t)s[4] << 24) |
            ((uint64_t)s[5] << 16) |
            ((uint64_t)s[6] << 8) |
            ((uint64_t)s[7] << 0)
    );
}


/* less common functions, no need to inline */

int round10(int val);

/* return a file's extension (a pointer to the first character of the
 * extension in the original filename or the ending null byte if no extension */
const char * filename_extension(const char * filename);

/* swap samples in machine endianness to little endian (useful to write .wav) */
void swap_samples_le(sample_t *buf, int count);

void concatn(int length, char * dst, const char * src);


/* Simple stdout logging for debugging and regression testing purposes.
 * Needs C99 variadic macros, uses do..while to force ";" as statement */
#ifdef VGM_DEBUG_OUTPUT

/* equivalent to printf when condition is true */
#define VGM_ASSERT(condition, ...) \
    do { if (condition) {printf(__VA_ARGS__);} } while (0)
#define VGM_ASSERT_ONCE(condition, ...) \
    do { static int written; if (!written) { if (condition) {printf(__VA_ARGS__); written = 1;} }  } while (0)
/* equivalent to printf */
#define VGM_LOG(...) \
    do { printf(__VA_ARGS__); } while (0)
#define VGM_LOG_ONCE(...) \
    do { static int written; if (!written) { printf(__VA_ARGS__); written = 1; } } while (0)
/* prints file/line/func */
#define VGM_LOGF() \
    do { printf("%s:%i '%s'\n",  __FILE__, __LINE__, __func__); } while (0)
/* prints to a file */
#define VGM_LOGT(txt, ...) \
    do { FILE *fl = fopen(txt,"a+"); if(fl){fprintf(fl,__VA_ARGS__); fflush(fl);} fclose(fl); } while(0)
/* prints a buffer/array */
#define VGM_LOGB(buf, buf_size, bytes_per_line) \
    do { \
        int i; \
        for (i=0; i < buf_size; i++) { \
            printf("%02x",buf[i]); \
            if (bytes_per_line && (i+1) % bytes_per_line == 0) printf("\n"); \
        } \
        printf("\n"); \
    } while (0)

#else/*VGM_DEBUG_OUTPUT*/

#define VGM_ASSERT(condition, ...) /* nothing */
#define VGM_ASSERT_ONCE(condition, ...) /* nothing */
#define VGM_LOG(...) /* nothing */
#define VGM_LOG_ONCE(...) /* nothing */
#define VGM_LOGF() /* nothing */
#define VGM_LOGT() /* nothing */
#define VGM_LOGB(buf, buf_size, bytes_per_line) /* nothing */


#endif/*VGM_DEBUG_OUTPUT*/

#endif
