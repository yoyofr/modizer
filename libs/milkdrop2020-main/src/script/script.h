#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <unordered_map>
#include <limits.h>
#include "MDcommon.h"
#include "platform.h"
#include "StopWatch.h"


namespace Script
{
    using ValueType = double;
    using ValueTypePtr = ValueType *;
    using ConstValueTypePtr = const ValueType *;
    using Var = ValueType;

    
    class IContext;
    class IKernel;
    class IKernelBuffer;

    using IContextPtr = std::shared_ptr<IContext>;
    using IKernelPtr = std::shared_ptr<IKernel>;
    using IKernelBufferPtr = std::shared_ptr<IKernelBuffer>;
    
    struct Color
    {
        ValueType r,g,b,a;
    };

    struct Vector2
    {
        ValueType x,y;
    };
    
    struct Vector4
    {
        ValueType x,y,z,w;
    };
    
    using EvalContext = ValueTypePtr;

    
    typedef void (*KernelFunc)(ValueTypePtr _state, ValueTypePtr _params, size_t _param_count, size_t _iter_count);

    struct KernelFuncEntry
    {
        uint64_t    hash;
        KernelFunc  func_ptr;
    };


    enum class VarUsage
    {
        Parameter = 0,
        Global = 1,
        Local = 2,
    };
    
    class IContext
    {
    public:
        IContext()
        {
            InstanceCounter<IContext>::Increment();
        }
        virtual ~IContext()
        {
            InstanceCounter<IContext>::Decrement();
        }
        
        virtual const std::string &GetName() const = 0;

        
        virtual void SaveState(ValueType *state, size_t count) = 0;
        virtual void LoadState(const ValueType *state, size_t count) = 0;

        
        virtual void RegVar(const std::string &name, Var &var, VarUsage usage = VarUsage::Global) = 0;

        virtual void RegVars(const std::string &prefix, int start, int count, Var *vars)
        {
            for (int i=0; i < count; i++)
            {
                int index = start + i;
                
                std::string name = prefix + std::to_string(index);
                RegVar(name, vars[i] );
            }
        }
        

        
        virtual void RegVar(const std::string &name, Color &color, VarUsage usage = VarUsage::Global)
        {
            RegVar(name + "r", color.r, usage);
            RegVar(name + "g", color.g, usage);
            RegVar(name + "b", color.b, usage);
            RegVar(name + "a", color.a, usage);
        }
        
        virtual void RegVar(const std::string &name, Vector2 &v, VarUsage usage = VarUsage::Global)
        {
            RegVar(name + "x", v.x, usage);
            RegVar(name + "y", v.y, usage);
        }
        
        virtual void RegVar(const std::string &name, Vector4 &v, VarUsage usage = VarUsage::Global)
        {
            RegVar(name + "x", v.x, usage);
            RegVar(name + "y", v.y, usage);
            RegVar(name + "z", v.z, usage);
            RegVar(name + "w", v.w, usage);
        }
        
        

        virtual IKernelPtr  Compile(const std::string &name, const std::string &code) = 0;

        
        virtual void ResetStats() = 0;
        
        virtual void DumpStats(int verbose = 0) = 0;
        virtual void DebugUI() = 0;
    };
    
    
    
    class IKernelBuffer
    {
    public:
        IKernelBuffer() = default;
        virtual ~IKernelBuffer() = default;
        
        virtual int  GetSize() const = 0;
        virtual void Resize(int capacity) = 0;
        virtual void CaptureState() = 0;
        virtual void RestoreState() = 0;
        virtual void StoreJob(int i) = 0;
        virtual void ExecuteAsync() = 0;

        virtual void Sync() = 0;
        virtual void ReadJob(int i) = 0;
        
        virtual double GetExecuteTime() = 0;

        
        virtual void DumpStats(int verbose = 0) = 0;
        virtual void DebugUI() = 0;
    };
    
    
    using IKernelBufferPtr = std::shared_ptr<IKernelBuffer>;
    
    class IKernel
    {
    public:
        IKernel() = default;
        virtual ~IKernel() = default;

        virtual bool IsEmpty() = 0;

        virtual IKernelBufferPtr CreateBuffer() = 0;

        virtual size_t GetRegisterCount() const = 0;
        virtual void SaveRegisters(ValueType *state, size_t count) = 0;

        virtual size_t GetParameterCount() const = 0;
        virtual void LoadRegisters(const ValueType *state, size_t count) = 0;

        virtual void Execute() = 0;
        virtual void Execute(ValueType *state) = 0;
        virtual void Execute(ValueType *state, ValueType *params, size_t param_count, size_t iter_count) = 0;
                             
        virtual void DumpState(const ValueType *state) = 0;
        
        
        virtual uint64_t GetHash() = 0;
        virtual const std::string &GetName() const = 0;
        virtual const std::string &GetSource() const = 0 ;
        virtual const std::string &GetDisassembly() = 0;
        virtual void DumpStats(int verbose = 0) = 0;
        virtual void DebugUI() = 0;
        
        

    };


    
    namespace mdpx {
        IContextPtr         CreateContext(std::string name);
        int GetExpressionCount();
    }

    
} // namespace Script



