#pragma once

#include <cstdint>

#ifdef WORDS_BIGENDIAN
# if defined(__GNUC__) && defined(__ppc__)
#define READ16LE(base) \
	({ unsigned short lhbrxResult;       \
		__asm__ ("lhbrx %0, 0, %1" : "=r" (lhbrxResult) : "r" (base) : "memory"); \
		lhbrxResult; })

#define READ32LE(base) \
	({ unsigned long lwbrxResult; \
		__asm__ ("lwbrx %0, 0, %1" : "=r" (lwbrxResult) : "r" (base) : "memory"); \
		lwbrxResult; })

#define WRITE16LE(base, value) \
	__asm__ ("sthbrx %0, 0, %1" : : "r" (value), "r" (base) : "memory")

#define WRITE32LE(base, value) \
	__asm__ ("stwbrx %0, 0, %1" : : "r" (value), "r" (base) : "memory")
# else
// swaps a 16-bit value
inline uint16_t swap16(uint16_t v)
{
	return (v << 8) | (v >> 8);
}

// swaps a 32-bit value
inline uint32_t swap32(uint32_t v)
{
	return (v << 24) | ((v << 8) & 0xff0000) | ((v >> 8) & 0xff00) | (v >> 24);
}

template<typename T> inline uint16_t READ16LE(const T *x) { return swap16(*reinterpret_cast<const uint16_t *>(x)); }
template<typename T> inline uint32_t READ32LE(const T *x) { return swap32(*reinterpret_cast<const uint32_t *>(x)); }
template<typename T> inline void WRITE16LE(T *x, uint16_t v) { *reinterpret_cast<uint16_t *>(x) = swap16(v); }
template<typename T> inline void WRITE32LE(T *x, uint32_t v) { *reinterpret_cast<uint32_t *>(x) = swap32(v); }
# endif
#else
template<typename T> inline uint16_t READ16LE(const T *x) { return *reinterpret_cast<const uint16_t *>(x); }
template<typename T> inline uint32_t READ32LE(const T *x) { return *reinterpret_cast<const uint32_t *>(x); }
template<typename T> inline void WRITE16LE(T *x, uint16_t v) { *reinterpret_cast<uint16_t *>(x) = v; }
template<typename T> inline void WRITE32LE(T *x, uint32_t v) { *reinterpret_cast<uint32_t *>(x) = v; }
#endif
