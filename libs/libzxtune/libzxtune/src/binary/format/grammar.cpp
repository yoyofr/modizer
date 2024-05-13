/**
*
* @file
*
* @brief  Format grammar
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "grammar.h"
//common includes
#include <pointers.h>
//std includes
#include <cctype>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace Binary;

  const std::string BINDIGITS("01");
  const std::string DIGITS = BINDIGITS + "23456789";
  const std::string HEXDIGITS = DIGITS + "abcdefABCDEF";

  class SpaceDelimitersTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      static const std::string SPACES(" \n\t\r");
      return lexeme.empty() || lexeme.npos != lexeme.find_first_not_of(SPACES)
        ? LexicalAnalysis::INVALID_TOKEN
        : DELIMITER;
    }
  };

  class SymbolDelimitersTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      static const char DELIMITERS[] = {DELIMITER_TEXT, 0};
      return lexeme.size() != 1 || lexeme.npos != lexeme.find_first_not_of(DELIMITERS)
        ? LexicalAnalysis::INVALID_TOKEN
        : DELIMITER;
    }
  };

  class DecimalNumbersTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      return lexeme.empty() || lexeme.npos != lexeme.find_first_not_of(DIGITS)
        ? LexicalAnalysis::INVALID_TOKEN
        : CONSTANT;
    }
  };

  class CharacterTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      return lexeme.empty() || lexeme[0] != SYMBOL_TEXT
        ? LexicalAnalysis::INVALID_TOKEN
        : (2 == lexeme.size() ? CONSTANT : LexicalAnalysis::INCOMPLETE_TOKEN);
    }
  };

  class AnyMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      static const char MASKS[] = {ANY_BYTE_TEXT, 0};
      return lexeme.size() != 1 || lexeme.npos != lexeme.find_first_not_of(MASKS)
        ? LexicalAnalysis::INVALID_TOKEN
        : MASK;
    }
  };

  class BinaryMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      static const std::string ANY_BITS = std::string(1, ANY_BIT_TEXT) + char(std::toupper(ANY_BIT_TEXT));
      static const std::string BITMATCHES = BINDIGITS + ANY_BITS;
      const std::size_t SIZE = 9;
      if (lexeme.empty() || lexeme[0] != BINARY_MASK_TEXT || lexeme.npos != lexeme.find_first_not_of(BITMATCHES, 1) || lexeme.size() > SIZE)
      {
        return LexicalAnalysis::INVALID_TOKEN;
      }
      else if (lexeme.size() == SIZE)
      {
        return lexeme.npos == lexeme.find_first_of(ANY_BITS) ? CONSTANT : MASK;
      }
      else
      {
        return LexicalAnalysis::INCOMPLETE_TOKEN;
      }
    }
  };

  class HexadecimalMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      static const std::string ANY_NIBBLES = std::string(1, ANY_NIBBLE_TEXT) + char(std::toupper(ANY_NIBBLE_TEXT));
      static const std::string DIGITS = HEXDIGITS + ANY_NIBBLES;
      if (lexeme.empty() || lexeme.npos != lexeme.find_first_not_of(DIGITS))
      {
        return LexicalAnalysis::INVALID_TOKEN;
      }
      else if (0 == lexeme.size() % 2)
      {
        return lexeme.npos == lexeme.find_first_of(ANY_NIBBLES) ? CONSTANT : MASK;
      }
      else
      {
        return LexicalAnalysis::INCOMPLETE_TOKEN;
      }
    }
  };

  class MultiplicityMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      if (lexeme.empty() || lexeme[0] != MULTIPLICITY_TEXT)
      {
        return LexicalAnalysis::INVALID_TOKEN;
      }
      else if (lexeme.size() == 1)
      {
        return LexicalAnalysis::INCOMPLETE_TOKEN;
      }
      else
      {
        return lexeme.npos != lexeme.find_first_not_of(DIGITS, 1)
          ? LexicalAnalysis::INVALID_TOKEN
          : MASK;
      }
    }
  };

  class OperationTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    virtual LexicalAnalysis::TokenType Parse(const std::string& lexeme) const
    {
      static const char OPERATIONS[] = {RANGE_TEXT, CONJUNCTION_TEXT, DISJUNCTION_TEXT,
        QUANTOR_BEGIN, QUANTOR_END, GROUP_BEGIN, GROUP_END, 0};
      return lexeme.size() != 1 || lexeme.npos != lexeme.find_first_not_of(OPERATIONS)
        ? LexicalAnalysis::INVALID_TOKEN
        : OPERATION;
    }
  };

  class FormatGrammar : public LexicalAnalysis::Grammar
  {
  public:
    FormatGrammar()
      : Delegate(LexicalAnalysis::CreateContextIndependentGrammar())
    {
      Delegate->AddTokenizer(boost::make_shared<SpaceDelimitersTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<SymbolDelimitersTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<DecimalNumbersTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<CharacterTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<AnyMaskTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<BinaryMaskTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<HexadecimalMaskTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<MultiplicityMaskTokenizer>());
      Delegate->AddTokenizer(boost::make_shared<OperationTokenizer>());
    }

    void AddTokenizer(LexicalAnalysis::Tokenizer::Ptr tokenizer)
    {
      return Delegate->AddTokenizer(tokenizer);
    }

    virtual void Analyse(const std::string& notation, LexicalAnalysis::Grammar::Callback& cb) const
    {
      return Delegate->Analyse(notation, cb);
    }
  private:
    const LexicalAnalysis::Grammar::RWPtr Delegate;
  };
}

namespace Binary
{
  LexicalAnalysis::Grammar::Ptr CreateFormatGrammar()
  {
    static FormatGrammar grammar;
    return MakeSingletonPointer(grammar);
  }
}
