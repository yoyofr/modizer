/**
*
* @file
*
* @brief  Serialize API
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <strings/map.h>

namespace Parameters
{
  void Convert(const Strings::Map& map, class Visitor& visitor);
  void Convert(const class Accessor& ac, Strings::Map& strings);
}
