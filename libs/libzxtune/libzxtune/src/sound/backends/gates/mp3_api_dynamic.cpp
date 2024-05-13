/**
*
* @file
*
* @brief  MP3 subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "mp3_api.h"
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/range/end.hpp>

namespace
{
  using namespace Sound::Mp3;

  class LameName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "mp3lame";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libmp3lame.so.0",
      };
      return std::vector<std::string>(ALTERNATIVES, boost::end(ALTERNATIVES));
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libmp3lame.dll",
      };
      return std::vector<std::string>(ALTERNATIVES, boost::end(ALTERNATIVES));
    }
  };

  const Debug::Stream Dbg("Sound::Backend::Mp3");

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

    
    virtual const char* get_lame_version()
    {
      static const char NAME[] = "get_lame_version";
      typedef const char* ( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual lame_t lame_init()
    {
      static const char NAME[] = "lame_init";
      typedef lame_t ( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual int lame_close(lame_t ctx)
    {
      static const char NAME[] = "lame_close";
      typedef int ( *FunctionType)(lame_t);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx);
    }
    
    virtual int lame_set_in_samplerate(lame_t ctx, int rate)
    {
      static const char NAME[] = "lame_set_in_samplerate";
      typedef int ( *FunctionType)(lame_t, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, rate);
    }
    
    virtual int lame_set_out_samplerate(lame_t ctx, int rate)
    {
      static const char NAME[] = "lame_set_out_samplerate";
      typedef int ( *FunctionType)(lame_t, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, rate);
    }
    
    virtual int lame_set_bWriteVbrTag(lame_t ctx, int flag)
    {
      static const char NAME[] = "lame_set_bWriteVbrTag";
      typedef int ( *FunctionType)(lame_t, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, flag);
    }
    
    virtual int lame_set_mode(lame_t ctx, MPEG_mode mode)
    {
      static const char NAME[] = "lame_set_mode";
      typedef int ( *FunctionType)(lame_t, MPEG_mode);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, mode);
    }
    
    virtual int lame_set_num_channels(lame_t ctx, int chans)
    {
      static const char NAME[] = "lame_set_num_channels";
      typedef int ( *FunctionType)(lame_t, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, chans);
    }
    
    virtual int lame_set_brate(lame_t ctx, int brate)
    {
      static const char NAME[] = "lame_set_brate";
      typedef int ( *FunctionType)(lame_t, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, brate);
    }
    
    virtual int lame_set_VBR(lame_t ctx, vbr_mode mode)
    {
      static const char NAME[] = "lame_set_VBR";
      typedef int ( *FunctionType)(lame_t, vbr_mode);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, mode);
    }
    
    virtual int lame_set_VBR_q(lame_t ctx, int quality)
    {
      static const char NAME[] = "lame_set_VBR_q";
      typedef int ( *FunctionType)(lame_t, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, quality);
    }
    
    virtual int lame_set_VBR_mean_bitrate_kbps(lame_t ctx, int brate)
    {
      static const char NAME[] = "lame_set_VBR_mean_bitrate_kbps";
      typedef int ( *FunctionType)(lame_t, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, brate);
    }
    
    virtual int lame_init_params(lame_t ctx)
    {
      static const char NAME[] = "lame_init_params";
      typedef int ( *FunctionType)(lame_t);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx);
    }
    
    virtual int lame_encode_buffer_interleaved(lame_t ctx, short int* pcm, int samples, unsigned char* dst, int dstSize)
    {
      static const char NAME[] = "lame_encode_buffer_interleaved";
      typedef int ( *FunctionType)(lame_t, short int*, int, unsigned char*, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, pcm, samples, dst, dstSize);
    }
    
    virtual int lame_encode_flush(lame_t ctx, unsigned char* dst, int dstSize)
    {
      static const char NAME[] = "lame_encode_flush";
      typedef int ( *FunctionType)(lame_t, unsigned char*, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, dst, dstSize);
    }
    
    virtual void id3tag_init(lame_t ctx)
    {
      static const char NAME[] = "id3tag_init";
      typedef void ( *FunctionType)(lame_t);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx);
    }
    
    virtual void id3tag_add_v2(lame_t ctx)
    {
      static const char NAME[] = "id3tag_add_v2";
      typedef void ( *FunctionType)(lame_t);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx);
    }
    
    virtual void id3tag_set_title(lame_t ctx, const char* title)
    {
      static const char NAME[] = "id3tag_set_title";
      typedef void ( *FunctionType)(lame_t, const char*);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, title);
    }
    
    virtual void id3tag_set_artist(lame_t ctx, const char* artist)
    {
      static const char NAME[] = "id3tag_set_artist";
      typedef void ( *FunctionType)(lame_t, const char*);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, artist);
    }
    
    virtual void id3tag_set_comment(lame_t ctx, const char* comment)
    {
      static const char NAME[] = "id3tag_set_comment";
      typedef void ( *FunctionType)(lame_t, const char*);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(ctx, comment);
    }
    
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

}

namespace Sound
{
  namespace Mp3
  {
    Api::Ptr LoadDynamicApi()
    {
      static const LameName NAME;
      const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
      return boost::make_shared<DynamicApi>(lib);
    }
  }
}
