/**
*
* @file
*
* @brief    Localization support API
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace L10n
{
  /*
    @brief Base translation unit
  */
  class Vocabulary
  {
  public:
    typedef boost::shared_ptr<const Vocabulary> Ptr;
    virtual ~Vocabulary() {}

    //! @brief Retreiving translated or converted text message
    virtual String GetText(const char* text) const = 0;
    /*
      @brief Retreiving tralslated or converted text message according to count
      @note Common algorithm is:
        if (Translation.Available)
          return Translation.GetPluralFor(count)
        else
          return count == 1 ? single : plural;
    */
    virtual String GetText(const char* single, const char* plural, int count) const = 0;

    //! @brief Same functions with context specified
    //! @note Specified design is limited by messages' collecting utilities workflow (context should be specified exactly near the message)
    virtual String GetText(const char* context, const char* text) const = 0;
    virtual String GetText(const char* context, const char* single, const char* plural, int count) const = 0;
  };

  struct Translation
  {
    std::string Domain;
    std::string Language;
    std::string Type;
    Dump Data;
  };

  class Library
  {
  public:
    virtual ~Library() {}

    virtual void AddTranslation(const Translation& trans) = 0;

    virtual void SelectTranslation(const std::string& translation) = 0;

    virtual Vocabulary::Ptr GetVocabulary(const std::string& domain) const = 0;

    static Library& Instance();
  };
}
