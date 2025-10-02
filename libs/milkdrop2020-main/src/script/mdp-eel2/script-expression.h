
#pragma once

#include "../script.h"
#include "script-functype.h"


namespace Script { namespace mdpx {

    class KernelRegister;
    using KernelRegisterPtr = KernelRegister *;
  

#define PROFILE_EXPRESSIONS_COUNT  0
#define PROFILE_EXPRESSIONS_TIMING 0

ValueType ReadGMegaBuf(int addr);
void WrteGMegaBuf(int addr, ValueType v);
ValueType ReadMegaBuf(int addr);
void WrteMegaBuf(int addr, ValueType v);


    class Expression
    {
    private:
        FuncType m_fn;

    public:
        Expression(FuncType fn)
        : m_fn(fn)
        {
            InstanceCounter<Expression>::Increment();
        }
        
        virtual ~Expression()
        {
            InstanceCounter<Expression>::Decrement();
        }
        
        virtual ValueType Evaluate(EvalContext ec) = 0;
        virtual bool AssignTo(EvalContext ec, ValueType value)  {return false;}
        
        virtual bool IsStatement() {return false;}
        virtual void Print(std::ostream &o) = 0;
        
        virtual bool Resolve(ValueType &const_val)  {return false;}
        virtual bool Resolve(KernelRegisterPtr &cvar) {return false;}

        virtual void MarkRead() {};
        virtual void MarkWritten() {};
        
        inline int GetType()            {return m_fn;}
        

        
        std::string GetOperatorName()
        {
            return GetFuncName(m_fn);
        }
        std::string GetFnName()
        {
            return std::string("fn_") + GetFuncName(m_fn);
        }
        
#if PROFILE_EXPRESSIONS_TIMING
        int eval_count = 0;
        StopWatch::Clock::duration eval_time = StopWatch::Clock::duration(0);
#endif
    };

    using ExpressionPtr = Expression *;
    using ExpressionUPtr = std::unique_ptr<Expression>;

    
    inline std::ostream & operator <<(std::ostream &o, ExpressionPtr e)
    {
        if (e)
            e->Print(o);
        return o;
    }

}} 

