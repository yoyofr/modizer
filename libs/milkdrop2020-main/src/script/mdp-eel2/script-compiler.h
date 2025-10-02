
#pragma once

#include "../script.h"

#include "script-expression.h"
#include "script-context.h"
#include "script-kernel.h"
#include "script-functype.h"

namespace Script { namespace mdpx {

    
class CompileContext
{
public:
    CompileContext() = default;
    virtual ~CompileContext() = default;
    
    
    virtual ExpressionPtr LookupVariable(const char *name) = 0;
    virtual ExpressionPtr AddVariable(const char *name) = 0;

    virtual ExpressionPtr CompileWithPreprocessor(const std::string &code, std::string &errors);
    virtual ExpressionPtr CompileExpression(const std::string &code, std::string &errors);

    virtual ExpressionPtr CreateCompiledConstantExpression(ValueType value) = 0;
    virtual ExpressionPtr CreateBlockStatement(std::vector<ExpressionPtr> &statements) = 0;


    virtual ExpressionPtr CreateCompiledExpression(FuncType  fn,
                                                   ExpressionPtr  code1,
                                                   ExpressionPtr  code2 = nullptr,
                                                   ExpressionPtr  code3 = nullptr
                                                   ) = 0;

protected:    
  
};
    
    
    
}} // namespace Script





