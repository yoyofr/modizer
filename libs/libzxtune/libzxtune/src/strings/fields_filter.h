/**
*
* @file
*
* @brief  Fields source filter adapter
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <contract.h>
//library includes
#include <strings/fields.h>
//std includes
#include <set>
//boost includes
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_unsigned.hpp>

namespace Strings
{
  class FilterFieldsSource : public FieldsSource
  {
  public:
    FilterFieldsSource(const FieldsSource& delegate, const String& src, const String& tgt)
      : Delegate(delegate)
      , Table(1 << (8 * sizeof(Char)))
    {
      BOOST_STATIC_ASSERT(boost::is_unsigned<Char>::value);
      Require(src.size() == tgt.size());
      Require(std::set<Char>(src.begin(), src.end()).size() == src.size());
      for (std::size_t idx = 0; idx != Table.size(); ++idx)
      {
        const String::size_type srcPos = src.find(idx);
        Table[idx] = srcPos != src.npos
          ? tgt[srcPos]
          : idx;
      }
    }

    FilterFieldsSource(const FieldsSource& delegate, const String& src, const Char tgt)
      : Delegate(delegate)
      , Table(1 << (8 * sizeof(Char)))
    {
      BOOST_STATIC_ASSERT(boost::is_unsigned<Char>::value);
      const std::set<Char> srcSymbols(src.begin(), src.end());
      Require(srcSymbols.size() == src.size());
      for (std::size_t idx = 0; idx != Table.size(); ++idx)
      {
        Table[idx] = srcSymbols.count(idx)
          ? tgt
          : idx;
      }
    }

    virtual String GetFieldValue(const String& fieldName) const
    {
      String val = Delegate.GetFieldValue(fieldName);
      for (String::iterator it = val.begin(), lim = val.end(); it != lim; ++it)
      {
        *it = Table[*it];
      }
      return val;
    }
  private:
    const FieldsSource& Delegate;
    std::vector<Char> Table;
  };
}
