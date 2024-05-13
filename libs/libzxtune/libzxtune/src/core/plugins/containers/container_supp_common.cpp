/**
* 
* @file
*
* @brief  Container plugin implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container_supp_common.h"
#include "core/src/callback.h"
#include "core/plugins/utils.h"
//library includes
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <strings/format.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

namespace
{
  const Debug::Stream Dbg("Core::ArchivesSupp");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");
}

namespace ZXTune
{
  class LoggerHelper
  {
  public:
    LoggerHelper(uint_t total, const Module::DetectCallback& delegate, const String& plugin, const String& path)
      : Total(total)
      , Delegate(delegate)
      , Progress(CreateProgressCallback(delegate, Total))
      , Id(plugin)
      , Path(path)
      , Current()
    {
    }

    void operator()(const Formats::Archived::File& cur)
    {
      if (Progress.get())
      {
        const String text = ProgressMessage(Id, Path, cur.GetName());
        Progress->OnProgress(Current, text);
      }
    }

    std::auto_ptr<Module::DetectCallback> CreateNestedCallback() const
    {
      Log::ProgressCallback* const parentProgress = Delegate.GetProgress();
      if (parentProgress && Total < 50)
      {
        Log::ProgressCallback::Ptr nestedProgress = CreateNestedPercentProgressCallback(Total, Current, *parentProgress);
        return std::auto_ptr<Module::DetectCallback>(new Module::CustomProgressDetectCallbackAdapter(Delegate, nestedProgress));
      }
      else
      {
        return std::auto_ptr<Module::DetectCallback>(new Module::CustomProgressDetectCallbackAdapter(Delegate));
      }
    }

    void Next()
    {
      ++Current;
    }
  private:
    const uint_t Total;
    const Module::DetectCallback& Delegate;
    const Log::ProgressCallback::Ptr Progress;
    const String Id;
    const String Path;
    uint_t Current;
  };

  class ContainerDetectCallback : public Formats::Archived::Container::Walker
  {
  public:
    ContainerDetectCallback(std::size_t maxSize, const String& plugin, DataLocation::Ptr location, uint_t count, const Module::DetectCallback& callback)
      : MaxSize(maxSize)
      , BaseLocation(location)
      , SubPlugin(plugin)
      , Logger(count, callback, SubPlugin, BaseLocation->GetPath()->AsString())
    {
    }

    void OnFile(const Formats::Archived::File& file) const
    {
      const String& name = file.GetName();
      const std::size_t size = file.GetSize();
      if (size <= MaxSize)
      {
        ProcessFile(file);
      }
      else
      {
        Dbg("'%1%' is too big (%1%). Skipping.", name, size);
      }
    }
  private:
    void ProcessFile(const Formats::Archived::File& file) const
    {
      Logger(file);
      if (const Binary::Container::Ptr subData = file.GetData())
      {
        const String subPath = file.GetName();
        const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(BaseLocation, subData, SubPlugin, subPath);
        const std::auto_ptr<Module::DetectCallback> nestedProgressCallback = Logger.CreateNestedCallback();
        Module::Detect(subLocation, *nestedProgressCallback);
      }
      Logger.Next();
    }
  private:
    const std::size_t MaxSize;
    const DataLocation::Ptr BaseLocation;
    const String SubPlugin;
    mutable LoggerHelper Logger;
  };

  class ArchivedContainerPlugin : public ArchivePlugin
  {
  public:
    ArchivedContainerPlugin(Plugin::Ptr descr, Formats::Archived::Decoder::Ptr decoder)
      : Description(descr)
      , Decoder(decoder)
      , SupportDirectories(0 != (Description->Capabilities() & CAP_STOR_DIRS))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
     const Binary::Container::Ptr rawData = input->GetData();
      if (const Formats::Archived::Container::Ptr archive = Decoder->Decode(*rawData))
      {
        if (const uint_t count = archive->CountFiles())
        {
          ContainerDetectCallback detect(~std::size_t(0), Description->Id(), input, count, callback);
          archive->ExploreFiles(detect);
        }
        return Analysis::CreateMatchedResult(archive->Size());
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const Analysis::Path& inPath) const
    {
      const Binary::Container::Ptr rawData = location->GetData();
      if (const Formats::Archived::Container::Ptr archive = Decoder->Decode(*rawData))
      {
        if (const Formats::Archived::File::Ptr fileToOpen = FindFile(*archive, inPath))
        {
          if (const Binary::Container::Ptr subData = fileToOpen->GetData())
          {
            return CreateNestedLocation(location, subData, Description->Id(), fileToOpen->GetName());
          }
        }
      }
      return DataLocation::Ptr();
    }
  private:
    Formats::Archived::File::Ptr FindFile(const Formats::Archived::Container& container, const Analysis::Path& path) const
    {
     Analysis::Path::Ptr resolved = Analysis::ParsePath(String(), Text::MODULE_SUBPATH_DELIMITER[0]);
      for (const Analysis::Path::Iterator::Ptr components = path.GetIterator();
           components->IsValid(); components->Next())
      {
        resolved = resolved->Append(components->Get());
        const String filename = resolved->AsString();

        Dbg("Trying '%1%'", filename);
        if (Formats::Archived::File::Ptr file = container.FindFile(filename))
        {
          Dbg("Found");
          return file;
        }
        if (!SupportDirectories)
        {
          break;
        }
      }
      return Formats::Archived::File::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const Formats::Archived::Decoder::Ptr Decoder;
    const bool SupportDirectories;
  };
}

namespace ZXTune
{
  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, uint_t caps, Formats::Archived::Decoder::Ptr decoder)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, decoder->GetDescription(), caps);
    return ArchivePlugin::Ptr(new ArchivedContainerPlugin(description, decoder));
  }

  String ProgressMessage(const String& id, const String& path)
  {
    return path.empty()
      ? Strings::Format(translate("%1% processing"), id)
      : Strings::Format(translate("%1% processing at %2%"), id, path);
  }

  String ProgressMessage(const String& id, const String& path, const String& element)
  {
    return path.empty()
      ? Strings::Format(translate("%1% processing for %2%"), id, element)
      : Strings::Format(translate("%1% processing for %2% at %3%"), id, element, path);
  }
}
