/**
*
* @file
*
* @brief  Declaration of sound sample
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>

namespace Sound
{
  struct Sample
  {
  public:
    static const uint_t CHANNELS = 2;
    static const uint_t BITS = 16;
    typedef int16_t Type;
    typedef int_t WideType;
    static const Type MIN = -32768;
    static const Type MID = 0;
    static const Type MAX = 32767;

    Sample()
      : Value()
    {
    }

    template<class T>
    Sample(T left, T right)
      : Value((StorageType(static_cast<uint16_t>(right)) << SHIFT) | static_cast<uint16_t>(left))
    {
    }

    WideType Left() const
    {
      return static_cast<Type>(Value);
    }

    WideType Right() const
    {
      return static_cast<Type>(Value >> SHIFT);
    }

    bool operator == (const Sample& rh) const
    {
      return Value == rh.Value;
    }

    static Sample FastAdd(Sample lh, Sample rh)
    {
      return Sample(lh.Value + rh.Value);
    }
  private:
    explicit Sample(uint_t val)
      : Value(val)
    {
    }
  private:
    typedef uint32_t StorageType;
    static const uint_t SHIFT = 8 * sizeof(StorageType) / 2;
    StorageType Value;
  };
}
