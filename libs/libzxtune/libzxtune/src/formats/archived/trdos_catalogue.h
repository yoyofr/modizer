/**
* 
* @file
*
* @brief  TR-DOS catalogue helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <formats/archived.h>

namespace TRDos
{
  class File : public Formats::Archived::File
  {
  public:
    typedef boost::shared_ptr<const File> Ptr;

    virtual std::size_t GetOffset() const = 0;

    static Ptr Create(Binary::Container::Ptr data, const String& name, std::size_t off, std::size_t size);
    static Ptr CreateReference(const String& name, std::size_t off, std::size_t size);
  };

  class CatalogueBuilder
  {
  public:
    typedef std::auto_ptr<CatalogueBuilder> Ptr;
    virtual ~CatalogueBuilder() {}

    virtual void SetRawData(Binary::Container::Ptr data) = 0;
    virtual void AddFile(File::Ptr file) = 0;

    virtual Formats::Archived::Container::Ptr GetResult() const = 0;

    static Ptr CreateGeneric();
    static Ptr CreateFlat();
  };
}
