/**
*
* @file
*
* @brief  Composite format implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "details.h"
//common includes
#include <contract.h>
#include <types.h>
//library includes
#include <binary/data_adapter.h>
#include <binary/format_factories.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Binary
{
  std::size_t GetSize(Format::Ptr format)
  {
    if (const FormatDetails::Ptr dtl = boost::dynamic_pointer_cast<const FormatDetails>(format))
    {
      return dtl->GetMinSize();
    }
    else
    {
      return 0;
    }
  }

  class CompositeFormat : public Format
  {
  public:
    CompositeFormat(Format::Ptr header, Format::Ptr footer, std::size_t minFooterOffset, std::size_t maxFooterOffset)
      : Header(header)
      , Footer(footer)
      , MinFooterOffset(std::max(minFooterOffset, GetSize(header)))
      , MaxFooterOffset(maxFooterOffset)
      , FooterSize(GetSize(footer))
    {
      Require(MinFooterOffset <= MaxFooterOffset);
    }

    virtual bool Match(const Data& data) const
    {
      const std::size_t size = data.Size();
      if (size < MinFooterOffset + FooterSize)
      {
        return false;
      }
      if (!Header->Match(data))
      {
        return false;
      }
      const std::size_t searchStartPos = MinFooterOffset;
      const uint8_t* const searchStart = static_cast<const uint8_t*>(data.Start()) + searchStartPos;
      const std::size_t searchSize = std::min(MaxFooterOffset + FooterSize, size) - searchStartPos;
	   
	  std::size_t s= SearchFooter(searchStart, searchSize);
      return searchSize != s;
    }

    virtual std::size_t NextMatchOffset(const Data& data) const
    {
      const uint8_t* const start = static_cast<const uint8_t*>(data.Start());
      const std::size_t limit = data.Size();
      for (std::size_t headerCursor = 1;;)
      {
        const std::size_t restForHeader = limit - headerCursor;
        const std::size_t headerOffset = SearchHeader(start + headerCursor, restForHeader);
        if (headerOffset + MinFooterOffset + FooterSize > restForHeader)
        {
          //no place for footer
          break;
        }
        const std::size_t absoluteHeaderOffset = headerCursor + headerOffset;

        const std::size_t footerCursor = absoluteHeaderOffset + MinFooterOffset;
        const std::size_t restForFooter = limit - footerCursor;
        const std::size_t footerOffset = SearchFooter(start + footerCursor, restForFooter);
        if (footerOffset == restForFooter)
        {
          //footer not found
          break;
        }
        else if (footerOffset + MinFooterOffset <= MaxFooterOffset)
        {
          return absoluteHeaderOffset;
        }
        const std::size_t absoluteFooterOffset = footerCursor + footerOffset;
        headerCursor = std::max(absoluteHeaderOffset + 1, absoluteFooterOffset - MaxFooterOffset);
      }
      return limit;
    }
  private:
    //returns absolute offset from start covering case when match happends at start
    std::size_t SearchHeader(const uint8_t* start, std::size_t rest) const
    {
      return Header->NextMatchOffset(DataAdapter(start - 1, rest + 1)) - 1;
    }

    std::size_t SearchFooter(const uint8_t* start, std::size_t rest) const
    {
      return Footer->NextMatchOffset(DataAdapter(start - 1, rest + 1)) - 1;
    }
  private:
    const Format::Ptr Header;
    const Format::Ptr Footer;
    const std::size_t MinFooterOffset;
    const std::size_t MaxFooterOffset;
    const std::size_t FooterSize;
  };
}

namespace Binary
{
  Format::Ptr CreateCompositeFormat(Format::Ptr header, Format::Ptr footer, std::size_t minFooterOffset, std::size_t maxFooterOffset)
  {
    return boost::make_shared<CompositeFormat>(header, footer, minFooterOffset, maxFooterOffset);
  }
}
