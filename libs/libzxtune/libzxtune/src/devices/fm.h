/**
* 
* @file
*
* @brief  FM support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <data_streaming.h>
#include <types.h>
//library includes
#include <devices/state.h>
#include <sound/receiver.h>
#include <time/stamp.h>
//boost includes
#include <boost/array.hpp>

namespace Devices
{
  namespace FM
  {
    const uint_t VOICES = 3;

    typedef Time::Microseconds Stamp;

    class Register
    {
    public:
      Register()
        : Val()
      {
      }

      Register(uint_t idx, uint_t val)
        : Val((idx << 8) | val)
      {
      }

      uint_t Index() const
      {
        return (Val >> 8) & 0xff;
      }

      uint_t Value() const
      {
        return Val & 0xff;
      }
    protected:
      uint_t Val;
    };

    typedef std::vector<Register> Registers;

    struct DataChunk
    {
      DataChunk() : TimeStamp()
      {
      }

      Stamp TimeStamp;
      Registers Data;
    };

    class Device
    {
    public:
      typedef boost::shared_ptr<Device> Ptr;
      virtual ~Device() {}

      /// render single data chunk
      virtual void RenderData(const DataChunk& src) = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    // Describes real device
    class Chip : public Device, public StateSource
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
    };

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() {}

      virtual uint_t Version() const = 0;
      virtual uint64_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}
