/**
*
* @file
*
* @brief  Simple binary data abstraction
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//boost includes
#include <boost/shared_ptr.hpp>

namespace Binary
{
  //! @brief Data container abstraction interface
  class Data
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<const Data> Ptr;

    virtual ~Data() {}

    //! @brief Raw data accessible for at least Size() bytes
    //! @invariant Should not change after first call
    //! @invariant Always != null
    virtual const void* Start() const = 0;
    //! @brief Getting data size in bytes
    //! @invariant Always >= 0
    virtual std::size_t Size() const = 0;
  };
}
