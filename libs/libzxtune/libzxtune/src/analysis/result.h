/**
* 
* @file
*
* @brief Analysis result interface and factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/format.h>
#include <binary/container.h>

namespace Analysis
{
  //! Abstract data detection result
  //! TODO: temporary solution with negative lookahead offsets for statistic collecting
  class Result
  {
  public:
    typedef boost::shared_ptr<const Result> Ptr;
    virtual ~Result() {}

    //! @brief Returns data size that is processed in current data position
    //! @return Size of input data format detected
    //! @invariant Return 0 if data is not matched at the beginning
    virtual std::size_t GetMatchedDataSize() const = 0;
    //! @brief Search format in forward data
    //! @return Offset in input data where perform next checking
    //! @invariant Return >0 if current data not matches format (up to input data's size if no matches at all)
    //! @invariant Return 0 if input data matches
    virtual std::size_t GetLookaheadOffset() const = 0;
  };


  Result::Ptr CreateMatchedResult(std::size_t matchedSize);
  Result::Ptr CreateUnmatchedResult(Binary::Format::Ptr format, Binary::Container::Ptr data);
  Result::Ptr CreateUnmatchedResult(std::size_t unmatchedSize);
}
