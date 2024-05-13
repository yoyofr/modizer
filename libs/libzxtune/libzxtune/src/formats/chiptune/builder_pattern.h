/**
* 
* @file
*
* @brief  Pattern builder interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>

namespace Formats
{
  namespace Chiptune
  {
    class PatternBuilder
    {
    public:
      virtual ~PatternBuilder() {}

      virtual void Finish(uint_t size) = 0;
      //! @invariant Lines are built sequentally
      virtual void StartLine(uint_t index) = 0;
      virtual void SetTempo(uint_t tempo) = 0;
    };

    PatternBuilder& GetStubPatternBuilder();
  }
}
