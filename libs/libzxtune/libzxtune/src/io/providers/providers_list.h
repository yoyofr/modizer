/**
*
* @file
*
* @brief  Supported providers list
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

namespace IO
{
  class ProvidersEnumerator;

  //forward declarations of supported providers
  void RegisterFileProvider(ProvidersEnumerator& enumerator);
#ifndef EMSCRIPTEN
  void RegisterNetworkProvider(ProvidersEnumerator& enumerator);
#endif
  inline void RegisterProviders(ProvidersEnumerator& enumerator)
  {
    RegisterFileProvider(enumerator);
#ifndef EMSCRIPTEN
    RegisterNetworkProvider(enumerator);
#endif
  }
}
