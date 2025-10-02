
#pragma once


namespace Script { namespace mdpx {
    
    enum FuncType
    {
        FN_ASSIGN   = 1,
        FN_MULTIPLY ,
        FN_DIVIDE   ,
        FN_MODULO   ,
        FN_ADD      ,
        FN_SUB      ,
        FN_BITWISE_AND      ,
        FN_BITWISE_OR       ,
        FN_UMINUS   ,
        FN_UPLUS    ,
        
        
        EFN_IF,
        EFN_LOGICAL_AND,
        EFN_LOGICAL_OR,
        EFN_LOOP,
        EFN_WHILE ,
        
        EFN_NOT,
        EFN_EQUAL,
        EFN_NOTEQ,
        EFN_BELOW,
        EFN_ABOVE,
        EFN_BELEQ,
        EFN_ABOEQ,
        
        EFN_SET,
        EFN_MOD,
        EFN_MULOP, // *=
        EFN_DIVOP, // /=
        EFN_OROP,  // |=
        EFN_ANDOP, // &=
        EFN_ADDOP, // +=
        EFN_SUBOP, // -=
        EFN_MODOP, // %=
        
        EFN_SIN,
        EFN_COS,
        EFN_TAN,
        EFN_ASIN,
        EFN_ACOS,
        EFN_ATAN,
        EFN_ATAN2,
        EFN_SQR,
        EFN_SQRT,
        EFN_POW,
        EFN_POWOP,  // ^=
        EFN_EXP,
        EFN_LOG,
        EFN_LOG10,
        
        EFN_ABS,
        EFN_MIN,
        EFN_MAX,
        EFN_SIGN,
        EFN_RAND,
        EFN_FLOOR,
        EFN_CEIL,
        
        EFN_INVSQRT,
        EFN_SIGMOID,
        
        EFN_BAND,
        EFN_BOR,
        
        EFN_EXEC2,
        EFN_EXEC3,
        EFN_MEM,
        EFN_GMEM,
        EFN_FREEMBUF,
        EFN_MEMCPY,
        EFN_MEMSET,
        
        EFN_CONSTANT,
        EFN_REGISTER,
        EFN_REGISTER_ASSIGNMENT,
        EFN_BLOCK_STATEMENT,
        EFN_REPEAT,
        EFN_REPEAT_WHILE,

        EFN_ASSERT_EQUALS,

        EFN_total
    };


    enum TokenType
    {
      VALUE  =  258,
      IDENTIFIER  =  259,
      FUNCTION1   = 260,
      FUNCTION2   = 261,
      FUNCTION3   = 262,
      UMINUS    = 263,
      UPLUS    = 264,
    };
    
    
    const char * GetFuncName(FuncType fn);
    bool          LookupFunction(const char *name, FuncType *type, TokenType *params);

}}


