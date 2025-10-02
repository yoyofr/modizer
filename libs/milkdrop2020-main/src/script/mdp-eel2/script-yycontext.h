#pragma once

#include <stdint.h>
#include <string>

//#include "ns-eel.h"
#include "script-lextab.h"
#include "script-compiler.h"
#include "script-functype.h"


namespace Script { namespace mdpx {

    
    typedef intptr_t INT_PTR;
    
#define YYSTYPE INT_PTR

    
    

struct lextab;
class CompileContext;
class Expression;

struct yycontext
{
    CompileContext *compiler;
    const lextab *  yylextab;
    int errVar;
    std::string *errorMessage;
    int colCount;
    YYSTYPE result;
    YYSTYPE yylval;
    int    yychar;            /*  the lookahead symbol        */
    int    yynerrs;            /*  number of parse errors so far       */
    
    char    *llsave[16];             /* Look ahead buffer            */
    char    llbuf[100];             /* work buffer                          */
    char    *llp1;//   = &llbuf[0];    /* pointer to next avail. in token      */
    char    *llp2;//   = &llbuf[0];    /* pointer to end of lookahead          */
    char    *llend;//  = &llbuf[0];    /* pointer to end of token              */
    char    *llebuf;// = &llbuf[sizeof llbuf];
    int     lleof;
    int     yyline;//  = 0;
    
};

    
INT_PTR nseel_lookup(yycontext *ctx, TokenType *typeOfObject);



#define INTCONST 1
#define DBLCONST 2
#define HEXCONST 3
#define VARIABLE 4
#define OTHER    5
YYSTYPE nseel_translate(yycontext *ctx, int type);


int nseel_yyerror(yycontext *ctx, const char *msg);
int nseel_yylex(yycontext *ctx, const char **exp);
int nseel_yyparse(yycontext *ctx, const char *exp);
void nseel_llinit(yycontext *ctx);
int nseel_gettoken(yycontext *ctx, char *lltb, int lltbsiz);


}} // namespace
