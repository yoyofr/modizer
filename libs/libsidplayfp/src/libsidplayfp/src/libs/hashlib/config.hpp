#ifndef HASHLIB_ALL_IN_ONE
#pragma once
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#endif

#define HASHLIB_VERSION "1.1.1"

#ifdef _MSVC_LANG
#define HASHLIB_CXX_STANDARD _MSVC_LANG
#else
#define HASHLIB_CXX_STANDARD __cplusplus
#endif

#define HASHLIB_CXX_STD98 199711L
#define HASHLIB_CXX_STD11 201103L
#define HASHLIB_CXX_STD14 201402L
#define HASHLIB_CXX_STD17 201703L
#define HASHLIB_CXX_STD20 202002L
#define HASHLIB_CXX_STD23 202302L

#if HASHLIB_CXX_STANDARD < HASHLIB_CXX_STD11
#error "hashlib requires a C++11 compiler."
#endif

#ifdef __clang__
#define HASHLIB_CXX_COMPILER_CLANG 1
#define HASHLIB_CXX_COMPILER_GCC 0
#define HASHLIB_CXX_COMPILER_MSVC 0
#elif defined(__GNUC__)
#define HASHLIB_CXX_COMPILER_CLANG 0
#define HASHLIB_CXX_COMPILER_GCC 1
#define HASHLIB_CXX_COMPILER_MSVC 0
#elif defined(_MSC_VER)
#define HASHLIB_CXX_COMPILER_CLANG 0
#define HASHLIB_CXX_COMPILER_GCC 0
#define HASHLIB_CXX_COMPILER_MSVC 1
#else
#error "hashlib requires the C++ compiler is clang, gcc or msvc."
#endif

#if HASHLIB_CXX_COMPILER_CLANG
#define HASHLIB_ALWAYS_INLINE [[clang::always_inline]]
#elif HASHLIB_CXX_COMPILER_GCC
#define HASHLIB_ALWAYS_INLINE [[gnu::always_inline]]
#elif HASHLIB_CXX_COMPILER_MSVC
#define HASHLIB_ALWAYS_INLINE [[msvc::forceinline]]
#endif

#ifdef HASHLIB_BUILD_MODULE
#define HASHLIB_MOD_EXPORT export
#define HASHLIB_MOD_EXPORT_BEGIN export {
#define HASHLIB_MOD_EXPORT_END }
#else
#define HASHLIB_MOD_EXPORT
#define HASHLIB_MOD_EXPORT_BEGIN
#define HASHLIB_MOD_EXPORT_END
#endif

#if HASHLIB_CXX_STANDARD >= HASHLIB_CXX_STD17
#define HASHLIB_CXX17_CONSTEXPR constexpr
#else
#define HASHLIB_CXX17_CONSTEXPR
#endif

#if HASHLIB_CXX_STANDARD >= HASHLIB_CXX_STD20
#define HASHLIB_CXX20_CONSTEXPR constexpr
#else
#define HASHLIB_CXX20_CONSTEXPR
#endif

#if HASHLIB_CXX_STANDARD >= HASHLIB_CXX_STD17
#define HASHLIB_CXX17_INLINE inline
#else
#define HASHLIB_CXX17_INLINE
#endif

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(nodiscard)
#define HASHLIB_NODISCARD [[nodiscard]]
#else
#define HASHLIB_NODISCARD
#endif
#else
#define HASHLIB_NODISCARD
#endif