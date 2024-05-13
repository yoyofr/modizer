/**
*
* @file
*
* @brief  Shared library adapter
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <platform/shared_library.h>
//std includes
#include <map>

namespace Platform
{
  class SharedLibraryAdapter
  {
  public:
    explicit SharedLibraryAdapter(SharedLibrary::Ptr lib)
      : Library(lib)
    {
    }

    template<class F>
    F GetSymbol(const char* name) const
    {
      const NameToSymbol::const_iterator it = Symbols.find(name);
      if (it != Symbols.end())
      {
        return reinterpret_cast<F>(it->second);
      }
      void* const symbol = Library->GetSymbol(name);
      Symbols.insert(NameToSymbol::value_type(name, symbol));
      return reinterpret_cast<F>(symbol);
    }
  private:
    const SharedLibrary::Ptr Library;
    typedef std::map<const char*, void*> NameToSymbol;
    mutable NameToSymbol Symbols;
  };
}
