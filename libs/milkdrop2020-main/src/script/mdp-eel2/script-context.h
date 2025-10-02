
#pragma once


#include "../script.h"




namespace Script { namespace mdpx {

    class Kernel;
    
    class Variable
    {
        ValueTypePtr          m_valueptr;
        const std::string     m_name;
    public:
        
        const VarUsage Usage;
        
        bool IsReadByKernel = false;
        bool IsWrittenByKernel = false;

        const std::string &GetName() const
        {
            return m_name;
        }
        
        
        inline ValueType GetValue()
        {
            return *m_valueptr;
        }
        
        inline void SetValue(ValueType v)
        {
            *m_valueptr = v;
        }
        
        
        inline ValueTypePtr GetValuePtr()
        {
            return m_valueptr;
        }
        
        Variable(const std::string &name, ValueTypePtr valueptr, VarUsage usage)
        : m_valueptr(valueptr), m_name(name), Usage(usage)
        {
            
        }
        
        virtual ~Variable() = default;
        
        
        void Print(std::ostream &o)
        {
            o << m_name;
        }
        
    };
    
    
    using VariablePtr = Variable *;
    using VariableUPtr = std::unique_ptr<Variable>;
    
    
    inline std::ostream & operator <<(std::ostream &o, VariablePtr v)
    {
        return o << v->GetName();
    }
    
    
    class LocalVariable : public Variable
    {
    public:
        LocalVariable(const std::string &name)
        :  Variable(name, &m_value, VarUsage::Local)
        {
     
        }
        
        virtual ~LocalVariable()
        {
            
        }
        
    protected:
        ValueType m_value = 0;
        
    };
    
    
    
    
    
    class ScriptContext : public Script::IContext, public std::enable_shared_from_this<ScriptContext>
    {
    public:
        ScriptContext(std::string name);
        virtual ~ScriptContext();
        
        virtual const std::string &GetName() const override {return m_name;}
        
        virtual void ResetStats() override;
        virtual void DumpStats(int verbose = 0) override;
//        
//        void ClearVars()
//        {
//            m_varmap.clear();
//        }
        
        
        virtual void DebugUI() override;
        
        virtual IKernelPtr  Compile(const std::string &name, const std::string &code) override;
  

        
        virtual void RegVar(const std::string &name, Var &var, VarUsage usage) override
        {
            RegisterVar(name, &var, usage);
        }
        
     

        VariablePtr LookupVar(const std::string &name)
        {
            auto it = m_varmap.find(name);
            if (it == m_varmap.end())
                return nullptr;
            
            return (*it).second;
        }

        VariablePtr RegisterVar(const std::string &name, ValueTypePtr vptr, VarUsage usage)
        {
            auto it = m_varmap.find(name);
            if (it != m_varmap.end())
            {
                assert(0); // duplicate
                return (*it).second;
            }
            
            // assign 0 to variable
            *vptr = 0;
            
            VariablePtr variable = new Variable(name, vptr, usage);
            m_varlist.push_back(variable);
            m_varmap[variable->GetName()] = variable;
            return variable;
        }
        
        
        
        VariablePtr RegisterLocalVar(const std::string &name)
        {
            
            auto it = m_varmap.find(name);
            if (it != m_varmap.end())
            {
                assert(0); // duplicate
                return (*it).second;
            }
            
            
            LocalVariable *variable = new LocalVariable(name);
            m_varlist.push_back(variable);
            m_varmap[variable->GetName()] = variable;
            return variable;
        }
        
        
        const std::vector<VariablePtr> &GetVariables() const { return m_varlist; };


        virtual void SaveState(ValueType *state, size_t count) override;
        virtual void LoadState(const ValueType *state, size_t count) override;

        
    protected:
        std::string m_name;

        std::vector<VariablePtr> m_varlist;
        std::unordered_map<std::string, VariablePtr>     m_varmap;
        

    };
    
    using ScriptContextPtr = std::shared_ptr<ScriptContext>;
    
    
    
}} // namespace Script





