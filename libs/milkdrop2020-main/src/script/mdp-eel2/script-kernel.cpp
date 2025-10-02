
#include "platform.h"
#include "script-kernel.h"
#include "script-functype.h"
#include "script-functions.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#define USE_JAVASCRIPT 0
#else
#define USE_JAVASCRIPT 0
#endif


#include <mutex>
#include <thread>
#include <future>         // std::async, std::future


namespace Script { namespace mdpx {

    
void Kernel::DebugUI()
{

    ImGui::TextWrapped("%s\n", GetSource().c_str());

    /*
    
    ImGui::TextWrapped("Disassembly:\n%s\n", GetDisassembly().c_str());

    ImGui::Separator();

        for (auto reg : m_registers)
        {
            ImGui::InputScalar(reg->GetName().c_str(),
                               ImGuiDataType_Double,
                               reg->vmvar->GetValuePtr()
                                 );
    
        }
    */

    
//    for (auto var : m_variables)
//    {
//        ImGui::InputScalar(var->GetName().c_str(),  ImGuiDataType_Double,
//                           &m_state[var->GetIndex()]
//                             );
//
//    }
//
    
    
//    ImGui::Text("eval_count:%d\n", eval_count);
//    ImGui::Text("eval_time:%.2f\n",eval_time);

    /*
    int count[EFN_total] = {0};
    double timing[EFN_total] = {0.0};
    for (size_t i=0; i < m_expressions.size(); i++)
    {
        auto e = m_expressions[i];
        if (e->eval_count > 0)
        {
            count[e->GetType()]+= e->eval_count;
            timing[e->GetType()] += e->eval_time.GetElapsedMicroSeconds();
        }
    }
    for (int i=0; i < EFN_total; i++)
    {
        if (count[i] > 0) {
            double ave = timing[i] / (double)count[i];
            printf("count:%-8d time:%-10.2f ave:%-10.2f fn:'%s' \n", count[i] , timing[i], ave, GetFnName( (FuncInfo) i ));
        }
        //                std::cout << e << std::endl;
    }
     */
  
   // ImGui::TextWrapped("Source:\n%s\n", m_source.c_str());
//    printf("source:\n%s\n", m_source.c_str());
    
   // std::cout << "decompiled:\n" << m_expr << std::endl;
}
    
    // cheap hash
    static uint64_t ComputeHash(const std::string &str)
    {
        uint64_t hash = 1;
        for (auto c : str)
        {
            hash *= 33;
            hash += (uint64_t)c;
        }
        if (!hash) hash = 1;
        return hash;
    }

    
    Kernel::Kernel(
           ScriptContextPtr vm,
           const std::string &name,
           const std::string &source,
           ExpressionPtr expr,
           const std::vector<KernelRegisterPtr> &registers)
    :
    m_name(name),
    m_vm(vm),
    m_source(source), m_expr(expr),
    m_registers(registers)
    {
        // resolve all variables
        for (auto reg : m_registers)
        {
            VariablePtr vmvar = vm->LookupVar(reg->GetName());
            if (!vmvar)
            {
                vmvar = vm->RegisterLocalVar(reg->GetName() );
            }
            
            reg->vmvar = vmvar;
            
            if (reg->IsRead)
                vmvar->IsReadByKernel = true;
            if (reg->IsWrite)
                vmvar->IsWrittenByKernel = true;

            reg->IsLocal = vmvar->Usage == Script::VarUsage::Local;
            reg->IsParameter = vmvar->Usage == Script::VarUsage::Parameter;
            
            if (vmvar->Usage == Script::VarUsage::Parameter)
            {
                m_parameters.push_back(reg);
            }
            
            m_register_values.push_back( vmvar->GetValuePtr() );
        }
        
        m_state.resize( m_registers.size() );
  
    }
    
    Kernel::~Kernel()
    {
        // delete root expression
        delete m_expr;
        
        // delete all registers
        for (auto reg : m_registers)
        {
            delete reg;
        }
        m_registers.clear();
    }
    
    
    const char *ToString(Script::VarUsage usage)
    {
        switch (usage)
        {
            case VarUsage::Global : return "Global";
            case VarUsage::Local : return "Local";
            case VarUsage::Parameter : return "Param";
            default:
                return "<unknown>";
        }
    }
    
    
    void Kernel::DumpStats(int verbose)
    {
        if (m_vm)
            LogPrint("context %s\n",
              m_vm->GetName().c_str());

        LogPrint("eval_count:%d eval_time:%.2f eval_ave:%f\n",
               eval_count, eval_time, (double)eval_time / (double)eval_count);

        if (verbose)
        for (auto reg : m_registers)
        {
            auto vmvar = reg->vmvar;
            double value = vmvar->GetValue();
            LogPrint("register %-7s %-16s %-10.4f (%s%s%s%s)\n",
                   ToString(vmvar->Usage),
                   reg->GetName().c_str(),
                   value,
                   reg->IsRead ? "read " : "",
                   reg->IsWrite ? "write " : "",
                   reg->IsReadBeforeWrite  ? "feedback " : "",
                   reg->IsWriteBeforeRead  ? "clobber " : ""

               );
            
            
            
            
        }
        
//        for (auto local : m_locals)
//        {
//            printf("local %d\n" , local);
//        }
//
//        for (auto global : m_globals)
//        {
//            printf("global %d\n" , global);
//        }

        int compile_count[EFN_total] = {0};

        int eval_count[EFN_total] = {0};
        double timing[EFN_total] = {0.0};
//        for (size_t i=0; i < m_expressions.size(); i++)
//        {
//            auto e = m_expressions[i];
//
//            compile_count[e->GetType()]++;
//            if (e->eval_count > 0)
//            {
//                eval_count[e->GetType()]+= e->eval_count;
//                timing[e->GetType()] += std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(e->eval_time).count(); ;
//            }
//        }
        for (int i=0; i < EFN_total; i++)
        {
            if (compile_count[i] > 0) {
                double ave = timing[i] / (double)eval_count[i];
                printf("count:%-8d eval_count:%-8d time:%-10.2f ave:%-10.6f fn:'%s' \n",
                       compile_count[i], eval_count[i] , timing[i], ave, GetFuncName( (FuncType) i ));
            }
            //                std::cout << e << std::endl;
        }
        printf("source:\n%s\n", m_source.c_str());
        
        std::cout << "decompiled:\n" << m_expr << std::endl;
    }
    
    void Kernel::DumpState(const ValueType *state)
    {
        
        
        for (auto reg : m_registers)
        {
            auto vmvar = reg->vmvar;
            double value = state[ reg->GetIndex() ];
            printf("register %-7s %-16s %-10.4f (%s%s%s%s)\n",
                   ToString(vmvar->Usage),
                   reg->GetName().c_str(),
                   value,
                   reg->IsRead ? "read " : "",
                   reg->IsWrite ? "write " : "",
                   reg->IsReadBeforeWrite  ? "feedback " : "",
                   reg->IsWriteBeforeRead  ? "clobber " : ""
                   );
           
        }
       
    }

    
    void Kernel::SaveRegisters(ValueType *state, size_t count)
    {
        count = std::min(count, m_register_values.size() );

        const ValueTypePtr *values= m_register_values.data();
        // capture VM state
        for (size_t i=0; i < count; i++)
        {
//            KernelRegisterPtr reg = m_registers[i];
//            state[i] = reg->vmvar->GetValue();
            
            state[i] = *(values[i]);
        }
    }

    void Kernel::LoadRegisters(const ValueType *state, size_t count)
    {
        count = std::min(count, m_register_values.size() );
        
        ValueTypePtr *values= m_register_values.data();

        // restore VM state
        for (size_t i=0; i < count; i++)
        {
            *(values[i]) = state[i];
//            KernelRegisterPtr reg = m_registers[i];
//            reg->vmvar->SetValue(state[i]);
        }

    }
    

    void Kernel::Execute(ValueType *state)
    {
        if (!m_expr) {
            return;
        }
 
        
        m_expr->Evaluate(state);
    }
    
    
    void Kernel::Execute(ValueType *state, ValueType *params, size_t param_count, size_t iter_count)
    {
        PROFILE_FUNCTION_CAT("script")

        for (size_t iter=0; iter < iter_count; iter++)
        {
            // set registers
            for (size_t p = 0; p < param_count; p++)
            {
                state[p] = params[p];
            }
            
            // evaluate
            if (m_expr)
                m_expr->Evaluate(state);

            // capture registers
            for (size_t p = 0; p < param_count; p++)
            {
                params[p] = state[p];
            }
            // next block
            params += param_count;
        }
    }
    
#if 0
    void Kernel::ExecuteNSEEL(const ValueType *input_state, ValueType *output_state, const ValueType *input, ValueType *output, size_t param_count, size_t iter_count)
    {
        // set all registers
        if (input_state)
        for (size_t i=0; i < m_registers.size(); i++)
        {
            auto reg = m_registers[i];
            *reg->vmvar->m_nseel_valueptr = input_state[i];
        }
        
        for (size_t iter=0; iter < iter_count; iter++)
        {
            // set parameters
            for (size_t j=0; j < param_count; j++)
            {
                auto reg = m_parameters[j];
                *reg->vmvar->m_nseel_valueptr = *input++;
            }
            
            NSEEL_code_execute(m_nseel_code);
            
            
            // set parameters
            for (size_t j=0; j < param_count; j++)
            {
                auto reg = m_parameters[j];
                *output++ = *reg->vmvar->m_nseel_valueptr;
            }
            
        }
        
        
        // capture state
        if (output_state)
        for (size_t i=0; i < m_registers.size(); i++)
        {
            auto reg = m_registers[i];
            output_state[i] = *reg->vmvar->m_nseel_valueptr;
        }
        
    }
#endif
    
     
    
    
    
    void Kernel::Execute()
    {
        if (!m_expr) {
            return;
        }
    
        StopWatch sw;
        
        
        
        // capture VM state
        SaveRegisters(m_state.data(), m_state.size());
        
        Execute(m_state.data());
        
        // restore vm state
        LoadRegisters(m_state.data(), m_state.size());

        
        eval_count++;
        eval_time += sw.GetElapsedMilliSeconds();
    }
    
    
    uint64_t Kernel::GetHash()
    {
        if (IsEmpty())
            return 0;
        
        if (!m_hash) {
            std::string disasm = GetDisassembly();
            m_hash = ComputeHash(disasm);
        }
        return m_hash;
    }
    
    
    const std::string &Kernel::GetDisassembly()
    {
        if (m_disassembly.empty())
        {
            std::stringstream ss;
            ss << m_expr;
            m_disassembly = ss.str();
        }
        return m_disassembly;
        
    }
    
        
    class KernelBuffer : public IKernelBuffer, public std::enable_shared_from_this<KernelBuffer>
    {
    public:
        KernelBuffer( KernelPtr kernel)
        : m_kernel(kernel), m_paramCount(kernel->GetParameterCount())
        {
        }
        
        virtual ~KernelBuffer()
        {
            Sync();
        }
        
        virtual int  GetSize() const override
        {
            return m_count;
        }
        
        virtual void CaptureState() override
        {
            // save state
            m_kernel->SaveRegisters(m_state.data(), m_state.size());
        }
        
        virtual void RestoreState() override
        {
            // save state
            m_kernel->LoadRegisters(m_state.data(), m_state.size());
        }

        
        virtual void Resize(int count) override
        {
            assert (!m_execute);
            
            m_count = count;
            m_data.resize(m_count * m_paramCount);
            
            m_state.resize( m_kernel->GetRegisterCount() );
        }
        
        virtual void StoreJob(int i) override
        {
            assert (!m_execute);
            ValueType *input = &m_data[i * m_paramCount];
            m_kernel->SaveRegisters( input , m_paramCount);
        }
        
        virtual void ReadJob(int i) override
        {
            assert (!m_execute);
            const ValueType *output = &m_data[i * m_paramCount];
            m_kernel->LoadRegisters( output, m_paramCount);
        }
        
        bool RunAsync =  true;
        

        virtual void Sync() override
        {
#if PLATFORM_SUPPORTS_THREADS
            if (m_future.valid()) {
                m_future.wait();
            }
#endif
            if (m_execute)
            {
                RestoreState();
            }
            m_execute =  false;
        }

        virtual void ExecuteAsync() override
        {
            //std::lock_guard<std::mutex> lock(m_mutex);
            assert(!m_execute);
            
            // wait for previous job to complete........
            Sync();
            
            m_execute = true;
            
            CaptureState();

            
#if PLATFORM_SUPPORTS_THREADS
            if (RunAsync)
            {
                auto self = shared_from_this();

                m_future = std::async( std::launch::async,
                                      [self] {
                                          self->DoExecute();
                                          return true;
                                      }
                            );
            }
            else
#endif
            {
                DoExecute();
            }
        }
        
        
        
        
        void DoExecute()
        {
      
            StopWatch sw;

            m_kernel->Execute(m_state.data(), m_data.data(),
                                      m_paramCount, m_count);
            

            eval_time = sw.GetElapsedMilliSeconds();
        }
        
#if 0
        
        bool DeltaOutput(const std::vector<ValueType> &a, const std::vector<ValueType> &b, int iter_count)
        {
            
            bool diff = false;
            
            auto params = m_kernel->GetParameters();
            for (size_t iter=0; iter < iter_count; iter++)
            {
                const ValueType *input = &a[iter * m_paramCount];
                const ValueType *output = &b[iter * m_paramCount];
                
                
                for (int j=0; j < m_paramCount; j++)
                {
                    auto reg = params[j];
                    if (!Functions::IsEqual(input[j], output[j]))
                    {
                        printf("kernel[%04d] ", (int)iter);
                        double delta = output[j] - input[j];
                        printf("%s: %f!=%f (%f)", reg->GetName().c_str(), input[j], output[j], delta);
                        printf("\n");
                        diff = true;
                    }
                }
            }
            
            return diff;
            
            
        }

        void TestCompareExecute()
        {
            std::vector<ValueType> mdx_state;
            std::vector<ValueType> mdx_output;
            
            
            std::vector<ValueType> nseel_state;
            std::vector<ValueType> nseel_output;
            
            nseel_state.resize(m_input_state.size());
            mdx_state.resize(m_input_state.size());

            nseel_output.resize(m_data.size());
            mdx_output.resize(m_data.size());
            
            int iter_count = m_count;
            
            
            g_FuncDebug =0;

            m_kernel->Execute(m_input_state.data(),
                                      m_data.data(),  m_paramCount, iter_count);
            
            
            m_kernel->ExecuteNSEEL(m_input_state.data(),  nseel_state.data(),
                                   m_data.data(), nseel_output.data(), m_paramCount, iter_count);
            
            
            if (DeltaOutput(mdx_output, nseel_output, 1))
            {
                // compare state
                for (size_t i=0; i < mdx_state.size(); i++)
                {
                    if (!Functions::IsEqual(nseel_state[i], mdx_state[i]))
                    {
                        auto reg = m_kernel->GetRegisters()[i];
                        printf("%s: %f!=%f\n", reg->GetName().c_str(), mdx_state[i], nseel_state[i]);
                    }
                }
                m_kernel->DumpStats(0);
                printf("mdx_state:\n");
                m_kernel->DumpState(mdx_state.data());
                printf("nseel_state:\n");
                m_kernel->DumpState(nseel_state.data());
                
            }
            
            g_FuncDebug = 0;
        }
#endif
        
        void DumpOutput()
        {
            m_kernel->DumpStats();
            
            auto params = m_kernel->GetParameters();
            for (size_t i=0; i < m_count; i++)
            {
                ValueType *input = &m_data[i * m_paramCount];
                ValueType *output = &m_data[i * m_paramCount];

                printf("kernel[%04d] ", (int)i);
                
                for (int j=0; j < m_paramCount; j++)
                {
                    auto reg = params[j];
                    if (!Functions::IsEqual(input[j], output[j]) )
                    {
                        printf("%s:%12.6f ", reg->GetName().c_str(), output[j]);
                    }
                }
                printf("\n");
            }
        }
    
        
        double eval_time = 0.0;
        
        
        virtual double GetExecuteTime() override
        {
            return eval_time;
        }
        
        virtual void DumpStats(int verbose) override
        {
            printf("kernelbuffer '%s' count:%d eval_time:%.2f eval_ave:%f\n",
                   m_kernel->GetName().c_str(),
                   m_count,  eval_time, (double)eval_time);
        }
        
        virtual void DebugUI() override
        {
            
        }
        
    protected:
        std::future<bool> m_future;
        

        KernelPtr           m_kernel;
        size_t             m_paramCount = 0;
        int                m_count = 0;
        
        bool                m_execute = false;

        std::vector<ValueType>        m_state;
        std::vector<ValueType>       m_data;

    };
    
    
    IKernelBufferPtr Kernel::CreateBuffer()
    {
        return std::make_shared<KernelBuffer>( shared_from_this() );
        
    }
    


    

}} // namespace

