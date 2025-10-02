
#include "platform.h"

#include "script-functype.h"

#include <map>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

namespace Script { namespace mdpx {

#define DEFINE_FUNC(__params, __type, __name) s_fnmap[ std::string(#__name) ] = s_efnmap[__type] = FuncInfo{#__name, __type, FUNCTION ## __params} ;
#define DEFINE_OP(__params, __type, __name) s_fnmap[ std::string(#__name) ] = s_efnmap[__type] = FuncInfo{#__name, __type, FUNCTION ## __params} ;
#define DEFINE_TOKEN(__token, __type, __name) s_fnmap[ std::string(#__name) ] = s_efnmap[__type] = FuncInfo{#__name, __type, __token} ;

    struct FuncInfo
    {
        const char *name;
        FuncType type;
        TokenType token;
    };
    
    
    static  std::map<std::string,   FuncInfo> s_fnmap;
    static  std::map<FuncType,      FuncInfo> s_efnmap;
    
    const char * GetFuncName(FuncType fn)
    {
        auto it = s_efnmap.find(fn);
        if (it != s_efnmap.end())
        {
            return it->second.name;
        }
        return "<unknown>";
    }
    
    static void AddAlias(const char *a, const char *b)
    {
        auto it = s_fnmap.find(b);
        if (it != s_fnmap.end())
        {
            FuncInfo fi = it->second;
            s_fnmap[a] = fi;
        }
    }
    
    
    static void RegisterFunctions( )
    {
        if (!s_fnmap.empty()) return;
        
        DEFINE_OP(2,FN_ASSIGN,  =)
        DEFINE_OP(2,FN_MULTIPLY, *)
        DEFINE_OP(2,FN_DIVIDE   , /)
        DEFINE_OP(2,FN_MODULO   , %)
        DEFINE_OP(2,FN_ADD      , + )
        DEFINE_OP(2,FN_SUB      , - )
        DEFINE_OP(2,FN_BITWISE_AND      , & )
        DEFINE_OP(2,FN_BITWISE_OR       , | )

        DEFINE_FUNC(2,FN_BITWISE_AND      , bitwise_and )
        DEFINE_FUNC(2,FN_BITWISE_OR       , bitwise_or )


        DEFINE_FUNC(1,FN_UMINUS   , _uminus )
        DEFINE_FUNC(1,FN_UPLUS    , _uplus )
        
        DEFINE_FUNC(3, EFN_IF,_if)
        DEFINE_FUNC(2,EFN_LOGICAL_AND, _and)
        DEFINE_FUNC(2,EFN_LOGICAL_OR,_or)
        DEFINE_FUNC(2,EFN_LOOP, loop)
        DEFINE_FUNC(1,EFN_WHILE, while)
        
        DEFINE_FUNC(1,EFN_NOT,_not)
        DEFINE_FUNC(2,EFN_EQUAL,_equal)
        DEFINE_FUNC(2,EFN_NOTEQ,_noteq)
        DEFINE_FUNC(2,EFN_BELOW,_below)
        DEFINE_FUNC(2,EFN_ABOVE,_above)
        DEFINE_FUNC(2,EFN_BELEQ,_beleq)
        DEFINE_FUNC(2,EFN_ABOEQ,_aboeq)
        DEFINE_FUNC(2,EFN_SET,_set)
        DEFINE_FUNC(2,EFN_MOD,_mod)
        DEFINE_FUNC(2,EFN_MULOP, _mulop)
        DEFINE_FUNC(2,EFN_DIVOP, _divop)
        DEFINE_FUNC(2,EFN_OROP,  _orop)
        DEFINE_FUNC(2,EFN_ANDOP, _andop)
        DEFINE_FUNC(2,EFN_ADDOP, _addop)
        DEFINE_FUNC(2,EFN_SUBOP, _subop)
        DEFINE_FUNC(2,EFN_MODOP, _modop)
        DEFINE_FUNC(1,EFN_SIN,sin)
        DEFINE_FUNC(1,EFN_COS,cos)
        DEFINE_FUNC(1,EFN_TAN,tan)
        DEFINE_FUNC(1,EFN_ASIN,asin)
        DEFINE_FUNC(1,EFN_ACOS,acos)
        DEFINE_FUNC(1,EFN_ATAN,atan)
        DEFINE_FUNC(2,EFN_ATAN2,atan2)
        DEFINE_FUNC(1,EFN_SQR,sqr)
        DEFINE_FUNC(1,EFN_SQRT,sqrt)
        DEFINE_FUNC(2,EFN_POW,pow)
        DEFINE_FUNC(2,EFN_POWOP, _powop)
        DEFINE_FUNC(1,EFN_EXP,exp)
        DEFINE_FUNC(1,EFN_LOG,log)
        DEFINE_FUNC(1,EFN_LOG10,log10)
        DEFINE_FUNC(1,EFN_ABS,abs)
        DEFINE_FUNC(2,EFN_MIN,min)
        DEFINE_FUNC(2,EFN_MAX,max)
        DEFINE_FUNC(1,EFN_SIGN,sign)
        DEFINE_FUNC(1,EFN_RAND,rand)
        DEFINE_FUNC(1,EFN_FLOOR,floor)
        DEFINE_FUNC(1,EFN_CEIL,ceil)
        DEFINE_FUNC(1,EFN_INVSQRT, invsqrt)
        DEFINE_FUNC(2,EFN_SIGMOID, sigmoid)
        DEFINE_FUNC(2,EFN_BAND, band)
        DEFINE_FUNC(2,EFN_BOR, bor)

        DEFINE_FUNC(2,EFN_ASSERT_EQUALS, assert_equals)

        DEFINE_FUNC(2,EFN_EXEC2, exec2)
        DEFINE_FUNC(3,EFN_EXEC3, exec3)
        
        DEFINE_FUNC(1,EFN_MEM, _mem)
        DEFINE_FUNC(1,EFN_GMEM, _gmem)
        DEFINE_FUNC(1,EFN_FREEMBUF, freembuf)
        DEFINE_FUNC(3,EFN_MEMCPY, memcpy)
        DEFINE_FUNC(3,EFN_MEMSET, memset)
        
        DEFINE_TOKEN(IDENTIFIER,EFN_CONSTANT, constant)
        DEFINE_TOKEN(IDENTIFIER,EFN_REGISTER, register)
        DEFINE_TOKEN(IDENTIFIER,EFN_REGISTER_ASSIGNMENT, register_assignment)
        DEFINE_TOKEN(IDENTIFIER,EFN_BLOCK_STATEMENT, block_statement)

        
        
        
        AddAlias("if","_if");
        AddAlias("bnot","_not");
        AddAlias("assign","_set");
        AddAlias("equal","_equal");
        AddAlias("below","_below");
        AddAlias("above","_above");
        AddAlias("megabuf","_mem");
        AddAlias("gmegabuf","_gmem");
        AddAlias("int","floor");
        
    }
    
    bool LookupFunction(const char *name, FuncType *type, TokenType *token)
    {
        RegisterFunctions();
        
        
        auto it = s_fnmap.find(name);
        if (it == s_fnmap.end())
        {
            // try lowercase...
            std::string lower(name);
            for (size_t i=0; i < lower.size(); i++)
            {
                lower[i] = std::tolower(lower[i]);
            }
            
            it = s_fnmap.find(lower);
            if (it == s_fnmap.end())
            {
                *type = (FuncType)0;
                *token = (TokenType)0;
                return false;
            }
        }
        
        FuncInfo fi = (*it).second;
        *token = fi.token;
        *type = fi.type;
        return true;
        
    }


}} // nseel
