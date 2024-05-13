/**
*
* @file
*
* @brief  Binary data container interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/data.h>

namespace Binary
{
  class Container : public Data
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<const Container> Ptr;

    //! @brief Provides isolated access to nested subcontainers should be able even after parent container destruction
    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const = 0;
  };
}
