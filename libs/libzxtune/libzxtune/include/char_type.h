/**
*
* @file
*
* @brief  Characters type definition
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

#if defined(UNICODE) && !defined(EMSCRIPTEN)
//! @brief Character type used for strings in unicode mode
typedef wchar_t Char;
#else
//! @brief Character type used for strings
typedef char Char;
#endif
