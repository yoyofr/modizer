/**
*
* @file
*
* @brief  Shared library interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <string>
#include <vector>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Platform
{
  class SharedLibrary
  {
  public:
    typedef boost::shared_ptr<const SharedLibrary> Ptr;
    virtual ~SharedLibrary() {}

    virtual void* GetSymbol(const std::string& name) const = 0;

    //! @param name Library name without platform-dependent prefixes and extension
    // E.g. Load("SDL") will try to load "libSDL.so" for Linux and and "SDL.dll" for Windows
    // If platform-dependent full filename is specified, no substitution is made
    static Ptr Load(const std::string& name);
    
    class Name
    {
    public:
      virtual ~Name() {}
      
      virtual std::string Base() const = 0;
      virtual std::vector<std::string> PosixAlternatives() const = 0;
      virtual std::vector<std::string> WindowsAlternatives() const = 0;
    };
    
    static Ptr Load(const Name& name);
  };
}
