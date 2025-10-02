
#pragma once

#include "script-context.h"
#include "script-expression.h"

#include <sstream>

namespace Script { namespace mdpx {
    
    
    
    class KernelRegister
    {
    public:
        KernelRegister(const std::string &name, int index)
        : m_name(name), m_index(index)
        {
        }
        
        void MarkWritten()
        {
            if (!IsWrite && IsRead)
            {
                IsReadBeforeWrite = true;
            }
            IsWrite = true;
        }
        
        void MarkRead()
        {
            if (!IsRead && IsWrite)
            {
                IsWriteBeforeRead  = true;
            }
          
            IsRead = true;
        }
        
        // vmvar binding
        VariablePtr    vmvar = nullptr;

        bool IsRead    = false;         // true if variable is read
        bool IsWrite    = false;         // true if variable is written
        bool IsReadBeforeWrite =false;  // true if read before written (needs to be set before kernel execution)
        bool IsWriteBeforeRead =false;  // true if clobbered, no need to initialize

        
        bool IsLocal   = false;
        bool IsParameter = false;
        
        int GetIndex() const                {return m_index;}
        const std::string &GetName() const  {return m_name;}
        
        void Print(std::ostream &o)
        {
            if (IsLocal) {
                o << "L_";
            } else if (IsParameter) {
                o << "P_";
            } else {
                o << "V_";
            }
            o << m_name;
        }
        
        inline ValueType GetValue(EvalContext ec)
        {
            return ec[m_index];
        }
        
        inline void SetValue(EvalContext ec, ValueType value)
        {
            ec[m_index] = value;
        }
        
        inline void SetValueChecked(EvalContext ec, ValueType value)
        {
            if (!isfinite(value))
            {
                value = 0.0;
            }
            ec[m_index] = value;
        }
    protected:
        std::string m_name;
        int         m_index;
    };
    
    using KernelRegisterPtr = KernelRegister *;
    
    inline std::ostream & operator <<(std::ostream &o, KernelRegisterPtr cv)
    {
        cv->Print(o);
        return o;
    }
    


    
    class Kernel : public Script::IKernel, public std::enable_shared_from_this<Kernel>
    {
    public:
        
        Kernel(
               ScriptContextPtr vm,
               const std::string &name,
               const std::string &source,
               ExpressionPtr expr,
               const std::vector<KernelRegisterPtr> &registers);
        
        virtual ~Kernel();
        
        

        virtual bool IsEmpty() override 
        {
            return m_expr == nullptr;
        }
        

        virtual void Execute() override;
        virtual void Execute(ValueType *state) override;
        virtual void Execute(ValueType *state, ValueType *params, size_t param_count, size_t iter_count) override;

        virtual void DumpState(const ValueType *state) override;

        
        virtual void DebugUI() override;
        virtual void DumpStats(int verbose = 0) override;

        

        virtual uint64_t GetHash() override;
        
        virtual const std::string &GetDisassembly() override;
        virtual const std::string &GetSource() const override
        {
            return m_source;
        }
        
        
        
        double  compile_time = 0.0;
        int     eval_count = 0;
        double  eval_time = 0.0;
        
       
        
        
        virtual IKernelBufferPtr CreateBuffer() override;
        
        virtual size_t GetRegisterCount() const override
        {
            return m_registers.size();
        }
        
        virtual size_t GetParameterCount() const override
        {
            return m_parameters.size();
        }
        
        virtual void SaveRegisters(ValueType *state, size_t count) override;
        virtual void LoadRegisters(const ValueType *state, size_t count) override;
        

        virtual const std::string &GetName() const override {return m_name;}

        
        const std::vector<KernelRegisterPtr> &GetParameters() const {return m_parameters;};
        const std::vector<KernelRegisterPtr> &GetRegisters() const {return m_registers;};

    protected:
        std::string                     m_name;
        ScriptContextPtr                m_vm;
        std::string                     m_source;
        std::string                     m_disassembly;
        ExpressionPtr                   m_expr;
        std::vector<KernelRegisterPtr>  m_registers;
        std::vector<ValueTypePtr>       m_register_values;
        uint64_t                        m_hash = 0;
        

        std::vector<ValueType>      m_state;
        
        
        std::vector<KernelRegisterPtr>            m_parameters;
    
    };
    
    using KernelPtr = std::shared_ptr<Kernel>;
    
    
    
    
}} // namespace Script





