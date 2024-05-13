/**
* 
* @file
*
* @brief  SampleTracker support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "dac_plugin.h"
#include "digital.h"
#include "core/plugins/registrator.h"
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/sampletracker.h>

namespace Module
{
namespace SampleTracker
{
  const std::size_t CHANNELS_COUNT = 3;

  typedef DAC::ModuleData ModuleData;
  typedef DAC::DataBuilder DataBuilder;

  /*
    0x0016 0x0017 0x0019 0x001a 0x001c 0x001e 0x001f 0x0021 0x0023 0x0025 0x0027 0x002a
    0x002c 0x002f 0x0032 0x0035 0x0038 0x003b 0x003f 0x0042 0x0046 0x004b 0x004f 0x0054
    0x0059 0x005e 0x0064 0x0069 0x0070 0x0076 0x007d 0x0085 0x008d 0x0095 0x009e 0x00a7
    0x00b1 0x00bc 0x00c7 0x00d3 0x00df 0x00ed 0x00fb 0x010a 0x0119 0x012a 0x013c 0x014f
    0x0163 0x0178 0x018e 0x01a6 0x01bf 0x01d9 0x01f6 0x0213 0x0233 0x0254 0x0278 0x029d
    0x02c5 0x02ef 0x031c 0x034b 0x0373 0x03b3 0x03eb 0x0427 0x0466 0x04a9 0x04f0 0x053b
  */

  class Factory : public DAC::Factory
  {
  public:
    virtual DAC::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      const std::auto_ptr<DataBuilder> dataBuilder = DataBuilder::Create<CHANNELS_COUNT>(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SampleTracker::Parse(rawData, *dataBuilder))
      {
        propBuilder.SetSource(*container);
        return boost::make_shared<DAC::SimpleChiptune>(dataBuilder->GetResult(), propBuilder.GetResult(), CHANNELS_COUNT);
      }
      else
      {
        return DAC::Chiptune::Ptr();
      }
    }
  };
}
}

namespace ZXTune
{
  void RegisterSTRSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'S', 'T', 'R', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSampleTrackerDecoder();
    const Module::DAC::Factory::Ptr factory = boost::make_shared<Module::SampleTracker::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
