/**
*
* @file
*
* @brief  Images support interfaces
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/container.h>
#include <binary/format.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Image
  {
    //! @brief Image raw data presentation
    class Container : public Binary::Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;

      //! @brief Getting size of source data this container was extracted from
      //! @return Size in bytes
      //! @invariant Result is always > 0
      virtual std::size_t OriginalSize() const = 0;
    };

    //! @brief Decoding functionality provider
    class Decoder
    {
    public:
      typedef boost::shared_ptr<const Decoder> Ptr;
      virtual ~Decoder() {}

      //! @brief Get short decoder description
      virtual String GetDescription() const = 0;

      //! @brief Get approximate format description to search in raw binary data
      //! @invariant Cannot be empty
      virtual Binary::Format::Ptr GetFormat() const = 0;

      //! @brief Perform raw data decoding
      //! @param rawData Data to be decoded
      //! @return Non-null object if data is successfully recognized and decoded
      //! @invariant Result is always rawData's subcontainer
      virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
    };
  }
}
