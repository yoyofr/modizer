/**
* 
* @file
*
* @brief  Programming by contract interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <exception>

inline void Require(bool requirement)
{
  if (!requirement)
  {
    throw std::exception();
  }
}
