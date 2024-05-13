/**
*
* @file
*
* @brief  Metainfo operating interfaces
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <binary/container.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Chiptune
  {
    class PatchedDataBuilder
    {
    public:
      typedef std::auto_ptr<PatchedDataBuilder> Ptr;
      virtual ~PatchedDataBuilder() {}

      virtual void InsertData(std::size_t offset, const Dump& data) = 0;
      virtual void OverwriteData(std::size_t offset, const Dump& data) = 0;
      //offset in original, non-patched data
      virtual void FixLEWord(std::size_t offset, int_t delta) = 0;
      virtual Binary::Container::Ptr GetResult() const = 0;

      static Ptr Create(const Binary::Container& data);
    };
  }
}
