

#include <assert.h>
#include <math.h>
#include <vector>
#include <string>
#include <functional>
#include <memory>

#include "script-expression.h"
#include "script-compiler.h"
#include "script-kernel.h"
#include "script-functions.h"


#if PROFILE_EXPRESSIONS_TIMING
#define PROFILE_BEGIN()          StopWatch sw;
#define PROFILE_END()            eval_time += sw.GetElapsed();
#else
#define PROFILE_BEGIN()
#define PROFILE_END()
#endif



#define RESOLVE 1


namespace Script { namespace mdpx {

#define FLAG_CHECK_NAN 0x100000


    
namespace Expressions
{
    using namespace Functions;
    


    
    
    template<typename T>
    ValueType evaluate(EvalContext ec, T e);
    
    
    template<>
    inline ValueType evaluate<ExpressionPtr >(EvalContext ec, ExpressionPtr e)
    {
#if PROFILE_EXPRESSIONS_COUNT
        e->eval_count++;
#endif
        return e->Evaluate(ec);
    }
    
    template<>
    inline ValueType evaluate<ValueType>(EvalContext ec, ValueType v)
    {
        return v;
    }
    
    template<>
    inline ValueType evaluate<ConstValueTypePtr>(EvalContext ec, ConstValueTypePtr v)
    {
        return *v;
    }
    
    
    template<>
    inline ValueType evaluate<KernelRegisterPtr>(EvalContext ec, KernelRegisterPtr var)
    {
        return var->GetValue(ec);
    }
    
    
    template<>
    inline ValueType evaluate<VariablePtr>(EvalContext ec, VariablePtr var)
    {
        return var->GetValue();
    }
    
    
    
    
    template<typename T>
    inline bool evaluate_as_boolean(EvalContext ec, T e)
    {
        ValueType value = evaluate(ec, e);
        return NonZero(value);
    }
    
    
    template<typename T>
    void mark_read(T e);

    template<>
    inline void mark_read<ValueType>(ValueType v) {}

    template<>
    inline void mark_read<ExpressionPtr>(ExpressionPtr e)
    {
        e->MarkRead();
    }

    template<>
    inline void mark_read<KernelRegisterPtr>(KernelRegisterPtr cv)
    {
        cv->MarkRead();
    }

 
    template<typename T>
    void mark_written(T e);
    
    template<>
    inline void mark_written<ExpressionPtr>(ExpressionPtr e)
    {
        e->MarkWritten();
    }
    
    template<>
    inline void mark_written<KernelRegisterPtr>(KernelRegisterPtr cv)
    {
        cv->MarkWritten();
    }


    template<typename T>
    void delete_expr(T e)
    {
        
    }

    template<>
    inline void delete_expr<ExpressionPtr>(ExpressionPtr e)
    {
        delete e;
    }

    template<typename T>
    bool check_nan(T e)
    {
        return true;
    }


    template<>
    bool check_nan<ValueType>(ValueType v)
    {
        return !isfinite(v) || (v == 0.0);
    }


    
    //
    //
    
    class BlockStatement : public Expression
    {
    public:
        BlockStatement(std::vector<ExpressionPtr> &list)
            : Expression(EFN_BLOCK_STATEMENT), m_statements(list)
        {
            
        }
        
        virtual ~BlockStatement() {
            for (auto expr : m_statements)
                delete_expr(expr);
        }
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType v = ValueType();
            for (size_t i=0; i < m_statements.size(); i++)
            {
                v = evaluate(ec, m_statements[i]);
            }
            return v;
        }
        
        virtual void Print(std::ostream &o) override
        {
            for (size_t i=0; i < m_statements.size(); i++)
            {
                o << '\t';
                
                if (!m_statements[i]->IsStatement()) {
                    o << "(void)";
                }
                
                o << m_statements[i];
                o << ";" << std::endl;
            }
        }
        
        virtual bool IsStatement() override {return true;}

    private:
        std::vector<ExpressionPtr> m_statements;
    };
    
    
   
    
    class RepeatWhile : public Expression
    {
    public:
        RepeatWhile(ExpressionPtr expr)
        : Expression(EFN_REPEAT_WHILE), m_expr(expr)
        {
            mark_read(expr);

        }
        
        virtual ~RepeatWhile()
        {
            delete_expr(m_expr);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType result = 0;
            do
            {
                result = evaluate(ec, m_expr);
            } while ( NonZero(result) );
            return result;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << "while(" << m_expr << ")";
        }
        
        virtual bool IsStatement() override {return true;}
    protected:
        ExpressionPtr m_expr;
    };
    
    class Repeat : public Expression
    {
    public:
        Repeat(ExpressionPtr count_expr, ExpressionPtr expr)
        : Expression(EFN_REPEAT), m_count(count_expr), m_expr(expr)
        {
            mark_read(expr);
        }
        
        virtual ~Repeat()
        {
            delete_expr(m_count);
            delete_expr(m_expr);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            int count = (int)m_count->Evaluate(ec);
            
            ValueType result = 0;
            //while (evaluate_as_boolean(ec, m_test))
            for (int i=0; i < count; i++)
            {
                result = evaluate(ec, m_expr);
            }
            return result;
        }
        
        
        virtual void Print(std::ostream &o) override
        {
            o << "loop(";
            o << m_count;
            o << ",";
            o << m_expr;
            o << ")";
        }
        
        virtual bool IsStatement() override {return true;}
    protected:
        ExpressionPtr m_count;
        ExpressionPtr m_expr;
    };
    
    class If : public Expression
    {
    public:
        If(ExpressionPtr test, ExpressionPtr ifthen, ExpressionPtr ifelse)
        : Expression(EFN_IF), m_test(test), m_ifthen(ifthen), m_ifelse(ifelse)
        {
            mark_read(test);
            mark_read(ifthen);
            mark_read(ifelse);

        }
        
        virtual ~If()
        {
            delete_expr(m_test);
            delete_expr(m_ifthen);
            delete_expr(m_ifelse);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            bool test = evaluate_as_boolean(ec, m_test);
            if (test) {
                return evaluate(ec, m_ifthen);
            } else {
                return evaluate(ec, m_ifelse);
            }
        }
        
        
        virtual void Print(std::ostream &o) override
        {
            #if 0
            o << "_IF";
            o << "(";
            o << m_test;
            o << ",";
            o << m_ifthen;
            o << ",";
            o << m_ifelse;
            o << ")";
            #else 

            o << "(";
            o << "IsTrue(" << m_test << ") ? (" << m_ifthen << ") : (" << m_ifelse << ")";
            o << ")";
            
            #endif 
        }
        
        virtual bool IsStatement() override {return true;}
        
    protected:
        ExpressionPtr m_test;
        ExpressionPtr m_ifthen;
        ExpressionPtr m_ifelse;
    };


class InvalidExpression: public Expression
{

public:
    InvalidExpression(FuncType efn)
    : Expression(efn)
    {
    }
    
    virtual ~InvalidExpression()
    {
    }

    
    virtual ValueType Evaluate(EvalContext ec) override
    {
        return 0;
        
    }
    
    virtual void Print(std::ostream &o) override
    {
        o << "invalid_operation()";
    }
};

    
    class UnaryExpression : public Expression
    {
    public:
        UnaryExpression(FuncType fn, ExpressionPtr expr)
        : Expression(fn),  m_expr(expr)
        {
            mark_read(expr);

        }

        virtual ~UnaryExpression()
        {
            delete_expr(m_expr);
        }
    
        virtual void Print(std::ostream &o) override
        {
            o << GetFnName();
            o << "(";
            o << m_expr;
            o << ")";
        }
        

    protected:
        
        ExpressionPtr m_expr;
    };
    
    
    template<OpFunc1 TFunc>
    class UnaryOp : public UnaryExpression
    {
    public:
        UnaryOp(FuncType fn, ExpressionPtr expr)
        : UnaryExpression(fn, expr)
        {
            mark_read(expr);
        }
        
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType arg = evaluate(ec, m_expr);
            ValueType result = TFunc(arg);
            return result;
        }
        
        
    };
    
    
    template<OpFunc1 TFunc, typename A0>
    class UnaryOpT : public Expression
    {
    public:
        UnaryOpT(FuncType fn, A0 expr)
        : Expression(fn), m_expr(expr)
        {
            mark_read(expr);
        }
        
        virtual ~UnaryOpT()
        {
            delete_expr(m_expr);
        }


        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType arg = evaluate(ec, m_expr);
            ValueType result = TFunc(arg);
            return result;
        }
        virtual void Print(std::ostream &o) override
        {
            o << GetFnName();
            o << "(";
            o << m_expr;
            o << ")";
        }
        
    protected:
        
        A0 m_expr;
    };
    
   

    template<OpFunc0 TFunc>
    class Func0 : public Expression
    {
    public:
        Func0(FuncType fn)
        {
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << GetFnName() << '(' << ')';
        }
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType result = TFunc();
            return result;
        }
    };
    
    template<OpFunc1 TFunc, typename A0>
    class Func1 : public Expression
    {
    public:
        Func1(FuncType fn, A0 a0)
        : Expression(fn), m_a0(a0)
        {
            mark_read(a0);
        }
        
        virtual ~Func1()
        {
            delete_expr(m_a0);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType v0 = evaluate(ec, m_a0);
            PROFILE_BEGIN();
            ValueType result = TFunc( v0 );
            PROFILE_END();
            return result;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << GetFnName();
            o << "(" << m_a0 << ")";
        }

    protected:
        A0 m_a0;
    };
    
    template<OpFunc2 TFunc, typename A0, typename A1>
    class Func2 : public Expression
    {
    public:
        Func2(FuncType fn, A0 a0, A1 a1)
        : Expression(fn), m_a0(a0), m_a1(a1)
        {
            mark_read(a0);
            mark_read(a1);
        }
        
        virtual ~Func2()
        {
            delete_expr(m_a0);
            delete_expr(m_a1);
        }


        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType v0 = evaluate(ec, m_a0);
            ValueType v1 = evaluate(ec, m_a1);
            PROFILE_BEGIN()
            ValueType result = TFunc( v0, v1 );
            PROFILE_END()
            return result;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << GetFnName();
            o << "(" << m_a0 << "," << m_a1 << ")";
        }

    protected:
        A0 m_a0;
        A1 m_a1;
    };
    
    
    
    template<OpFunc3 TFunc, typename A0, typename A1, typename A2>
    class Func3 : public Expression
    {
    public:
        Func3(FuncType fn, A0 a0, A1 a1, A2 a2)
        : Expression(fn), m_a0(a0), m_a1(a1), m_a2(a2)
        {
            mark_read(a0);
            mark_read(a1);
            mark_read(a2);
        }
        
        virtual ~Func3()
        {
            delete_expr(m_a0);
            delete_expr(m_a1);
            delete_expr(m_a2);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType result = TFunc( evaluate(m_a0), evaluate(m_a1), evaluate(m_a2));
            return result;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << GetFnName();
            o << "(" << m_a0 << "," << m_a1 << "," << m_a2 << ")";
        }

    protected:
        A0 m_a0;
        A1 m_a1;
        A2 m_a2;
    };
    
    
    class IntRandomOp : public UnaryExpression
    {
        KernelRegisterPtr m_random_reg;
    public:
        IntRandomOp(FuncType fn, ExpressionPtr expr, KernelRegisterPtr random_reg)
        : UnaryExpression(fn, expr), m_random_reg(random_reg)
        {
            mark_read(expr);
            mark_read(random_reg);
            mark_written(random_reg);
        }
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType f = evaluate(ec, m_expr);
            
            ValueType random_state = m_random_reg->GetValue(ec);
            
            ValueType result = fn_rand(random_state, f);
            
            m_random_reg->SetValue(ec, random_state);

            return result;
        }
        

        virtual void Print(std::ostream &o) override
        {
           o << GetFnName();
           o << "(" << m_random_reg << "," << m_expr << ")";
        }

        
    };
    
    
    class BinaryExpression : public Expression
    {
    public:
        BinaryExpression(FuncType fn, ExpressionPtr left, ExpressionPtr right)
        : Expression(fn), m_left(left), m_right(right)
        {
            mark_read(left);
            mark_read(right);
        }
        
        virtual ~BinaryExpression()
        {
            delete_expr(m_left);
            delete_expr(m_right);
        }

        
        virtual void Print(std::ostream &o) override
        {
            
            o << GetFnName();
            o << "(";
            o << m_left;
            o << ",";
            o << m_right;
            o << ")";
        }

        
    protected:
        ExpressionPtr m_left;
        ExpressionPtr m_right;
    };




    
    template<OpFunc2 TFunc, typename TLeft, typename TRight>
    class BinaryOpT : public Expression
    {
    public:
        BinaryOpT(FuncType fn, TLeft left, TRight right)
        : Expression(fn), m_left(left), m_right(right)
        {
            mark_read(left);
            mark_read(right);
        }
        
        virtual ~BinaryOpT()
        {
            delete_expr(m_left);
            delete_expr(m_right);
        }

        
        virtual void Print(std::ostream &o) override
        {

            switch (GetType())
           {
               case EFN_INVSQRT:
               case FN_DIVIDE:
                   if (check_nan(m_right))
                       o.setf(FLAG_CHECK_NAN);
                   break;
           }
            
            o << "(" << m_left << GetOperatorName() << m_right << ")";
        }
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType left = evaluate(ec, m_left);
            ValueType right = evaluate(ec, m_right);
            PROFILE_BEGIN()
            ValueType result = TFunc(left, right);
            PROFILE_END()
            return result;
        }

    protected:
        TLeft  m_left;
        TRight m_right;
    };
    

    
    template<OpFunc2 TFunc>
    class AssignBinaryOp : public Expression
    {
    public:
        AssignBinaryOp(FuncType fn, KernelRegisterPtr left, ExpressionPtr right)
        : Expression(fn), m_left(left), m_right(right)
        {
            mark_read(left);
            mark_read(right);
            mark_written(left);
        }
        
        virtual ~AssignBinaryOp()
        {
            delete_expr(m_right);
        }

        
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType left = evaluate(ec, m_left);
            ValueType right = evaluate(ec, m_right);
            ValueType result =  TFunc(left, right);
            
            m_left->SetValueChecked(ec, result );
            return result;
        }
        
        virtual void Print(std::ostream &o) override
        {

             switch (GetType())
            {
                case EFN_INVSQRT:
                case FN_DIVIDE:
                    if (check_nan(m_right))
                        o.setf(FLAG_CHECK_NAN);
                    break;
            }
            
            o << GetFnName();
            o << "(";
            o << m_left;
            o << ",";
            o << m_right;
            o << ")";
        }
        
        
    protected:
        KernelRegisterPtr     m_left;
        ExpressionPtr   m_right;
    };
    

template<OpFunc2 TFunc>
class AssignExpressionBinaryOp : public Expression
{
public:
    AssignExpressionBinaryOp(FuncType fn, ExpressionPtr left, ExpressionPtr right)
    : Expression(fn), m_left(left), m_right(right)
    {
        mark_read(left);
        mark_read(right);
        mark_written(left);
    }
    
    virtual ~AssignExpressionBinaryOp()
    {
        delete_expr(m_left);
        delete_expr(m_right);
    }

    
    
    virtual ValueType Evaluate(EvalContext ec) override
    {
        ValueType left = evaluate(ec, m_left);
        ValueType right = evaluate(ec, m_right);
        ValueType result =  TFunc(left, right);
        
        m_left->AssignTo(ec, result );
        return result;
    }
    
    virtual void Print(std::ostream &o) override
    {

         switch (GetType())
        {
            case EFN_INVSQRT:
            case FN_DIVIDE:
                if (check_nan(m_right))
                    o.setf(FLAG_CHECK_NAN);
                break;
        }
        
        o << GetFnName();
        o << "(";
        o << m_left;
        o << ",";
        o << m_right;
        o << ")";
    }
    
    
protected:
    ExpressionPtr     m_left;
    ExpressionPtr   m_right;
};



    
    class LogicalAnd : public BinaryExpression
    {
    public:
        LogicalAnd(FuncType fn,  ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(fn, left, right)
        {
        }
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            return _LOGICAL_AND( evaluate(ec, m_left), evaluate(ec, m_right) );
        }

        virtual void Print(std::ostream &o) override
        {
            #if 0
            o << "_LOGICAL_AND";
            o << '(' << m_left  << ',' << m_right << ')';
            #else
            o << "(((IsTrue(" << m_left << ") && IsTrue(" << m_right << ")) ? 1.0 : 0.0)";
            #endif
        }
        
        

        
    };
    
    
    
    class LogicalOr : public BinaryExpression
    {
    public:
        LogicalOr(FuncType fn, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(fn, left, right)
        {
        }
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            return _LOGICAL_OR( evaluate(ec, m_left), evaluate(ec, m_right) );
        }

        virtual void Print(std::ostream &o) override
        {
            #if 0
            o << "_LOGICAL_OR";
            o << '(' << m_left  << ',' << m_right << ')';
            #else
            o << "(((IsTrue(" << m_left << ") || IsTrue(" << m_right << ")) ? 1.0 : 0.0)";
            #endif              

        }


    };
    //
    //
    
    
    class ConstantValue : public Expression
    {
    public:
        ConstantValue(ValueType value)
        : Expression(EFN_CONSTANT), m_Value(value)
        {
        }
        
     
        
        inline ValueType GetValue() const
        {
            return m_Value;
        }

        virtual ValueType Evaluate(EvalContext ec) override
        {
            return m_Value;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << GetValue();
        }
        
        
        virtual bool Resolve(ValueType &const_val) override
        {
            const_val = m_Value;
            return true;
        }

        
     
    protected:
        ValueType m_Value;
    };
    
    
    
    
    template<typename TSource>
    class RegisterAssignment : public Expression
    {
    public:
        RegisterAssignment(KernelRegisterPtr target, TSource expr)
        : Expression(EFN_REGISTER_ASSIGNMENT), m_target(target), m_expr(expr)
        {
            mark_read(expr);
            mark_written(target);
        }
        
        
        virtual ~RegisterAssignment()
        {
            delete_expr(m_expr);
        }

        
        virtual void Print(std::ostream &o) override
        {
//            o << m_target << "=" << m_expr;
            
            // evaluate expression
            std::stringstream ss;
            ss << m_expr;
            if (ss.flags() & FLAG_CHECK_NAN)
            {
                o << "fn_assign(" << m_target << ", " << ss.str() << ")";
                o.setf(FLAG_CHECK_NAN);
            }
            else
            {
                o << m_target << "=" << ss.str();
            }
        }
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType value = evaluate(ec, m_expr);
            
            // set value
            m_target->SetValueChecked(ec, value);
            return value;
        }
        
        virtual bool IsStatement() override {return true;}

    protected:
        KernelRegisterPtr m_target;
        TSource     m_expr;
    };
    
    

    class RegisterReference: public Expression
    {
        KernelRegisterPtr m_register;

    public:
        RegisterReference(KernelRegisterPtr reg)
        : Expression(EFN_REGISTER), m_register(reg)
        {
        }
        
        virtual void MarkRead() override
        {
            mark_read(m_register);
        };
        
        virtual void MarkWritten() override
        {
         
            mark_written(m_register);
        };
        
        
        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            return m_register->GetValue(ec);
        }
        
        virtual bool AssignTo(EvalContext ec, ValueType value) override
        {
            m_register->SetValueChecked(ec, value);
            return true;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << m_register;
        }
        
        
        virtual bool Resolve(KernelRegisterPtr &cvar) override
        {
            cvar = m_register;
            return true;
            
        }

    
   };


    static int EvalIndex(EvalContext ec, ExpressionPtr index)
    {
        ValueType value = index->Evaluate(ec);
        if (value < 0) return 0;
        return (int)std::floor(value);
    }




class Megabuffer
{
    int _capacity;
    int _bankShift;
    int _bankSize;
    int _bankCount;
    
    // vector of banks of bankSize
    std::vector< std::vector<ValueType> > _data;
    
    std::mutex _mutex;
    
public:
    Megabuffer(int capacity, int bankShift)
    :_capacity(capacity), _bankShift(bankShift)
    {
        _bankSize = 1 << _bankShift;
        _bankCount = _capacity >> _bankShift;
        
        _data.resize(_bankCount);
    }
    
    virtual ~Megabuffer()
    {
       
    }
    
    void Clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _data.clear();
        _data.resize(_bankCount);
    }
    
    ValueType Read(int addr)
    {
        addr &= (_capacity - 1);
        int bank = addr >> _bankShift;
        int offset = addr & (_bankSize - 1);

        if (bank >= _data.size()) {
            return 0;
        }

        std::lock_guard<std::mutex> lock(_mutex);

        const auto &v = _data[bank];
        if (v.empty()) return 0;
        
        return v[offset];
    }
    
    void Write(int addr, ValueType value)
    {
        addr &= (_capacity - 1);
        int bank = addr >> _bankShift;
        int offset = addr & (_bankSize - 1);

        if (bank >= _data.size()) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(_mutex);
        
        auto &v = _data[bank];
        if (v.empty()) {
            v.resize(_bankSize);
        }
        
        // store value
        v[offset] = value;
    }
    
};

//
//static ValueType s__GMegaBuf[SHARED_GRAM_SIZE];
//static ValueType s__MegaBuf[SHARED_RAM_SIZE];


static constexpr int SHARED_GRAM_SIZE = (1<<20);
static constexpr int SHARED_RAM_SIZE = (128 * 64 * 1024);


static Megabuffer s__GMegaBuf {SHARED_GRAM_SIZE, 10 };
static Megabuffer s__MegaBuf {SHARED_RAM_SIZE, 13 };





static ValueType ReadGMegaBuf(int addr)
{
    return s__GMegaBuf.Read(addr);
}

static void WriteGMegaBuf(int addr, ValueType v)
{
    s__GMegaBuf.Write(addr, v);
}

static ValueType ReadMegaBuf(int addr)
{
    return s__MegaBuf.Read(addr);
}

static void WriteMegaBuf(int addr, ValueType v)
{
    s__MegaBuf.Write(addr, v);
}


    class Megabuf: public Expression
    {
        ExpressionPtr m_index;

    public:
        Megabuf(ExpressionPtr index)
        : Expression(EFN_MEM), m_index(index)
        {
        }
        
        
        virtual ~Megabuf()
        {
            delete_expr(m_index);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            int index = EvalIndex(ec, m_index);
            index &= SHARED_RAM_SIZE - 1;

            // not thread or context safe
            return ReadMegaBuf(index);
        }
        
        virtual bool AssignTo(EvalContext ec, ValueType value) override
        {
            int index = EvalIndex(ec, m_index);
            index &= SHARED_RAM_SIZE - 1;

            // not thread or context safe
            WriteMegaBuf(index, value);
            return true;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << "megabuf(" << m_index << ")";
        }
    };

    class GMegabuf: public Expression
    {
        ExpressionPtr m_index;

    public:
        GMegabuf(ExpressionPtr index)
        : Expression(EFN_GMEM), m_index(index)
        {
        }
        
        virtual ~GMegabuf()
        {
            delete_expr(m_index);
        }


        virtual ValueType Evaluate(EvalContext ec) override
        {
            int index = EvalIndex(ec, m_index);
            index &= SHARED_GRAM_SIZE - 1;

            // not thread or context safe
            return ReadGMegaBuf(index);
            
        }

        virtual bool AssignTo(EvalContext ec, ValueType value) override
        {
            int index = EvalIndex(ec, m_index);
            index &= SHARED_GRAM_SIZE - 1;

            // not thread or context safe
            WriteGMegaBuf(index, value);
            return value;
        }
        

        virtual void Print(std::ostream &o) override
        {
            o << "gmegabuf(" << m_index << ")";
        }
    };


    class Exec2Expression: public Expression
    {
        ExpressionPtr m_arg0;
        ExpressionPtr m_arg1;

    public:
        Exec2Expression(ExpressionPtr arg0, ExpressionPtr arg1)
        : Expression(EFN_EXEC2), m_arg0(arg0), m_arg1(arg1)
        {
        }
        
        virtual ~Exec2Expression()
        {
            delete_expr(m_arg0);
            delete_expr(m_arg1);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            m_arg0->Evaluate(ec);
            return m_arg1->Evaluate(ec); // i guess we just return the last one here...
            
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << "exec2(" << m_arg0 << "," << m_arg1 << ")";
        }
    };


    class Exec3Expression: public Expression
    {
        ExpressionPtr m_arg0;
        ExpressionPtr m_arg1;
        ExpressionPtr m_arg2;

    public:
        Exec3Expression(ExpressionPtr arg0, ExpressionPtr arg1, ExpressionPtr arg2)
        : Expression(EFN_EXEC3), m_arg0(arg0), m_arg1(arg1), m_arg2(arg2)
        {
        }
        
        virtual ~Exec3Expression()
        {
            delete_expr(m_arg0);
            delete_expr(m_arg1);
            delete_expr(m_arg2);
        }


        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            m_arg0->Evaluate(ec);
            m_arg1->Evaluate(ec);
            return m_arg2->Evaluate(ec); // i guess we just return the last one here...
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << "exec3(" << m_arg0 << "," << m_arg1 << "," << m_arg2 << ")";
        }
    };


    class AssignmentExpression: public Expression
    {
        ExpressionPtr m_lhs;
        ExpressionPtr m_rhs;

    public:
        AssignmentExpression(ExpressionPtr lhs, ExpressionPtr rhs)
        : Expression(EFN_REGISTER_ASSIGNMENT), m_lhs(lhs), m_rhs(rhs)
        {
        }
        
        virtual ~AssignmentExpression()
        {
            delete_expr(m_lhs);
            delete_expr(m_rhs);
        }

        
        virtual ValueType Evaluate(EvalContext ec) override
        {
            ValueType value = m_rhs->Evaluate(ec); // i guess we just return the last one here...
            m_lhs->AssignTo(ec, value);
            return value;
        }
        
        virtual void Print(std::ostream &o) override
        {
            o << "fn_assign(" << m_lhs << "," << m_rhs << ")";
        }
    };





    template<typename TExpr, typename... TArgs>
    TExpr *NewExpr(TArgs && ...args)
    {
        TExpr *expr = new TExpr( std::forward<TArgs>(args)... );
        return expr;
    }


        template<OpFunc1 TFunc, typename A0>
        ExpressionPtr MakeUnaryOpT(FuncType fn, A0 expr)
        {
            return NewExpr< UnaryOpT<TFunc, A0> >(fn, expr);
        }
        
        template<OpFunc1 TFunc>
        ExpressionPtr MakeUnaryOp(FuncType fn, ExpressionPtr a0)
        {
#if RESOLVE
            ValueType c0;
            if (a0->Resolve(c0)) {
                delete_expr(a0);
                return MakeUnaryOpT<TFunc>(fn, c0);
            }
            
            KernelRegisterPtr v0;
            if (a0->Resolve(v0)) {
                delete_expr(a0);
                return MakeUnaryOpT<TFunc>(fn, v0);
            }
#endif
            return MakeUnaryOpT<TFunc>(fn, a0);
        }
        
        template<OpFunc2 TFunc, typename A0,  typename A1 >
        ExpressionPtr MakeBinaryOpT(FuncType fn, A0 left, A1 right)
        {
            return NewExpr< BinaryOpT<TFunc, A0, A1> >(fn, left, right);
        }
      

        template<OpFunc2 TFunc, typename A0>
        ExpressionPtr MakeBinaryOpRight(FuncType fn, A0 a0, ExpressionPtr a1)
        {
#if RESOLVE
            ValueType c1;
            if (a1->Resolve(c1)) {
                delete_expr(a1);
                return MakeBinaryOpT<TFunc>(fn, a0, c1);
            }

            KernelRegisterPtr v1;
            if (a1->Resolve(v1)) {
                delete_expr(a1);
                return MakeBinaryOpT<TFunc>(fn, a0, v1);
            }
#endif
            return MakeBinaryOpT<TFunc>(fn, a0, a1);
        }

        
        template<OpFunc2 TFunc>
        ExpressionPtr MakeBinaryOp(FuncType fn, ExpressionPtr a0, ExpressionPtr a1)
        {
#if RESOLVE
            ValueType c0;
            if (a0->Resolve(c0)) {
                delete_expr(a0);
                return MakeBinaryOpRight<TFunc>(fn, c0, a1);
            }
            
            KernelRegisterPtr v0;
            if (a0->Resolve(v0)) {
                delete_expr(a0);
                return MakeBinaryOpRight<TFunc>(fn, v0, a1);
            }
#endif
            return MakeBinaryOpRight<TFunc>(fn, a0, a1);
        }
        
        
        template<OpFunc2 TFunc>
        ExpressionPtr MakeAssignBinaryOp(FuncType fn, ExpressionPtr left, ExpressionPtr right)
        {
            KernelRegisterPtr v;
            if (left->Resolve(v)) {
                v->MarkWritten();
                delete_expr(left);
                return NewExpr< AssignBinaryOp<TFunc> >(fn, v, right );
            } else {
                return NewExpr< AssignExpressionBinaryOp<TFunc> >(fn, left, right);
            }
        }
        
        template<OpFunc0 TFunc>
        ExpressionPtr MakeFunc(FuncType fn)
        {
            return NewExpr< Func0<TFunc> >(fn);
        }
        
        //
        //
        //
        
        template<OpFunc1 TFunc, typename A0>
        ExpressionPtr MakeFuncT(FuncType fn, A0 a0)
        {
            return NewExpr<  Func1<TFunc, A0> >(fn, a0);
        }
        
        template<OpFunc1 TFunc>
        ExpressionPtr MakeFunc(FuncType fn, ExpressionPtr a0)
        {
#if RESOLVE
            ValueType c0;
            if (a0->Resolve(c0)) {
                delete_expr(a0);
                return MakeFuncT<TFunc>(fn, c0);
            }
            
            KernelRegisterPtr v0;
            if (a0->Resolve(v0)) {
                delete_expr(a0);
                return MakeFuncT<TFunc>(fn, v0);
            }
#endif
            return MakeFuncT<TFunc>(fn, a0);
        }
        
        //
        // OpFunc2
        //
        
        template<OpFunc2 TFunc, typename A0, typename A1>
        ExpressionPtr MakeFuncT(FuncType fn, A0 a0, A1 a1)
        {
            return NewExpr<  Func2<TFunc, A0, A1> >(fn, a0, a1);
        }
        
        template<OpFunc2 TFunc, typename A0>
        ExpressionPtr MakeFunc_2(FuncType fn, A0 a0, ExpressionPtr a1)
        {
#if RESOLVE
            ValueType c1;
            if (a1->Resolve(c1)) {
                delete_expr(a1);
                return MakeFuncT<TFunc>(fn, a0, c1);
            }
            
            KernelRegisterPtr v1;
            if (a1->Resolve(v1)) {
                delete_expr(a1);
                return MakeFuncT<TFunc>(fn, a0, v1);
            }
#endif
            return MakeFuncT<TFunc>(fn, a0, a1);
        }
        
        template<OpFunc2 TFunc>
        ExpressionPtr MakeFunc(FuncType fn, ExpressionPtr a0, ExpressionPtr a1)
        {
#if RESOLVE
            ValueType c0;
            if (a0->Resolve(c0)) {
                delete_expr(a0);
                return MakeFunc_2<TFunc>(fn, c0, a1);
            }
            
            KernelRegisterPtr v0;
            if (a0->Resolve(v0)) {
                delete_expr(a0);
                return MakeFunc_2<TFunc>(fn, v0, a1);
            }
#endif
            return MakeFunc_2<TFunc>(fn, a0, a1);
        }
        
        //
        // OpFunc3
        //
        
        template<OpFunc3 TFunc>
        ExpressionPtr MakeFunc(FuncType fn, ExpressionPtr a0, ExpressionPtr a1, ExpressionPtr a2)
        {
            return NewExpr<  Func3<TFunc, ExpressionPtr, ExpressionPtr, ExpressionPtr> >(fn, a0, a1, a2);
        }
        

        template<typename T>
        ExpressionPtr MakeAssignmentT(KernelRegisterPtr var, T a0)
        {
             return NewExpr<RegisterAssignment<T> >(var, a0);
        }

        
        ExpressionPtr MakeAssignment(KernelRegisterPtr var, ExpressionPtr a0)
        {
#if RESOLVE
            ValueType c0;
            if (a0->Resolve(c0)) {
                delete_expr(a0);
                return MakeAssignmentT(var, c0);
            }
            
            KernelRegisterPtr v0;
            if (a0->Resolve(v0)) {
                delete_expr(a0);
                return MakeAssignmentT(var, v0);
            }
#endif
            return MakeAssignmentT(var, a0);
        }

        ExpressionPtr MakeAssignment(ExpressionPtr lhs, ExpressionPtr a0)
        {
            KernelRegisterPtr v;
            if (lhs->Resolve(v))
            {
                delete_expr(lhs);
                return MakeAssignment(v, a0);
            }
            else
            {
                return NewExpr< AssignmentExpression >(lhs, a0);
            }
        }



        ExpressionPtr CreateReference(KernelRegisterPtr var)
        {
            return NewExpr<Expressions::RegisterReference>(var);
        }



        

  
    
    
}
    
    using namespace Expressions;
    

    
    
    
    class KernelCompiler : public CompileContext
    {
    protected:
        
        std::vector<KernelRegisterPtr>                         m_reglist;
        std::unordered_map<std::string, KernelRegisterPtr>     m_regmap;
        
    public:

        
        KernelCompiler()
        {
        }
        
        virtual ~KernelCompiler()
        {
         
        }
        
        

        
        
        
        virtual ExpressionPtr CreateCompiledConstantExpression(ValueType value) override
        {
            return NewExpr<Expressions::ConstantValue>(value);
        }
        
        
        
        
        
        virtual ExpressionPtr CreateBlockStatement(std::vector<ExpressionPtr> &statements) override
        {
            return NewExpr<Expressions::BlockStatement>(statements);
        }
        
    
        
        std::string ToLower(const char *name)
        {
            std::string key(name);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            return key;
        }

        
        virtual KernelRegisterPtr GetRegister(const char *_name)
        {
            auto name = ToLower(_name);
            
            auto it = m_regmap.find(name);
            if (it != m_regmap.end())
            {
                return (*it).second;
            }
            
            return nullptr;
        }
        
        virtual KernelRegisterPtr GetOrCreateRegister(const char *_name)
        {
            auto name = ToLower(_name);

            auto it = m_regmap.find(name);
            if (it != m_regmap.end())
            {
                return (*it).second;
            }
            
            int index =(int)m_reglist.size();
            auto *cv = new KernelRegister(name, index);
            m_regmap[name] = cv;
            m_reglist.push_back(cv);
            return cv;
        }

        
         virtual ExpressionPtr LookupVariable(const char *name) override
         {
             KernelRegisterPtr reg = GetRegister(name);
             if (reg) {
                 return CreateReference(reg);
             } else {
                 return nullptr;
             }
         }
                
        virtual ExpressionPtr AddVariable(const char *name)  override
        {
            KernelRegisterPtr reg = GetOrCreateRegister(name);
            return CreateReference(reg);
        }

  
        
        
        virtual ExpressionPtr CreateCompiledExpression(FuncType fn,
                                                       ExpressionPtr code1,
                                                       ExpressionPtr code2,
                                                       ExpressionPtr code3) override
        {
            switch (fn)
            {
                case FN_ASSIGN:
                    return MakeAssignment(code1, code2);
                case FN_ADD:
                    return MakeBinaryOp<fn_add>(fn, code1, code2);
                case FN_SUB:
                    return MakeBinaryOp<fn_sub>(fn, code1, code2);
                case FN_MULTIPLY:
                    return MakeBinaryOp<fn_mul>(fn, code1, code2);
                case FN_DIVIDE:
                    return MakeBinaryOp<fn_div>(fn, code1, code2);
                case FN_MODULO:
                    return MakeFunc<fn__mod>(fn, code1, code2);
                case FN_BITWISE_AND:
                    return MakeFunc<fn_bitwise_and>(fn, code1, code2);
                case FN_BITWISE_OR:
                    return MakeFunc<fn_bitwise_or>(fn, code1, code2);
                case FN_UPLUS:
                    return MakeUnaryOp<fn__uplus>(fn, code1);
                case FN_UMINUS:
                    return MakeUnaryOp<fn__uminus>(fn, code1);
                    
                case EFN_IF:
                    return NewExpr<If>(code1, code2, code3);
                case EFN_LOGICAL_AND:
                    return NewExpr<LogicalAnd>(fn, code1, code2);
                case EFN_LOGICAL_OR:
                    return NewExpr<LogicalOr>(fn, code1, code2);
                case EFN_LOOP:
                    return NewExpr<Repeat>(code1, code2);
                case EFN_WHILE:
                    return NewExpr<RepeatWhile>(code1);
                    
                case EFN_NOT:
                    return MakeFunc<fn__not>(fn, code1);
                case EFN_EQUAL:
                    return MakeFunc<fn__equal>(fn, code1, code2);
                case EFN_NOTEQ:
                    return MakeFunc<fn__noteq>(fn, code1, code2);
                case EFN_ABOVE:
                    return MakeFunc<fn__above>(fn, code1, code2);
                case EFN_BELOW:
                    return MakeFunc<fn__below>(fn, code1, code2);
                case EFN_BELEQ:
                    return MakeFunc<fn__beleq>(fn, code1, code2);
                case EFN_ABOEQ:
                    return MakeFunc<fn__aboeq>(fn, code1, code2);
                    
                case EFN_SET:
                    return MakeAssignment(code1, code2);
                case EFN_MOD:
                    return MakeFunc<fn__mod>(fn, code1, code2);
                    
                case EFN_MULOP:
                    return MakeAssignBinaryOp<fn_mul>(fn, code1, code2);
                case EFN_DIVOP: // /=
                    return MakeAssignBinaryOp<fn_div>(fn, code1, code2);
                case EFN_OROP:  // |=
                    return MakeAssignBinaryOp<fn_bitwise_or>(fn, code1, code2);
                case EFN_ANDOP: // &=
                    return MakeAssignBinaryOp<fn_bitwise_and>(fn, code1, code2);
                case EFN_ADDOP: // +=
                    return MakeAssignBinaryOp<fn_add>(fn, code1, code2);
                case EFN_SUBOP: // -=
                    return MakeAssignBinaryOp<fn_sub>(fn, code1, code2);
                case EFN_MODOP: // %=
                    return MakeAssignBinaryOp<fn__mod>(fn, code1, code2);
                case EFN_POWOP: // ^=
                    return MakeAssignBinaryOp<fn_pow>(fn, code1, code2);

                case EFN_SIN:
                    return MakeFunc<fn_sin>(fn, code1);
                case EFN_COS:
                    return MakeFunc<fn_cos>(fn, code1);
                case EFN_TAN:
                    return MakeFunc<fn_tan>(fn, code1);
                case EFN_ASIN:
                    return MakeFunc<fn_asin>(fn, code1);
                case EFN_ACOS:
                    return MakeFunc<fn_acos>(fn, code1);
                case EFN_ATAN:
                    return MakeFunc<fn_atan>(fn, code1);
                case EFN_ATAN2:
                    return MakeFunc<fn_atan2>(fn, code1, code2);
                    
                case EFN_SQR:
                    return MakeFunc<fn_sqr>(fn, code1);
                case EFN_SQRT:
                    return MakeFunc<fn_sqrt>(fn, code1);
                case EFN_EXP:
                    return MakeFunc<fn_exp>(fn, code1);
                case EFN_POW:
                    return MakeFunc<fn_pow>(fn, code1, code2);
                case EFN_LOG:
                    return MakeFunc<fn_log>(fn, code1);
                case EFN_LOG10:
                    return MakeFunc<fn_log10>(fn, code1);
                case EFN_ABS:
                    return MakeFunc<fn_abs>(fn, code1);
                case EFN_MIN:
                    return MakeFunc<fn_min>(fn, code1, code2);
                case EFN_MAX:
                    return MakeFunc<fn_max>(fn, code1, code2);
                case EFN_SIGN:
                    return MakeFunc<fn_sign>(fn, code1);
                case EFN_RAND:
                {
                    
                    const char *random_name = "_random_";
                    KernelRegisterPtr random_reg = GetOrCreateRegister(random_name);
                    return NewExpr<IntRandomOp>(fn, code1, random_reg);
                    
//                    return MakeFunc<fn_rand>(fn, code1);
                }
                case EFN_FLOOR:
                    return MakeFunc<fn_floor>(fn, code1);
                case EFN_CEIL:
                    return MakeFunc<fn_ceil>(fn, code1);
                case EFN_INVSQRT:
                    return MakeFunc<fn_invsqrt>(fn, code1);
                case EFN_SIGMOID:
                    return MakeFunc<fn_sigmoid>(fn, code1, code2);
                case EFN_BAND:
                    return MakeFunc<fn_band>(fn, code1, code2);
                case EFN_BOR:
                    return MakeFunc<fn_bor>(fn, code1, code2);
                case EFN_ASSERT_EQUALS:
                    return MakeFunc<fn_assert_equals>(fn, code1, code2);
                case EFN_EXEC2:
                    return NewExpr<Exec2Expression>(code1, code2);
                    
                case EFN_EXEC3:
                    return NewExpr<Exec3Expression>(code1, code2, code3);

                case EFN_MEM:
                    return NewExpr<Megabuf>(code1);

                case EFN_GMEM:
                    return NewExpr<GMegabuf>(code1);
                    
                case EFN_FREEMBUF:
                case EFN_MEMCPY:
                case EFN_MEMSET:
                default:
                    return NewExpr<InvalidExpression>(fn);
            }
            
            assert(0);
            return nullptr;
        }
        
        
        
        
        virtual Script::IKernelPtr Compile(ScriptContextPtr vm, const std::string &name, const std::string &code, std::string &errors)
        {
            StopWatch sw;
            
            
            // register all parameters variables
            for (auto var : vm->GetVariables())
            {
                if (var->Usage == VarUsage::Parameter)
                {
                    KernelRegisterPtr cv = GetOrCreateRegister(var->GetName().c_str());
                    cv->vmvar = var;
                }
            }
        
            ExpressionPtr expr = CompileWithPreprocessor(code, errors);
            
            
            std::shared_ptr<Kernel> compiled = std::make_shared<Kernel>(vm, name, code, expr, m_reglist);
            compiled->compile_time = sw.GetElapsedMilliSeconds();
            return compiled;
        }

    }; // class KernelCompiler
    
    
    
    Script::IKernelPtr Compile(ScriptContextPtr vm, const std::string &name, const std::string &code, std::string &errors)
    {
        KernelCompiler compiler;
        return compiler.Compile(vm, name, code, errors);
    }


}} // namespace
