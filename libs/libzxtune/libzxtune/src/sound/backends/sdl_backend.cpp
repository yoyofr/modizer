/**
*
* @file
*
* @brief  SDL backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

#define DECLSPEC

//local includes
#include "backend_impl.h"
#include "storage.h"
#include "gates/sdl_api.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread/condition_variable.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG 608CF986

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Sdl");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace Sdl
{
  const String ID = Text::SDL_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("SDL support backend");
  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM;

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  class BackendParameters
  {
  public:
    explicit BackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    uint_t GetBuffersCount() const
    {
      Parameters::IntType val = Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS, val) &&
          (!Math::InRange<Parameters::IntType>(val, BUFFERS_MIN, BUFFERS_MAX)))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("SDL backend error: buffers count (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(val), BUFFERS_MIN, BUFFERS_MAX);
      }
      return static_cast<uint_t>(val);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class BuffersQueue
  {
  public:
    explicit BuffersQueue(uint_t size)
      : Buffers(size)
      , FillIter(&Buffers.front(), &Buffers.back() + 1)
      , PlayIter(FillIter)
    {
    }

    void AddData(Chunk& buffer)
    {
      boost::mutex::scoped_lock locker(BufferMutex);
      while (FillIter->BytesToPlay)
      {
        PlayedEvent.wait(locker);
      }
      FillIter->Data.swap(buffer);
      FillIter->BytesToPlay = FillIter->Data.size() * sizeof(FillIter->Data.front());
      ++FillIter;
      FilledEvent.notify_one();
    }

    void GetData(uint8_t* stream, uint_t len)
    {
      boost::mutex::scoped_lock locker(BufferMutex);
      while (len)
      {
        //wait for data
        while (!PlayIter->BytesToPlay)
        {
          FilledEvent.wait(locker);
        }
        const uint_t inBuffer = PlayIter->Data.size() * sizeof(PlayIter->Data.front());
        const uint_t toCopy = std::min<uint_t>(len, PlayIter->BytesToPlay);
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(&PlayIter->Data.front());
        std::memcpy(stream, src + (inBuffer - PlayIter->BytesToPlay), toCopy);
        PlayIter->BytesToPlay -= toCopy;
        stream += toCopy;
        len -= toCopy;
        if (!PlayIter->BytesToPlay)
        {
          //buffer is played
          ++PlayIter;
          PlayedEvent.notify_one();
        }
      }
    }

    void SetSize(uint_t size)
    {
      Dbg("Change buffers count %1% -> %2%", Buffers.size(), size);
      Buffers.resize(size);
      FillIter = CycledIterator<Buffer*>(&Buffers.front(), &Buffers.back() + 1);
      PlayIter = FillIter;
    }

    uint_t GetSize() const
    {
      return Buffers.size();
    }
  private:
    boost::mutex BufferMutex;
    boost::condition_variable FilledEvent, PlayedEvent;
    struct Buffer
    {
      Buffer() : BytesToPlay()
      {
      }
      uint_t BytesToPlay;
      Chunk Data;
    };
    std::vector<Buffer> Buffers;
    CycledIterator<Buffer*> FillIter, PlayIter;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Api::Ptr api, Parameters::Accessor::Ptr params)
      : SdlApi(api)
      , Params(params)
      , WasInitialized(SdlApi->SDL_WasInit(SDL_INIT_EVERYTHING))
      , Queue(Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS_DEFAULT)
    {
      if (0 == WasInitialized)
      {
        Dbg("Initializing");
        CheckCall(SdlApi->SDL_Init(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Dbg("Initializing sound subsystem");
        CheckCall(SdlApi->SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
    }

    virtual ~BackendWorker()
    {
      if (0 == WasInitialized)
      {
        Dbg("Shutting down");
        SdlApi->SDL_Quit();
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Dbg("Shutting down sound subsystem");
        SdlApi->SDL_QuitSubSystem(SDL_INIT_AUDIO);
      }
    }

    virtual void Startup()
    {
      Dbg("Starting playback");

      SDL_AudioSpec format;
      format.format = -1;
      const bool sampleSigned = Sample::MID == 0;
      switch (Sample::BITS)
      {
      case 8:
        format.format = sampleSigned ? AUDIO_S8 : AUDIO_U8;
        break;
      case 16:
        format.format = sampleSigned ? AUDIO_S16SYS : AUDIO_U16SYS;
        break;
      default:
        assert(!"Invalid format");
      }

      const RenderParameters::Ptr sound = RenderParameters::Create(Params);
      const BackendParameters backend(*Params);
      format.freq = sound->SoundFreq();
      format.channels = static_cast< ::Uint8>(Sample::CHANNELS);
      format.samples = sound->SamplesPerFrame();
      //fix if size is not power of 2
      if (0 != (format.samples & (format.samples - 1)))
      {
        unsigned msk = 1;
        while (format.samples > msk)
        {
          msk <<= 1;
        }
        format.samples = msk;
      }
      format.callback = OnBuffer;
      format.userdata = &Queue;
      Queue.SetSize(backend.GetBuffersCount());
      CheckCall(SdlApi->SDL_OpenAudio(&format, 0) >= 0, THIS_LINE);
      SdlApi->SDL_PauseAudio(0);
    }

    virtual void Shutdown()
    {
      Dbg("Shutdown");
      SdlApi->SDL_CloseAudio();
    }

    virtual void Pause()
    {
      Dbg("Pause");
      SdlApi->SDL_PauseAudio(1);
    }

    virtual void Resume()
    {
      Dbg("Resume");
      SdlApi->SDL_PauseAudio(0);
    }

    virtual void FrameStart(const Module::TrackState& /*state*/)
    {
    }

    virtual void FrameFinish(Chunk::Ptr buffer)
    {
      Queue.AddData(*buffer);
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }
  private:
    void CheckCall(bool ok, Error::LocationRef loc) const
    {
      if (!ok)
      {
        if (const char* txt = SdlApi->SDL_GetError())
        {
          throw MakeFormattedError(loc,
            translate("Error in SDL backend: %1%."), FromStdString(txt));
        }
        throw Error(loc, translate("Unknown error in SDL backend."));
      }
    }

    static void OnBuffer(void* param, ::Uint8* stream, int len)
    {
      BuffersQueue* const queue = static_cast<BuffersQueue*>(param);
      queue->GetData(stream, len);
    }
  private:
    const Api::Ptr SdlApi;
    const Parameters::Accessor::Ptr Params;
    const ::Uint32 WasInitialized;
    BuffersQueue Queue;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : SdlApi(api)
    {
    }

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const
    {
      return boost::make_shared<BackendWorker>(SdlApi, params);
    }
  private:
    const Api::Ptr SdlApi;
  };
}//Sdl
}//Sound

namespace Sound
{
  void RegisterSdlBackend(BackendsStorage& storage)
  {
    try
    {
      const Sdl::Api::Ptr api = Sdl::LoadDynamicApi();
      const SDL_version* const vers = api->SDL_Linked_Version();
      Dbg("Detected SDL %1%.%2%.%3%", unsigned(vers->major), unsigned(vers->minor), unsigned(vers->patch));
      const BackendWorkerFactory::Ptr factory = boost::make_shared<Sdl::BackendWorkerFactory>(api);
      storage.Register(Sdl::ID, Sdl::DESCRIPTION, Sdl::CAPABILITIES, factory);
    }
    catch (const Error& e)
    {
      storage.Register(Sdl::ID, Sdl::DESCRIPTION, Sdl::CAPABILITIES, e);
    }
  }
}
