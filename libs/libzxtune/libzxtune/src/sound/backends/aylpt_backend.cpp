/**
*
* @file
*
* @brief  AYLPT backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "backend_impl.h"
#include "storage.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/conversion/api.h>
#include <debug/log.h>
#include <devices/aym.h>
#include <l10n/api.h>
#include <platform/shared_library.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/range/end.hpp>
#include <boost/static_assert.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG F1936398

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Aylpt");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace AyLpt
{
  const String ID = Text::AYLPT_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("Real AY via LPT backend");
  const uint_t CAPABILITIES = CAP_TYPE_HARDWARE;

  //http://logix4u.net/component/content/article/14-parallel-port/15-a-tutorial-on-parallel-port-interfacing
  //http://bulba.untergrund.net/LPT-YM.7z
  enum
  {
    DATA_PORT = 0x378,
    CONTROL_PORT = DATA_PORT + 2,

    //pin14 (Control-1) -> ~BDIR
    PIN_NOWRITE = 0x2,
    //pin16 (Control-2) -> BC1
    PIN_ADDR = 0x4,
    //pin17 (Control-3) -> ~RESET
    PIN_NORESET = 0x8,
    //other unused pins
    PIN_UNUSED = 0xf1,

    CMD_SELECT_ADDR = PIN_ADDR | PIN_NORESET | PIN_UNUSED,
    CMD_SELECT_DATA = PIN_NORESET | PIN_UNUSED,
    CMD_WRITE_COMMIT = PIN_NOWRITE | PIN_NORESET | PIN_UNUSED,
    CMD_RESET_START = PIN_ADDR | PIN_UNUSED,
    CMD_RESET_STOP = PIN_NORESET | PIN_ADDR | PIN_UNUSED,
  };

  BOOST_STATIC_ASSERT(CMD_SELECT_ADDR == 0xfd);
  BOOST_STATIC_ASSERT(CMD_SELECT_DATA == 0xf9);
  BOOST_STATIC_ASSERT(CMD_WRITE_COMMIT == 0xfb);
  BOOST_STATIC_ASSERT(CMD_RESET_START == 0xf5);
  BOOST_STATIC_ASSERT(CMD_RESET_STOP == 0xfd);

  void Delay()
  {
    //according to datasheets, maximal timing is reset pulse width 5uS
    boost::this_thread::sleep(boost::posix_time::microseconds(10));
  }

  class LptPort
  {
  public:
    typedef boost::shared_ptr<LptPort> Ptr;
    virtual ~LptPort() {}

    virtual void Control(uint_t val) = 0;
    virtual void Data(uint_t val) = 0;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Parameters::Accessor::Ptr params, Binary::Data::Ptr data, LptPort::Ptr port)
      : Params(Sound::RenderParameters::Create(params))
      , Data(data)
      , Port(port)
    {
    }

    virtual ~BackendWorker()
    {
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }

    virtual void Startup()
    {
      Reset();
      Dbg("Successfull start");
      FrameDuration = boost::posix_time::microseconds(Params->FrameDuration().Get());
    }

    virtual void Shutdown()
    {
      Reset();
      Dbg("Successfull shut down");
    }

    virtual void Pause()
    {
      Reset();
    }

    virtual void Resume()
    {
    }

    virtual void FrameStart(const Module::TrackState& state)
    {
      WaitForNextFrame();
      const uint8_t* regs = static_cast<const uint8_t*>(Data->Start()) + state.Frame() * Devices::AYM::Registers::TOTAL;
      WriteRegisters(regs);
    }

    virtual void FrameFinish(Chunk::Ptr /*buffer*/)
    {
    }
  private:
    void Reset()
    {
      Port->Control(CMD_RESET_STOP);
      Port->Control(CMD_RESET_START);
      Delay();
      Port->Control(CMD_RESET_STOP);
    }

    void Write(uint_t reg, uint_t val)
    {
      Port->Control(CMD_WRITE_COMMIT);
      Port->Data(reg);
      Port->Control(CMD_SELECT_ADDR);
      Port->Data(reg);

      Port->Control(CMD_WRITE_COMMIT);
      Port->Data(val);
      Port->Control(CMD_SELECT_DATA);
      Port->Data(val);
      Port->Control(CMD_WRITE_COMMIT);
    }

    void WaitForNextFrame()
    {
      if (NextFrameTime.is_not_a_date_time())
      {
        NextFrameTime = boost::get_system_time();
      }
      else
      {
        boost::mutex::scoped_lock lock(Mutex);
        //prevent spurious wakeup
        while (Event.timed_wait(lock, NextFrameTime)) {}
      }
      NextFrameTime += FrameDuration;
    }

    void WriteRegisters(const uint8_t* data)
    {
      for (uint_t idx = 0; idx <= Devices::AYM::Registers::ENV; ++idx)
      {
        const uint_t reg = Devices::AYM::Registers::ENV - idx;
        const uint_t val = data[reg];
        if (reg != Devices::AYM::Registers::ENV || val != 0xff)
        {
          Write(reg, val);
        }
      }
    }
  private:
    const Sound::RenderParameters::Ptr Params;
    const Binary::Data::Ptr Data;
    const LptPort::Ptr Port;
    boost::mutex Mutex;
    boost::condition_variable Event;
    boost::system_time NextFrameTime;
    boost::posix_time::time_duration FrameDuration;
  };

  class DlPortIO : public LptPort
  {
  public:
    explicit DlPortIO(Platform::SharedLibrary::Ptr lib)
      : Lib(lib)
      , WriteByte(reinterpret_cast<WriteFunctionType>(Lib->GetSymbol("DlPortWritePortUchar")))
    {
    }

    virtual void Control(uint_t val)
    {
      WriteByte(CONTROL_PORT, val);
    }

    virtual void Data(uint_t val)
    {
      WriteByte(DATA_PORT, val);
    }
  private:
    const Platform::SharedLibrary::Ptr Lib;
    typedef void (__stdcall *WriteFunctionType)(unsigned short port, unsigned char val);
    const WriteFunctionType WriteByte;
  };

  class DllName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "dlportio";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      return std::vector<std::string>();
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "inpout32.dll",
        "inpoutx64.dll"
      };
      return std::vector<std::string>(ALTERNATIVES, boost::end(ALTERNATIVES));
    }
  };

  LptPort::Ptr LoadLptLibrary()
  {
    static const DllName NAME;
    const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
    return boost::make_shared<DlPortIO>(lib);
  }

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(LptPort::Ptr port)
      : Port(port)
    {
    }

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const
    {
      static const Module::Conversion::AYDumpConvertParam spec;
      if (const Binary::Data::Ptr data = Module::Convert(*holder, spec, params))
      {
        return boost::make_shared<BackendWorker>(params, data, Port);
      }
      throw Error(THIS_LINE, translate("Real AY via LPT is not supported for this module."));
    }
  private:
    const LptPort::Ptr Port;
  };
}//AyLpt
}//Sound

namespace Sound
{
  void RegisterAyLptBackend(BackendsStorage& storage)
  {
    try
    {
      const AyLpt::LptPort::Ptr port = AyLpt::LoadLptLibrary();
      const BackendWorkerFactory::Ptr factory = boost::make_shared<AyLpt::BackendWorkerFactory>(port);
      storage.Register(AyLpt::ID, AyLpt::DESCRIPTION, AyLpt::CAPABILITIES, factory);
    }
    catch (const Error& e)
    {
      storage.Register(AyLpt::ID, AyLpt::DESCRIPTION, AyLpt::CAPABILITIES, e);
    }
  }
}
