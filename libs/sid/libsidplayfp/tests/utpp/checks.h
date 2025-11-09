#pragma once
/*
  UTPP - A New Generation of UnitTest++
  (c) Mircea Neacsu 2017-2024

  See LICENSE file for full copyright information.
*/

/*!
  \file checks.h
  \brief Definition of Check... template functions and 
  CHECK_... macro-definitions
*/

/// \cond
#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
/// \endcond

#include <sstream>
#include <vector>
#include <array>
#include <list>
#include <string>
#include <cmath>
#include <cstring>

/*!
  \ingroup checks
@{
*/

/*!
  \def CHECK
  \brief Generate a failure if value is 0. Failure message is the value itself.

  \hideinitializer
*/
#ifdef CHECK
#error Macro CHECK is already defined
#endif
#define CHECK(value)                                                          \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      if (!UnitTest::Check(value))                                            \
        UnitTest::ReportFailure (__FILE__, __LINE__, "Check failed: " #value);\
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK(" #value ")");                          \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_EX
  \brief Generate a failure with the given message if value is 0

  \hideinitializer
*/
#ifdef CHECK_EX
#error Macro CHECK_EX is already defined
#endif
#define CHECK_EX(value, ...)                                                  \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      if (!UnitTest::Check(value)){                                           \
        char message[UnitTest::MAX_MESSAGE_SIZE];                             \
        snprintf (message, UnitTest::MAX_MESSAGE_SIZE, __VA_ARGS__);          \
        UnitTest::ReportFailure (__FILE__, __LINE__, message);                \
      }                                                                       \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_EX(" #value ")");                       \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_EQUAL
  \brief Generate a failure if actual value is different from expected.

  \hideinitializer
*/
#ifdef CHECK_EQUAL
#error Macro CHECK_EQUAL is already defined
#endif

#define CHECK_EQUAL(expected, actual)                                         \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckEqual((expected), (actual), str__))                 \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_EQUAL(" #expected ", " #actual ")");    \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_NAN
  \brief Generate a failure if value is not NaN

  \hideinitializer
*/
#ifdef CHECK_NAN
#error Macro CHECK_NAN is already defined
#endif

#define CHECK_NAN(value)                                                      \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      if (!UnitTest::CheckNaN(value))                                         \
        UnitTest::ReportFailure (__FILE__, __LINE__, "Check failed: " #value " is not NaN");\
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_NAN(" #value ")");                      \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_EQUAL_EX
  \brief  Generate a failure if actual value is different from expected.
          The given message is appended to the standard CHECK_EQUAL message. 

  \hideinitializer
*/
#ifdef CHECK_EQUAL_EX
#error Macro CHECK_EQUAL_EX is already defined
#endif
#define CHECK_EQUAL_EX(expected, actual, ...)                                 \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckEqual((expected), (actual), str__))                 \
      {                                                                       \
        char message[UnitTest::MAX_MESSAGE_SIZE];                             \
        snprintf (message, UnitTest::MAX_MESSAGE_SIZE, __VA_ARGS__);          \
        str__ += " - ";                                                       \
        str__ += message;                                                     \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
      }                                                                       \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_EQUAL_EX(" #expected ", " #actual ")"); \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_CLOSE
  \brief  Generate a failure if actual value differs from expected value with
          more than tolerance

  \hideinitializer
*/

#ifdef CHECK_CLOSE
#error Macro CHECK_CLOSE is already defined
#endif
#define CHECK_CLOSE(expected, actual,...)                                     \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckClose ((expected), (actual), (__VA_ARGS__+0), str__)) \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (UnitTest::tolerance_not_set&)                                      \
    {                                                                         \
      throw UnitTest::test_abort (__FILE__, __LINE__, "UnitTest::default_tolerance not set"); \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_CLOSE(" #expected ", " #actual ")");    \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_CLOSE_EX
  \brief  Generate a failure if actual value differs from expected value with
          more than given tolerance.
          The given message is appended to the standard CHECK_CLOSE message.
  \hideinitializer
*/

#ifdef CHECK_CLOSE_EX
#error Macro CHECK_CLOSE_EX is already defined
#endif
#define CHECK_CLOSE_EX(expected, actual, tolerance, ...)                      \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckClose ((expected), (actual), (tolerance), str__))   \
      {                                                                       \
        char message[UnitTest::MAX_MESSAGE_SIZE];                             \
        snprintf (message, UnitTest::MAX_MESSAGE_SIZE, __VA_ARGS__);          \
        str__ += " - ";                                                       \
        str__ += message;                                                     \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
      }                                                                       \
    }                                                                         \
    catch (UnitTest::tolerance_not_set&)                                      \
    {                                                                         \
      throw UnitTest::test_abort (__FILE__, __LINE__, "UnitTest::default_tolerance not set"); \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_CLOSE_EX(" #expected ", " #actual ")"); \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_ARRAY_EQUAL
  \brief Generate a failure if actual array is different from expected one

  \hideinitializer
*/
#ifdef CHECK_ARRAY_EQUAL
#error Macro CHECK_ARRAY_EQUAL is already defined
#endif

#define CHECK_ARRAY_EQUAL(expected, actual, count) \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckArrayEqual ((expected), (actual), (count), str__))  \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
       "Unhandled exception in CHECK_ARRAY_EQUAL(" #expected ", " #actual ")"); \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_ARRAY_CLOSE
  \brief  Generate a failure if `actual` array elements differs from `expected`
          ones with more than given `tolerance`.

  \hideinitializer
*/
#ifdef CHECK_ARRAY_CLOSE
#error Macro CHECK_ARRAY_CLOSE is already defined
#endif

#define CHECK_ARRAY_CLOSE(expected, actual, count, ...)                       \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckArrayClose ((expected), (actual), (count), (__VA_ARGS__+0), str__)) \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (UnitTest::tolerance_not_set&)                                      \
    {                                                                         \
      throw UnitTest::test_abort (__FILE__, __LINE__, "UnitTest::default_tolerance not set"); \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_ARRAY_CLOSE(" #expected ", " #actual ")"); \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_ARRAY2D_EQUAL
  \brief  Generate a failure if 2D array `actual` differs from `expected` values.

  \hideinitializer
*/

#ifdef CHECK_ARRAY2D_EQUAL
#error Macro CHECK_ARRAY2D_EQUAL is already defined
#endif

#define CHECK_ARRAY2D_EQUAL(expected, actual, rows, columns)                  \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckArray2DEqual ((expected), (actual), (rows), (columns), str__)) \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_ARRAY2D_EQUAL(" #expected ", " #actual ")"); \
    }                                                                         \
  } while (0)


/*!
  \def CHECK_ARRAY2D_CLOSE
  \brief  Generate a failure if 2D array `actual` differs from `expected` values
  with more than given `tolerance`.

  \hideinitializer
*/
#ifdef CHECK_ARRAY2D_CLOSE
#error Macro CHECK_ARRAY2D_CLOSE is already defined
#endif

#define CHECK_ARRAY2D_CLOSE(expected, actual, rows, columns, ...)             \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckArray2DClose (expected, actual, rows, columns, (__VA_ARGS__+0), str__)) \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (UnitTest::tolerance_not_set&)                                      \
    {                                                                         \
      throw UnitTest::test_abort (__FILE__, __LINE__, "UnitTest::default_tolerance not set"); \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_ARRAY2D_CLOSE(" #expected ", " #actual ")"); \
    }                                                                         \
  } while (0)

/*!
  \def CHECK_THROW
  \brief  Generate a failure if evaluating the expression __does not__ throw
          an exception of the `expected` type.
  \param except   Expected exception type
  \param expr     Expression to evaluate
  \hideinitializer
*/
#ifdef CHECK_THROW
#error Macro CHECK_THROW is already defined
#endif
#define CHECK_THROW(expr, except) \
  do                                                                          \
  {                                                                           \
    bool caught_ = false;                                                     \
    try { (expr); }                                                           \
    catch (const except& ) { caught_ = true; }                                \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unexpected exception in CHECK_THROW");                               \
    }                                                                         \
    if (!caught_)                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Expected exception: \"" #except "\", not thrown");                   \
  } while(0)

/*!
  \def CHECK_THROW_EX
  \brief  Generate a failure if evaluating the expression __does not__ throw
          an exception of the `expected` type.

  Appends a printf type string to standard failure message.
  \hideinitializer
*/
#ifdef CHECK_THROW_EX
#error Macro CHECK_THROW_EX is already defined
#endif
#define CHECK_THROW_EX(expr, except, ...) \
  do                                                                          \
  {                                                                           \
    bool caught_ = false;                                                     \
    try { (expr); }                                                           \
    catch (const except& ) { caught_ = true; }                                \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unexpected exception in CHECK_THROW_EX");                            \
    }                                                                         \
    if (!caught_) {                                                           \
      std::string str__{"Expected exception: \"" #except "\", not thrown"};   \
      char message[UnitTest::MAX_MESSAGE_SIZE];                               \
      snprintf (message, UnitTest::MAX_MESSAGE_SIZE, __VA_ARGS__);            \
      str__ += " - ";                                                         \
      str__ += message;                                                       \
      UnitTest::ReportFailure (__FILE__, __LINE__, str__);                    \
    }                                                                         \
  } while(0)

/*!
  \def CHECK_THROW_EQUAL
  \brief  Checks if evaluating the expression triggers an exception of the given
          type and with the expected value.

  \hideinitializer
*/

#ifdef CHECK_THROW_EQUAL
#error Macro CHECK_THROW_EQUAL is already defined
#endif
#define CHECK_THROW_EQUAL(expression, value, except)                          \
  do                                                                          \
  {                                                                           \
    bool caught_ = false;                                                     \
    try { expression; }                                                       \
    catch (const except& actual) {                                            \
      caught_ = true;                                                         \
      std::string str__;                                                      \
      if (!UnitTest::CheckEqual(value, actual, str__))                        \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unexpected exception in CHECK_THROW_EQUAL");                         \
    }                                                                         \
    if (!caught_)                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Expected exception: \"" #except "\", not thrown");                   \
  } while(0)

/*!
  \def CHECK_THROW_EQUAL_EX
  \brief  Checks if evaluating the expression triggers an exception of the given
          type and with the expected value.

  Appends a printf type string to standard failure message.
  \hideinitializer
*/

#ifdef CHECK_THROW_EQUAL_EX
#error Macro CHECK_THROW_EQUAL_EX is already defined
#endif
#define CHECK_THROW_EQUAL_EX(expression, value, except, ...)                  \
  do                                                                          \
  {                                                                           \
    bool caught_ = false;                                                     \
    try { expression; }                                                       \
    catch (const except& actual) {                                            \
      caught_ = true;                                                         \
      std::string str__;                                                      \
      if (!UnitTest::CheckEqual(value, actual, str__))                        \
      {                                                                       \
        char message[UnitTest::MAX_MESSAGE_SIZE];                             \
        snprintf (message, UnitTest::MAX_MESSAGE_SIZE, __VA_ARGS__);          \
        str__ += " - ";                                                       \
        str__ += message;                                                     \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
      }                                                                       \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unexpected exception in CHECK_THROW_EQUAL");                         \
    }                                                                         \
    if (!caught_)                                                             \
    {                                                                         \
      std::string str__{ "Expected exception: \"" #except "\", not thrown" }; \
      char message[UnitTest::MAX_MESSAGE_SIZE];                               \
      snprintf (message, UnitTest::MAX_MESSAGE_SIZE, __VA_ARGS__);            \
      str__ += " - ";                                                         \
      str__ += message;                                                       \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Expected exception: \"" #except "\", not thrown");                   \
    }                                                                         \
  } while(0)

/*!
  \def CHECK_FILE_EQUAL
  \brief  Generate a failure if the two files are different.

  \hideinitializer
*/
#ifdef CHECK_FILE_EQUAL
#error Macro CHECK_FILE_EQUAL is already defined
#endif
#define CHECK_FILE_EQUAL(expected, actual)                                    \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (!UnitTest::CheckFileEqual((expected), (actual), str__))             \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
        "Unhandled exception in CHECK_EQUAL(" #expected ", " #actual ")");    \
    }                                                                         \
  } while (0)

///@}

namespace UnitTest {

#if _MSVC_LANG < 201703L
/// Default tolerance for CLOSE... macros
extern double default_tolerance;
#else
/// Default tolerance for CLOSE... macros
inline double default_tolerance = 0;
#endif

//------------------ Check functions -----------------------------------------

/*!
  Check if value is true (or not 0)

  \param value object to check. Must have convertible to bool
  \return `true` if successful
*/ 

template <typename Value>
bool Check (Value const value)
{
  return (bool)value;
}

/*!
  Check if value is a NaN

  \param value object to check. Must be a floating point type
  \return `true` if object is a NaN
*/
template <typename Value>
bool CheckNaN (Value const value)
{
  return std::isnan(value);
}

/*!
  Check if two values are equal. If not, generate a failure message.

  \param expected - expected value
  \param actual   - actual value
  \param msg      - generated error message
  \return `true` if values compare as equal
@{
*/
template <typename expected_T, typename actual_T>
bool CheckEqual (const expected_T& expected, const actual_T& actual, std::string& msg)
{
  if (!(expected == actual))
  {
    std::stringstream stream;
    stream << "Expected " << expected << " but was " << actual;
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}

template <typename expected_T, typename actual_T>
bool CheckEqual (const expected_T* expected, const actual_T* actual, std::string& msg)
{
  if (!(*expected == *actual))
  {
    std::stringstream stream;
    stream << "Expected " << *expected << " but was " << *actual;
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}
///@}

/*!
  CheckEqual for C++ vectors.

  The generated error message lists the expected and actual vector elements.

  \param expected - expected_T vector values
  \param actual   - actual_T vector values
  \param msg      - generated error message

  \return `true` if vectors compare as equal
*/ 
template <typename expected_T, typename actual_T>
inline
bool CheckEqual (const std::vector<expected_T>& expected, const std::vector<actual_T>& actual, std::string& msg)
{
  if (expected != actual)
  {
    std::stringstream stream;
    stream << "Expected [ ";
    for (auto& p : expected)
      stream << p << " ";

    stream << "] but was [ ";
    for (auto& p : actual)
      stream << p << " ";

    stream << "]";
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}

/*!
  CheckEqual for C++ arrays.

  The generated error message lists the expected and actual array elements.

  \param expected - expected_T array
  \param actual   - actual_T array
  \param msg      - generated error message

  \return `true` if arrays compare as equal
*/
template <typename expected_T, typename actual_T, size_t N>
inline
bool CheckEqual (const std::array<expected_T,N>& expected, const std::array<actual_T,N>& actual, std::string& msg)
{
  if (expected != actual)
  {
    std::stringstream stream;
    stream << "Expected [ ";
    for (size_t i = 0; i < N; ++i)
      stream << expected[i] << " ";

    stream << "] but was [ ";
    for (size_t i = 0; i < N; ++i)
      stream << actual[i] << " ";

    stream << "]";
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}

/*!
  CheckEqual for C++ lists.

  The generated error message shows the expected and actual list elements.

  \param expected - expected_T list
  \param actual   - actual_T list
  \param msg      - generated error message

  \return `true` if lists compare as equal
*/
template <typename expected_T, typename actual_T>
inline
bool CheckEqual (const std::list<expected_T>& expected, const std::list<actual_T>& actual, std::string& msg)
{
  if (expected != actual)
  {
    std::stringstream stream;
    stream << "Expected ( ";
    for (auto const& x : expected)
      stream << x << " ";

    stream << ") but was ( ";
    for (auto const& x : actual)
      stream << x << " ";

    stream << ")";
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}


/// Internal function for conversion from UTF-16 to UTF-8
#if __WCHAR_MAX__ > 0x10000
inline std::string to_utf8 (const std::wstring& ws)
{
  std::string out;
  auto in = ws.cbegin ();
  while (in != ws.end ())
  {
    unsigned int c = (unsigned int)*in++;
    if (c < 0x7f)
      out.push_back ((char)c);
    else if (c < 0x7ff)
    {
      out.push_back (0xC0 | (c >> 6));
      out.push_back (0x80 | (c & 0x3f));
    }
    else if (c < 0xffff)
    {
      out.push_back (0xE0 | (c >> 12));
      out.push_back (0x80 | ((c >> 6) & 0x3f));
      out.push_back (0x80 | (c & 0x3f));
    }
    else
    {
      out.push_back (0xF0 | (c >> 18));
      out.push_back (0x80 | ((c >> 12) & 0x3f));
      out.push_back (0x80 | ((c >> 6) & 0x3f));
      out.push_back (0x80 | (c & 0x3f));
    }
  }
  return out;
}
#else
inline std::string to_utf8 (const std::wstring& ws)
{
  std::string out;
  auto in = ws.cbegin ();
  while (in != ws.end ())
  {
    unsigned int c1 = (unsigned int)*in++;
    if (c1 < 0xD800 || c1 > 0xe000)
    {
      if (c1 < 0x7f)
        out.push_back ((char)c1);
      else if (c1 < 0x7ff)
      {
        out.push_back (char(0xC0 | (c1 >> 6)));
        out.push_back (char(0x80 | (c1 & 0x3f)));
      }
      else
      {
        out.push_back (char(0xE0 | (c1 >> 12)));
        out.push_back (char(0x80 | ((c1 >> 6) & 0x3f)));
        out.push_back (char(0x80 | (c1 & 0x3f)));
      }
    }
    else if (in != ws.end ())
    {
      unsigned int c2 = (unsigned int)*in++;
      if (c1 > 0xdbff || c2 < 0xdc00)
        break; // invalid high/low surrogates order

      c1 &= 0x3ff;
      c2 &= 0x3ff;

      unsigned int c = ((c1 << 10) | c2) + 0x10000;

      out.push_back (char(0xF0 | (c >> 18)));
      out.push_back (char(0x80 | ((c >> 12) & 0x3f)));
      out.push_back (char(0x80 | ((c >> 6) & 0x3f)));
      out.push_back (char(0x80 | (c & 0x3f)));
    }
    else
      break; //malformed input; just bail out
  }
  return out;
}
#endif
/*!
  CheckEqual function for wide C++ strings.

  \param expected - expected string value
  \param actual   - actual string value
  \param msg      - generated error message

  \return `true` if strings match
@{
*/
inline
bool CheckEqual (const std::wstring expected, const std::wstring actual,
                                          std::string& msg)
{
  if (expected != actual)
  {
    std::stringstream stream;
    std::string u8exp = to_utf8 (expected);
    std::string u8act = to_utf8 (actual);
    stream << "Expected \'" << u8exp << "\' but was \'" << u8act << "\'";
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}

/*!
  CheckEqual function for wide C strings.

  \param expected - expected string value
  \param actual   - actual string value
  \param msg      - generated error message

  \return `true` if strings match
@{
*/
inline 
bool CheckEqual (const wchar_t *expected, const wchar_t *actual,
                                          std::string &msg)
{
  if (wcscmp (expected, actual))
  {
    std::stringstream stream;
    std::string u8exp = to_utf8 (expected);
    std::string u8act = to_utf8 (actual);
    stream << "Expected \'" << u8exp << "\' but was \'" << u8act << "\'";
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}


inline
bool CheckEqual (wchar_t *expected, wchar_t *actual, std::string &msg)
{
  return CheckEqual (const_cast<const wchar_t *> (expected), const_cast<const wchar_t *> (actual), msg);
}

inline
bool CheckEqual (const wchar_t *expected, wchar_t *actual, std::string &msg)
{
  return CheckEqual (expected, const_cast<const wchar_t *> (actual), msg);
}

inline
bool CheckEqual (wchar_t *expected, const wchar_t *actual, std::string &msg)
{
  return CheckEqual (const_cast<const wchar_t *> (expected), actual, msg);
}

///@}

/*!
  Specializations of CheckEqual function for C strings
  @{
*/
inline
bool CheckEqual (const char* expected, const char* actual, std::string& msg)
{
  if (strcmp (expected, actual))
  {
    std::stringstream stream;
    stream << "Expected \'" << expected << "\' but was \'" << actual << "\'";
    msg = stream.str ();
    return false;
  }
  return true;
}

inline
bool CheckEqual (char* expected, char* actual, std::string& msg)
{
  return CheckEqual (const_cast<const char *>(expected), const_cast<const char*>(actual), msg);
}

inline
bool CheckEqual (const char* expected, char* actual, std::string& msg)
{
  return CheckEqual (const_cast<const char *>(expected), const_cast<const char*>(actual), msg);
}

inline
bool CheckEqual (char* expected, const char* actual, std::string& msg)
{
  return CheckEqual (const_cast<const char *>(expected), const_cast<const char*>(actual), msg);
}
///@}

/*!
  Check if two values are closer than specified tolerance. If not, generate a
  failure message.

  \param expected   - expected value
  \param actual     - actual value
  \param tolerance  - allowed tolerance

  \return `true` if actual value is within the tolerance range

  If \p tolerance is 0, the function uses UnitTest::default_tolerance value.
  
  If UnitTest::default_tolerance is 0, the function throws an exception causing
  test to abort.
*/
template <typename expected_T, typename actual_T>
bool isClose (const expected_T& expected, const actual_T& actual, double tolerance)
{
  if (tolerance == 0)
  {
    if (UnitTest::default_tolerance == 0)
      throw UnitTest::tolerance_not_set ();
    tolerance = UnitTest::default_tolerance;
  }
  return fabs ((double)(actual - expected)) <= tolerance;
}
/*!
  Check if two values are closer than specified tolerance. If not, generate a
  failure message.

  \param expected   - expected value
  \param actual     - actual value
  \param tolerance  - allowed tolerance
  \param msg        - generated error message

  \return `true` if actual value is within the tolerance range
*/
template <typename expected_T, typename actual_T>
bool CheckClose (const expected_T& expected, const actual_T& actual, double tolerance,
                 std::string& msg)
{
  if (!isClose(actual, expected, tolerance))
  {
    auto fail_tol = tolerance ? tolerance : UnitTest::default_tolerance;
    int prec = (int)(1 - log10 (fail_tol));
    std::stringstream stream;
    stream.precision (prec);
    stream.setf (std::ios::fixed);
    stream << "Expected " << expected << " +/- " << fail_tol << " but was " << actual;
    msg = stream.str ();
    return false;
  }
  else
    msg.clear ();
  return true;
}

/*!
  Return true if two arrays are equal.
  \param expected   - array of expected values
  \param actual     - array of actual values
  \param count      - number of elements in each array

  \return `true` if the two arrays are equal
*/
template <typename expected_T, typename actual_T>
bool Equal1D (const expected_T& expected, const actual_T& actual, size_t count)
{
  for (size_t i = 0; i < count; ++i)
    if (expected[i] != actual[i])
      return false;
  return true;
}

/*!
  Check if two arrays are equal. If not, generate a failure message.
  \param expected   - Expected value
  \param actual     - Actual value
  \param count      - number of elements in each array
  \param msg        - generated error message

  \return `true` if the two values are equal
*/
template <typename expected_T, typename actual_T>
bool CheckArrayEqual (const expected_T& expected, const actual_T& actual,
                      size_t count, std::string& msg)
{
  if (!Equal1D (expected, actual, count))
  {
    std::stringstream stream;
    stream << "Expected [ ";
    for (size_t i = 0; i < count; ++i)
      stream << expected[i] << " ";

    stream << "] but was [ ";
    for (size_t i = 0; i < count; ++i)
      stream << actual[i] << " ";

    stream << "]";
    msg = stream.str ();
    return false;
  }
  return true;
}

/*!
  Return true if values in two arrays are closer than specified tolerance.

  \param expected   - array of expected values
  \param actual     - array of actual values
  \param count      - number of elements in each array
  \param tolerance  - allowed tolerance

  \return `true` if all actual values are within the tolerance range

  Calls isClose() for each element of the array to verify that it is within
  allowed limits.
*/
template <typename expected_T, typename actual_T>
bool isClose1D (const expected_T& expected, const actual_T& actual, size_t count, double tolerance)
{
  for (size_t i = 0; i < count; ++i)
  {
    if (!isClose(expected[i], actual[i], tolerance))
      return false;
  }
  return true;
}


/*!
  Check if values in two C arrays are closer than specified tolerance. If not,
  generate a failure message.

  \param expected   - array of expected values
  \param actual     - array of actual values
  \param count      - arrays size
  \param tolerance  - allowed tolerance
  \param msg        - generated error message

  \return `true` if all actual values are within the tolerance range
*/
template <typename expected_T, typename actual_T>
bool CheckArrayClose (const expected_T& expected, const actual_T& actual, size_t count, 
                      double tolerance, std::string& msg)
{
  if (!isClose1D (expected, actual, count, tolerance))
  {
    auto fail_tol = tolerance ? tolerance : UnitTest::default_tolerance;
    int prec = (int)(1 - log10 (fail_tol));

    std::stringstream stream;
    stream.precision (prec);
    stream.setf (std::ios::fixed);
    stream << "Expected [ ";
    for (size_t i = 0; i < count; ++i)
      stream << expected[i] << " ";

    stream << "] +/- " << fail_tol << " but was [ ";
    for (size_t i = 0; i < count; ++i)
      stream << actual[i] << " ";
    stream << "]";
    msg = stream.str ();
    return false;
  }
  return true;
}

/*!
  CheckEqual function for const void* pointers.

  \param expected - expected_T pointer value
  \param actual   - actual_T pointer value
  \param msg      - generated error message

  \return `true` if strings match
*/
template <>
inline
bool CheckEqual<void, void>(const void* expected, const void* actual, std::string& msg)
{
  if (!(expected == actual)) {
    std::stringstream stream;
    stream << "Expected " << expected << " but was " << actual;
    msg = stream.str();
    return false;
  } else
    msg.clear();
  return true;
}

/*!
  Check if values in two C++ vectors are closer than specified tolerance. If not,
  generate a failure message.

  \param expected   - vector of expected values
  \param actual     - vector of actual values
  \param tolerance  - allowed tolerance
  \param msg        - generated error message
  \return `true` if all actual values are within the tolerance range
*/
template <typename expected_T, typename actual_T>
bool CheckClose (const std::vector<expected_T>& expected, const std::vector<actual_T>& actual, double tolerance,
  std::string& msg)
{
  if (expected.size () != actual.size () 
   || !isClose1D (&expected[0], &actual[0], expected.size(), tolerance))
  {
    auto fail_tol = tolerance ? tolerance : UnitTest::default_tolerance;
    int prec = (int)(1 - log10 (fail_tol));
    std::stringstream stream;
    stream.precision (prec);
    stream.setf (std::ios::fixed);
    stream << "Expected [ ";
    for (auto& p : expected)
      stream << p << " ";

    stream << "] +/- " << fail_tol << " but was [ ";
    for (auto& p : actual)
      stream << p << " ";
    stream << "]";
    msg = stream.str ();
    return false;
  }
  return true;
}

/*!
  Check if values in two C++ arrays are closer than specified tolerance. If not,
  generate a failure message.

  \param expected   - array of expected values
  \param actual     - array of actual values
  \param tolerance  - allowed tolerance
  \param msg        - generated error message
  \return `true` if all actual values are within the tolerance range
*/
template <typename expected_T, typename actual_T, size_t N>
bool CheckClose (const std::array<expected_T, N>& expected, const std::array<actual_T, N>& actual, double tolerance,
  std::string& msg)
{
  if (!isClose1D (&expected[0], &actual[0], N, tolerance))
  {
    auto fail_tol = tolerance ? tolerance : UnitTest::default_tolerance;
    int prec = (int)(1 - log10 (fail_tol));
    std::stringstream stream;
    stream.precision (prec);
    stream.setf (std::ios::fixed);
    stream << "Expected [ ";
    for (auto& p : expected)
      stream << p << " ";

    stream << "] +/- " << fail_tol << " but was [ ";
    for (auto& p : actual)
      stream << p << " ";
    stream << "]";
    msg = stream.str ();
    return false;
  }
  return true;
}

/*!
  Return true if two 2D arrays are equal.
  \param expected - array of expected values
  \param actual   - array of actual values
  \param rows     - number of rows in each array
  \param columns  - number of columns in each array

  \return `true` if the two arrays are equal
*/
template <typename expected_T, typename actual_T>
bool Equal2D (const expected_T& expected, const actual_T& actual, size_t rows, size_t columns)
{
  for (size_t i = 0; i < rows; ++i)
    if (!Equal1D (expected[i], actual[i], columns))
      return false;
  return true;
}

/*!
  Check if two 2D arrays are equal. If not, generate a failure message.
  \param expected - array of expected values
  \param actual   - array of actual values
  \param rows     - number of rows in each array
  \param columns  - number of columns in each array
  \param msg      - generated error message

  \return `true` if the two arrays are equal
*/
template <typename expected_T, typename actual_T>
bool CheckArray2DEqual (const expected_T& expected, const actual_T& actual,
                        size_t rows, size_t columns, std::string& msg)
{
  if (!Equal2D (expected, actual, rows, columns))
  {
    std::stringstream stream;
    size_t i, j;
    stream << "Expected [\n";
    for (i = 0; i < rows; ++i)
    {
      stream << " [";
      for (j = 0; j < columns; ++j)
        stream << expected[i][j] << " ";
      stream << "]\n";
    }

    stream << "] but was [\n";
    for (i = 0; i < rows; ++i)
    {
      stream << " [";
      for (j = 0; j < columns; ++j)
        stream << actual[i][j] << " ";
      stream << "]\n";
    }
    stream << "]";
    msg = stream.str ();
    return false;
  }
  return true;
}

/*!
  Return true if values in two 2D arrays are closer than specified tolerance.
  \param expected   - array of expected values
  \param actual     - array of actual values
  \param rows       - number of rows in each array
  \param columns    - number of columns in each array
  \param tolerance  - allowed tolerance

  \return `true` if all values in the two arrays are within given tolerance
*/
template <typename expected_T, typename actual_T>
bool isClose2D (const expected_T& expected, const actual_T& actual, size_t rows, size_t columns,
              double tolerance)
{
  for (size_t i = 0; i < rows; ++i)
    if (!isClose1D (expected[i], actual[i], columns, tolerance))
      return false;
  return true;
}

/*!
  Check if values in two 2D arrays are closer than specified tolerance. If not,
  generate a failure message.
  \param expected   - array of expected values
  \param actual     - array of actual values
  \param rows       - number of rows in each array
  \param columns    - number of columns in each array
  \param tolerance  - allowed tolerance
  \param msg        - generated error message

  \return `true` if all values in the two arrays are within given tolerance
*/
template <typename expected_T, typename actual_T>
bool CheckArray2DClose (const expected_T& expected, const actual_T& actual,
                        size_t rows, size_t columns, double tolerance, std::string& msg)
{
  if (!isClose2D (expected, actual, rows, columns, tolerance))
  {
    auto fail_tol = tolerance ? tolerance : UnitTest::default_tolerance;
    int prec = (int)(1 - log10 (fail_tol));
    std::stringstream stream;
    stream.precision (prec);
    stream.setf (std::ios::fixed);
    stream << "Expected [\n";
    size_t i, j;
    for (i = 0; i < rows; ++i)
    {
      stream << " [ ";
      for (j = 0; j < columns; ++j)
        stream << expected[i][j] << " ";
      stream << "]\n";
    }

    stream << "] +/- " << fail_tol << " but was [\n";
    for (i = 0; i < rows; ++i)
    {
      stream << " [ ";
      for (j = 0; j < columns; ++j)
        stream << actual[i][j] << " ";
      stream << "]\n";
    }
    stream << "]";
    msg = stream.str ();
    return false;
  }
  msg.clear ();
  return true;
}

/*!
  Function called by CHECK_FILE_EQUAL() macro to compare two files.
  \param ref      Name of reference file
  \param actual   Name of output file
  \param message  Generated error message
  \return `true` if files are equal

  Files are compared as ASCII files and the error message tries to show where
  the first difference is.
*/
inline
bool CheckFileEqual (const std::string& ref, const std::string& actual, std::string& message)
{
  struct stat st1, st2;
  std::ostringstream buf;

  stat (ref.c_str(), &st1);
  stat (actual.c_str(), &st2);
  if (st1.st_size != st2.st_size)
  {
    buf << "Size is different (" << st1.st_size << " vs " << st2.st_size
      << ") while comparing " << ref << " and " << actual;
    message = buf.str();
    return false;
  }

  FILE* f1, * f2;
  f1 = fopen (ref.c_str(), "r");
  f2 = fopen (actual.c_str(), "r");
  if (!f1 || !f2)
  {
    if (f1) fclose (f1);
    if (f2) fclose (f2);
    buf << "Failed to open files while comparing "
      << ref << " and " << actual;
    message = buf.str();
    return false; //something wrong with files
  }

  size_t ln = 0;
  bool ok = true;
  char ln1[1024], ln2[1024];
  while (ok)
  {
    ln++;
    if (fgets (ln1, sizeof (ln1), f1)
      && fgets (ln2, sizeof (ln2), f2))
      ok = !strcmp (ln1, ln2);
    else
      break;
  }
  fclose (f1);
  fclose (f2);
  if (!ok)
  {
    char* p1, * p2;
    int off;
    for (off = 0, p1 = ln1, p2 = ln2;
      *p1 && *p2 && *p1 == *p2;
      p1++, p2++, off++)
      ;
	buf << "Difference at line " << ln << " position " << off
      << " while comparing " << ref << " and " << actual;
    message = buf.str();
  }
  else
    message.clear ();
  return ok;
}

} //end namespace

/*!
  \ingroup gt 
  These macro definitions provide some compatibility with GoogleTest
@{
*/

#define EXPECT_TRUE(x) CHECK (x)
#define EXPECT_FALSE(x) CHECK (!(x))
#define EXPECT_EQ(A, B) CHECK_EQUAL (B, A)
#define EXPECT_NE(A, B)                                                       \
  do                                                                          \
  {                                                                           \
    try {                                                                     \
      std::string str__;                                                      \
      if (UnitTest::CheckEqual ((A), (B), str__))                             \
        UnitTest::ReportFailure (__FILE__, __LINE__, str__);                  \
    }                                                                         \
    catch (...) {                                                             \
      UnitTest::ReportFailure (__FILE__, __LINE__,                            \
            "Unhandled exception in CHECK_EQUAL(" #A ", " #B ")");            \
    }                                                                         \
  } while (0)

#define EXPECT_GE(A, B) CHECK ((A) >= (B))
#define EXPECT_GT(A, B) CHECK ((A) > (B))
#define EXPECT_LE(A, B) CHECK ((A) <= (B))
#define EXPECT_LT(A, B) CHECK ((A) < (B))

#define EXPECT_NEAR(A, B, tol) CHECK_CLOSE(B, A, tol)
#define EXPECT_THROW(expr, except) CHECK_THROW(expr, except)

#define ASSERT_TRUE(expr) ABORT (!(expr))
#define ASSERT_FALSE(expr) ABORT (expr)
#define ASSERT_EQ(e1, e2)                                                     \
  do                                                                          \
  {                                                                           \
    std::string str__;                                                        \
    if (!UnitTest::CheckEqual((e1), (e2), str__))                             \
      throw UnitTest::test_abort (__FILE__, __LINE__, str__.c_str());         \
  } while (0)

#define ASSERT_NE(e1, e2)                                                     \
  do                                                                          \
  {                                                                           \
    std::string str__;                                                        \
    if (UnitTest::CheckEqual ((e1), (e2), str__))                             \
    {                                                                         \
      std::stringstream stream__;                                             \
      stream__ << (e1) << " and " << (e2) << " should be different";          \
      throw UnitTest::test_abort (__FILE__, __LINE__, stream__.str ().c_str ());\
    }                                                                         \
  } while (0)

#define ASSERT_GE(e1, e2) ABORT ((e1) < (e2))
#define ASSERT_GT(e1, e2) ABORT ((e1) <= (e2))
#define ASSERT_LE(e1, e2) ABORT ((e1) > (e2))
#define ASSERT_LT(e1, e2) ABORT ((e1) >= (e2))
///@}

/*!
  \defgroup checks  Assertion Checking Macros
  \defgroup tests   Test Definition Macros
  \defgroup time    Time Control Macros
  \defgroup gt      Compatibility Macros
  \defgroup exec    Execution Control 
*/
