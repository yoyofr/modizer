/**
* 
* @file
*
* @brief  Factories of different container plugins
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include <core/plugins/registrator.h>

namespace ZXTune
{
  void RegisterRawContainer(ArchivePluginsRegistrator& registrator);
  void RegisterSidContainer(ArchivePluginsRegistrator& registrator);
  void RegisterAyContainer(ArchivePluginsRegistrator& registrator);
  void RegisterZdataContainer(ArchivePluginsRegistrator& registrator);
  void RegisterArchiveContainers(ArchivePluginsRegistrator& registrator);
}
