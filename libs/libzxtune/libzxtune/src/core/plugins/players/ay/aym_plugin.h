/**
* 
* @file
*
* @brief  AYM-based player plugin factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_factory.h"
#include "core/plugins/plugins_types.h"
//library includes
#include <formats/chiptune.h>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::AYM::Factory::Ptr factory);
}
