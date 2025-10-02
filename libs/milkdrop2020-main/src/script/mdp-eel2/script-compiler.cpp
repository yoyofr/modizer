/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2008 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  nseel-compiler.c

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


#include "../script.h"
#include "script-expression.h"
#include "script-compiler.h"
#include "script-preprocessor.h"
#include "script-yycontext.h"

namespace Script { namespace mdpx {


    
//
//
//
    
ExpressionPtr CompileContext::CompileExpression(const std::string &exp, std::string &errors)
{
    yycontext ctx;
    memset(&ctx, 0, sizeof(ctx));
    nseel_llinit(&ctx);
    ctx.compiler = this;
    ctx.yylextab = GetDefaultLexTab();
    ctx.errorMessage = &errors;
    if (!nseel_yyparse(&ctx,exp.c_str()) && !ctx.errVar)
    {
        return (ExpressionPtr)ctx.result;
    }
    return nullptr;
}
    
ExpressionPtr CompileContext::CompileWithPreprocessor(const std::string &code, std::string &errors)
{
    
    std::vector<std::string> lines;    
    nseel_preprocessCode(code, lines);
    
    std::vector<ExpressionPtr> statements;
    
    for (auto line : lines)
    {
        // parse
        std::string error;
        ExpressionPtr e = CompileExpression(line.c_str(), error);
        if (e)
        {
            statements.push_back(e);
        }
        else
        {
            errors += error;
            errors += '\n';
        }
    }

    if (!errors.empty()) {
        return nullptr;
    }


    return CreateBlockStatement(statements);
}





}} // namespace
