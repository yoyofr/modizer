/**
*
* @file
*
* @brief  Parameters containers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <parameters/container.h>
//std includes
#include <map>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace Parameters;

  template<class T>
  bool FindByName(const std::map<NameType, T>& map, const NameType& name, T& res)
  {
    const typename std::map<NameType, T>::const_iterator it = map.find(name);
    return it != map.end()
      ? (res = it->second, true)
      : false;
  }

  class StorageContainer : public Container
  {
  public:
    StorageContainer()
      : VersionValue(0)
    {
    }

    //accessor virtuals
    virtual uint_t Version() const
    {
      return VersionValue;
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return FindByName(Integers, name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return FindByName(Strings, name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return FindByName(Datas, name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      for (IntegerMap::const_iterator it = Integers.begin(), lim = Integers.end(); it != lim; ++it)
      {
        visitor.SetValue(it->first, it->second);
      }
      for (StringMap::const_iterator it = Strings.begin(), lim = Strings.end(); it != lim; ++it)
      {
        visitor.SetValue(it->first, it->second);
      }
      for (DataMap::const_iterator it = Datas.begin(), lim = Datas.end(); it != lim; ++it)
      {
        visitor.SetValue(it->first, it->second);
      }
    }

    //visitor virtuals
    virtual void SetValue(const NameType& name, IntType val)
    {
      if (Set(Integers[name], val) | Strings.erase(name) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      if (Integers.erase(name) | Set(Strings[name], val) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      if (Integers.erase(name) | Strings.erase(name) | Set(Datas[name], val))
      {
        ++VersionValue;
      }
    }

    //modifier virtuals
    virtual void RemoveValue(const NameType& name)
    {
      if (Integers.erase(name) | Strings.erase(name) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }
  private:
    template<class Type>
    static std::size_t Set(Type& dst, const Type& src)
    {
      if (dst != src)
      {
        dst = src;
        return 1;
      }
      else
      {
        return 0;
      }
    }

    static std::size_t Set(DataType& dst, const DataType& src)
    {
      dst = src;
      return 1;
    }
  private:
    uint_t VersionValue;
    typedef std::map<NameType, IntType> IntegerMap;
    typedef std::map<NameType, StringType> StringMap;
    typedef std::map<NameType, DataType> DataMap;
    IntegerMap Integers;
    StringMap Strings;
    DataMap Datas;
  };

  class CompositeContainer : public Container
  {
  public:
    CompositeContainer(Accessor::Ptr accessor, Modifier::Ptr modifier)
      : AccessDelegate(accessor)
      , ModifyDelegate(modifier)
    {
    }

    //accessor virtuals
    virtual uint_t Version() const
    {
      return AccessDelegate->Version();
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return AccessDelegate->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return AccessDelegate->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return AccessDelegate->FindValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      return AccessDelegate->Process(visitor);
    }

    //visitor virtuals
    virtual void SetValue(const NameType& name, IntType val)
    {
      return ModifyDelegate->SetValue(name, val);
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      return ModifyDelegate->SetValue(name, val);
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      return ModifyDelegate->SetValue(name, val);
    }

    //modifier virtuals
    virtual void RemoveValue(const NameType& name)
    {
      return ModifyDelegate->RemoveValue(name);
    }
  private:
    const Accessor::Ptr AccessDelegate;
    const Modifier::Ptr ModifyDelegate;
  };
}

namespace Parameters
{
  Container::Ptr Container::Create()
  {
    return boost::make_shared<StorageContainer>();
  }

  Container::Ptr Container::CreateAdapter(Accessor::Ptr accessor, Modifier::Ptr modifier)
  {
    return boost::make_shared<CompositeContainer>(accessor, modifier);
  }
}
