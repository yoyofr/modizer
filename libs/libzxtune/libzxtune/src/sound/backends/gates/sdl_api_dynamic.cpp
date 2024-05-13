/**
*
* @file
*
* @brief  SDL subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sdl_api.h"
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/range/end.hpp>

namespace
{
  using namespace Sound::Sdl;

  class SdlName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "SDL";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libSDL-1.2.so.0",
      };
      return std::vector<std::string>(ALTERNATIVES, boost::end(ALTERNATIVES));
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      return std::vector<std::string>();
    }
  };

  const Debug::Stream Dbg("Sound::Backend::Sdl");

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(lib)
    {
      Dbg("Library loaded");
    }

    virtual ~DynamicApi()
    {
      Dbg("Library unloaded");
    }

    
    virtual char* SDL_GetError(void)
    {
      static const char NAME[] = "SDL_GetError";
      typedef char* ( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual const SDL_version* SDL_Linked_Version(void)
    {
      static const char NAME[] = "SDL_Linked_Version";
      typedef const SDL_version* ( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual int SDL_Init(Uint32 flags)
    {
      static const char NAME[] = "SDL_Init";
      typedef int ( *FunctionType)(Uint32);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(flags);
    }
    
    virtual int SDL_InitSubSystem(Uint32 flags)
    {
      static const char NAME[] = "SDL_InitSubSystem";
      typedef int ( *FunctionType)(Uint32);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(flags);
    }
    
    virtual void SDL_QuitSubSystem(Uint32 flags)
    {
      static const char NAME[] = "SDL_QuitSubSystem";
      typedef void ( *FunctionType)(Uint32);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(flags);
    }
    
    virtual Uint32 SDL_WasInit(Uint32 flags)
    {
      static const char NAME[] = "SDL_WasInit";
      typedef Uint32 ( *FunctionType)(Uint32);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(flags);
    }
    
    virtual void SDL_Quit(void)
    {
      static const char NAME[] = "SDL_Quit";
      typedef void ( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
    {
      static const char NAME[] = "SDL_OpenAudio";
      typedef int ( *FunctionType)(SDL_AudioSpec *, SDL_AudioSpec *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(desired, obtained);
    }
    
    virtual void SDL_PauseAudio(int pause_on)
    {
      static const char NAME[] = "SDL_PauseAudio";
      typedef void ( *FunctionType)(int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(pause_on);
    }
    
    virtual void SDL_CloseAudio(void)
    {
      static const char NAME[] = "SDL_CloseAudio";
      typedef void ( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

}

namespace Sound
{
  namespace Sdl
  {
    Api::Ptr LoadDynamicApi()
    {
      static const SdlName NAME;
      const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
      return boost::make_shared<DynamicApi>(lib);
    }
  }
}
