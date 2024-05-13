/**
*
* @file
*
* @brief  Format syntax helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>

namespace Binary
{
  class FormatTokensVisitor
  {
  public:
    typedef std::auto_ptr<FormatTokensVisitor> Ptr;
    virtual ~FormatTokensVisitor() {}

    virtual void Match(const std::string& val) = 0;
    virtual void GroupStart() = 0;
    virtual void GroupEnd() = 0;
    virtual void Quantor(uint_t count) = 0;
    virtual void Operation(const std::string& op) = 0;
  };

  void ParseFormatNotation(const std::string& notation, FormatTokensVisitor& visitor);
  void ParseFormatNotationPostfix(const std::string& notation, FormatTokensVisitor& visitor);
  FormatTokensVisitor::Ptr CreatePostfixSynaxCheckAdapter(FormatTokensVisitor& visitor);
}
