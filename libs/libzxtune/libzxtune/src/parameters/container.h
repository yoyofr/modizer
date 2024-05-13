/**
*
* @file
*
* @brief  Parameters container interface and factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/accessor.h>
#include <parameters/modifier.h>

namespace Parameters
{
  //! @brief Service type to simply properties keep and give access
  //! @invariant Only last value is kept for multiple assignment
  class Container : public Accessor
                  , public Modifier
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Container> Ptr;

    static Ptr Create();
    static Ptr CreateAdapter(Accessor::Ptr accessor, Modifier::Ptr modifier);
  };
}
