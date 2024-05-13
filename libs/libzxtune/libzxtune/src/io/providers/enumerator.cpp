/**
*
* @file
*
* @brief  Providers enumerator implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "enumerator.h"
#include "providers_list.h"
//common includes
#include <error_tools.h>
//library includes
#include <debug/log.h>
#include <io/api.h>
#include <l10n/api.h>
//std includes
#include <algorithm>
#include <list>
#include <map>
//boost includes
#include <boost/make_shared.hpp>

#define FILE_TAG 03113EE3

namespace
{
  const Debug::Stream Dbg("IO::Enumerator");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("io");
}

namespace IO
{
  typedef std::vector<DataProvider::Ptr> ProvidersList;

  //implementation of IO providers enumerator
  class ProvidersEnumeratorImpl : public ProvidersEnumerator
  {
  public:
    ProvidersEnumeratorImpl()
    {
      RegisterProviders(*this);
    }

    virtual void RegisterProvider(DataProvider::Ptr provider)
    {
      Providers.push_back(provider);
      Dbg("Registered provider '%1%'", provider->Id());
      const Strings::Set& schemes = provider->Schemes();
      for (Strings::Set::const_iterator it = schemes.begin(), lim = schemes.end(); it != lim; ++it)
      {
        Schemes.insert(std::make_pair(*it, provider));
      }
    }

    virtual Identifier::Ptr ResolveUri(const String& uri) const
    {
      Dbg("Resolving uri '%1%'", uri);
      if (const Identifier::Ptr id = Resolve(uri))
      {
        return id;
      }
      Dbg(" No suitable provider found");
      throw MakeFormattedError(THIS_LINE, translate("Failed to resolve uri '%1%'."), uri);
    }

    virtual Binary::Container::Ptr OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb) const
    {
      Dbg("Opening path '%1%'", path);
      if (Identifier::Ptr id = Resolve(path))
      {
        if (const DataProvider* provider = FindProvider(id->Scheme()))
        {
          Dbg(" Used provider '%1%'", provider->Id());
          return provider->Open(id->Path(), params, cb);
        }
      }
      Dbg(" No suitable provider found");
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    virtual Binary::OutputStream::Ptr CreateStream(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb) const
    {
      Dbg("Creating stream '%1%'", path);
      if (Identifier::Ptr id = Resolve(path))
      {
        if (const DataProvider* provider = FindProvider(id->Scheme()))
        {
          Dbg(" Used provider '%1%'", provider->Id());
          //pass nonchanged parameter to lower level
          return provider->Create(path, params, cb);
        }
      }
      Dbg(" No suitable provider found");
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    virtual Provider::Iterator::Ptr Enumerate() const
    {
      return Provider::Iterator::Ptr(new RangedObjectIteratorAdapter<ProvidersList::const_iterator, Provider::Ptr>(Providers.begin(), Providers.end()));
    }
  private:
    Identifier::Ptr Resolve(const String& uri) const
    {
      for (ProvidersList::const_iterator it = Providers.begin(), lim = Providers.end(); it != lim; ++it)
      {
        if (const Identifier::Ptr res = (*it)->Resolve(uri))
        {
          return res;
        }
      }
      return Identifier::Ptr();
    }

    const DataProvider* FindProvider(const String& scheme) const
    {
      const SchemeToProviderMap::const_iterator it = Schemes.find(scheme);
      return it != Schemes.end()
        ? it->second.get()
        : 0;
    }
  private:
    ProvidersList Providers;
    typedef std::map<String, DataProvider::Ptr> SchemeToProviderMap;
    SchemeToProviderMap Schemes;
  };

  class UnavailableProvider : public DataProvider
  {
  public:
    UnavailableProvider(const String& id, const char* descr, const Error& status)
      : IdValue(id)
      , DescrValue(descr)
      , StatusValue(status)
    {
    }

    virtual String Id() const
    {
      return IdValue;
    }

    virtual String Description() const
    {
      return translate(DescrValue);
    }

    virtual Error Status() const
    {
      return StatusValue;
    }

    virtual bool Check(const String&) const
    {
      return false;
    }

    virtual Binary::Container::Ptr Open(const String&, const Parameters::Accessor&, Log::ProgressCallback&) const
    {
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    virtual Binary::OutputStream::Ptr Create(const String&, const Parameters::Accessor&, Log::ProgressCallback&) const
    {
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    virtual Strings::Set Schemes() const
    {
      return Strings::Set();
    }

    virtual Identifier::Ptr Resolve(const String& /*uri*/) const
    {
      return Identifier::Ptr();
    }
  private:
    const String IdValue;
    const char* const DescrValue;
    const Error StatusValue;
  };
}

namespace IO
{
  ProvidersEnumerator& ProvidersEnumerator::Instance()
  {
    static ProvidersEnumeratorImpl instance;
    return instance;
  }

  Identifier::Ptr ResolveUri(const String& uri)
  {
    return ProvidersEnumerator::Instance().ResolveUri(uri);
  }

  Binary::Container::Ptr OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb)
  {
    return ProvidersEnumerator::Instance().OpenData(path, params, cb);
  }

  Binary::OutputStream::Ptr CreateStream(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb)
  {
    return ProvidersEnumerator::Instance().CreateStream(path, params, cb);
  }

  Provider::Iterator::Ptr EnumerateProviders()
  {
    return ProvidersEnumerator::Instance().Enumerate();
  }

  DataProvider::Ptr CreateDisabledProviderStub(const String& id, const char* description)
  {
    return CreateUnavailableProviderStub(id, description, Error(THIS_LINE, translate("Not supported in current configuration")));
  }

  DataProvider::Ptr CreateUnavailableProviderStub(const String& id, const char* description, const Error& status)
  {
    return boost::make_shared<UnavailableProvider>(id, description, status);
  }
}
