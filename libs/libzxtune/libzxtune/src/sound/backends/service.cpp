/**
*
* @file
*
* @brief  Sound service implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "backends_list.h"
#include "backend_impl.h"
#include "storage.h"
//common includes
#include <error_tools.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
#include <parameters/merged_accessor.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/service.h>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/make_shared.hpp>

#define FILE_TAG A6428476

namespace
{
  const Debug::Stream Dbg("Sound::Backend");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
  class StaticBackendInformation : public BackendInformation
  {
  public:
    StaticBackendInformation(const String& id, const char* descr, uint_t caps, const Error& status)
      : IdValue(id)
      , DescrValue(descr)
      , CapsValue(caps)
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

    virtual uint_t Capabilities() const
    {
      return CapsValue;
    }

    virtual Error Status() const
    {
      return StatusValue;
    }
  private:
    const String IdValue;
    const char* const DescrValue;
    const uint_t CapsValue;
    const Error StatusValue;
  };

  class ServiceImpl : public Service, public BackendsStorage
  {
  public:
    explicit ServiceImpl(Parameters::Accessor::Ptr options)
      : Options(options)
    {
    }

    virtual BackendInformation::Iterator::Ptr EnumerateBackends() const
    {
      return CreateRangedObjectIteratorAdapter(Infos.begin(), Infos.end());
    }

    virtual Strings::Array GetAvailableBackends() const
    {
      const Strings::Array order = GetOrder();
      Strings::Array available = GetAvailable();
      Strings::Array result;
      for (Strings::Array::const_iterator it = order.begin(), lim = order.end(); it != lim; ++it)
      {
        const Strings::Array::iterator avIt = std::find(available.begin(), available.end(), *it);
        if (avIt != available.end())
        {
          result.push_back(*avIt);
          available.erase(avIt);
        }
      }
      std::copy(available.begin(), available.end(), std::back_inserter(result));
      return result;
    }

    virtual Backend::Ptr CreateBackend(const String& backendId, Module::Holder::Ptr module, BackendCallback::Ptr callback) const
    {
      try
      {
        if (const BackendWorkerFactory::Ptr factory = FindFactory(backendId))
        {
          const Parameters::Accessor::Ptr params = Parameters::CreateMergedAccessor(module->GetModuleProperties(), Options);
          return Sound::CreateBackend(params, module, callback, factory->CreateWorker(params, module));
        }
        throw MakeFormattedError(THIS_LINE, translate("Backend '%1%' not registered."), backendId);
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE,
          translate("Failed to create backend '%1%'."), backendId).AddSuberror(e);
      }
    }

    virtual void Register(const String& id, const char* description, uint_t caps, BackendWorkerFactory::Ptr factory)
    {
      Factories.push_back(FactoryWithId(id, factory));
      const BackendInformation::Ptr info = boost::make_shared<StaticBackendInformation>(id, description, caps, Error());
      Infos.push_back(info);
      Dbg("Service(%1%): Registered backend %2%", this, id);
    }

    virtual void Register(const String& id, const char* description, uint_t caps, const Error& status)
    {
      const BackendInformation::Ptr info = boost::make_shared<StaticBackendInformation>(id, description, caps, status);
      Infos.push_back(info);
      Dbg("Service(%1%): Registered disabled backend %2%", this, id);
    }

    virtual void Register(const String& id, const char* description, uint_t caps)
    {
      const Error status = Error(THIS_LINE, translate("Not supported in current configuration"));
      const BackendInformation::Ptr info = boost::make_shared<StaticBackendInformation>(id, description, caps, status);
      Infos.push_back(info);
      Dbg("Service(%1%): Registered stub backend %2%", this, id);
    }
  private:
    Strings::Array GetOrder() const
    {
      Parameters::StringType order;
      Options->FindValue(Parameters::ZXTune::Sound::Backends::ORDER, order);
      Strings::Array orderArray;
      boost::algorithm::split(orderArray, order, !boost::algorithm::is_alnum());
      return orderArray;
    }

    Strings::Array GetAvailable() const
    {
      Strings::Array ids;
      for (std::vector<BackendInformation::Ptr>::const_iterator it = Infos.begin(), lim = Infos.end(); it != lim; ++it)
      {
        const BackendInformation::Ptr info = *it;
        if (!info->Status())
        {
          ids.push_back(info->Id());
        }
      }
      return ids;
    }

    BackendWorkerFactory::Ptr FindFactory(const String& id) const
    {
      const std::vector<FactoryWithId>::const_iterator it = std::find(Factories.begin(), Factories.end(), id);
      return it != Factories.end()
        ? it->Factory
        : BackendWorkerFactory::Ptr();
    }
  private:
    const Parameters::Accessor::Ptr Options;
    std::vector<BackendInformation::Ptr> Infos;
    struct FactoryWithId
    {
      String Id;
      BackendWorkerFactory::Ptr Factory;

      FactoryWithId()
      {
      }

      FactoryWithId(const String& id, BackendWorkerFactory::Ptr factory)
        : Id(id)
        , Factory(factory)
      {
      }

      bool operator == (const String& id) const
      {
        return Id == id;
      }
    };
    std::vector<FactoryWithId> Factories;
  };

  Service::Ptr CreateSystemService(Parameters::Accessor::Ptr options)
  {
    const boost::shared_ptr<ServiceImpl> result = boost::make_shared<ServiceImpl>(options);
    RegisterSystemBackends(*result);
    return result;
  }

  Service::Ptr CreateFileService(Parameters::Accessor::Ptr options)
  {
    const boost::shared_ptr<ServiceImpl> result = boost::make_shared<ServiceImpl>(options);
    RegisterFileBackends(*result);
    return result;
  }

  Service::Ptr CreateGlobalService(Parameters::Accessor::Ptr options)
  {
    const boost::shared_ptr<ServiceImpl> result = boost::make_shared<ServiceImpl>(options);
    RegisterAllBackends(*result);
    return result;
  }
}
