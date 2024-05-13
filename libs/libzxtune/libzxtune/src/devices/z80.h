/**
* 
* @file
*
* @brief  Z80 support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <time/oscillator.h>
//boost includes
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

namespace Devices
{
  namespace Z80
  {
    // Use optimized stamp and oscillator types- 7% accuracy
    typedef Time::Stamp<uint64_t, 1 << 30> Stamp;
    typedef Time::Oscillator<Stamp> Oscillator;

    class ChipIO
    {
    public:
      typedef boost::shared_ptr<ChipIO> Ptr;
      virtual ~ChipIO() {}

      virtual uint8_t Read(uint16_t addr) = 0;
      virtual void Write(const Oscillator& timeStamp, uint16_t addr, uint8_t data) = 0;
    };

    struct Registers
    {
      enum
      {
        REG_AF,
        REG_BC,
        REG_DE,
        REG_HL,
        REG_AF_,
        REG_BC_,
        REG_DE_,
        REG_HL_,
        REG_IX,
        REG_IY,
        REG_IR,
        REG_PC,
        REG_SP,

        REG_LAST
      };

      typedef boost::array<uint16_t, REG_LAST> Dump;
      Dump Data;
      uint32_t Mask;
    };

    class Chip
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
      virtual ~Chip() {}

      virtual void Reset() = 0;
      virtual void Interrupt() = 0;
      virtual void Execute(const Stamp& till) = 0;
      virtual void SetRegisters(const Registers& regs) = 0;
      virtual void GetRegisters(Registers::Dump& regs) const = 0;
      virtual Stamp GetTime() const = 0;
      virtual uint64_t GetTick() const = 0;
      virtual void SetTime(const Stamp& time) = 0;
    };

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;
      virtual ~ChipParameters() {}

      virtual uint_t Version() const = 0;
      virtual uint_t IntTicks() const = 0;
      virtual uint64_t ClockFreq() const = 0;
    };

    Chip::Ptr CreateChip(ChipParameters::Ptr params, ChipIO::Ptr memory, ChipIO::Ptr ports);
    Chip::Ptr CreateChip(ChipParameters::Ptr params, const Dump& memory, ChipIO::Ptr ports);
  }
}
