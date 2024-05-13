/**
*
* @file
*
* @brief  End-user IO API
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/container.h>
#include <binary/output_stream.h>
#include <io/identifier.h>

//forward declarations
class Error;
namespace Parameters
{
  class Accessor;
}

namespace Log
{
  class ProgressCallback;
}

namespace IO
{
  //! @brief Resolve uri to identifier object
  //! @param uri Full data identifier
  //! @throw Error if failed to resolve
  Identifier::Ptr ResolveUri(const String& uri);

  //! @brief Performs opening specified uri
  //! @param path External data identifier
  //! @param params %Parameters accessor to setup providers' work
  //! @param cb Callback for long-time controllable operations
  //! @throw Error if failed to open
  Binary::Container::Ptr OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb);

  //! @brief Performs creating output stream with specified path
  //! @param path Data identifier
  //! @param %Parameters accessor
  //! @param cb Callback for long-time operations
  Binary::OutputStream::Ptr CreateStream(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb);
}
