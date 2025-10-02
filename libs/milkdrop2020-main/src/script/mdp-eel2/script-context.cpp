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


#include "script-context.h"
#include "script-compiler.h"
#include "script-kernel.h"

namespace Script { namespace mdpx {

    Script::IKernelPtr Compile(ScriptContextPtr vm, const std::string &name, const std::string &code, std::string &errors);

    IContextPtr CreateContext(std::string name)
    {
        return std::make_shared<ScriptContext>(name);
    }

    
    ScriptContext::ScriptContext(std::string name)
    :m_name(name)
    {

    }


    ScriptContext::~ScriptContext()
    {
        for (auto var : m_varlist)
        {
            delete var;
        }
        m_varlist.clear();
        
        m_varmap.clear();
    }

    

    IKernelPtr ScriptContext::Compile(const std::string &name, const std::string &code)
    {
        std::string full_name= GetName() + "_" + name;
        
        std::string errors;
        IKernelPtr  func = ::Script::mdpx::Compile(shared_from_this(), full_name, code, errors);
        if (!func) {
            return nullptr;
        }
        return func;
    }
    
    void ScriptContext::SaveState(ValueType *state, size_t count)
    {
        count = std::min(count, m_varlist.size());
        for (size_t i=0; i < count; i++)
        {
            state[i] = m_varlist[i]->GetValue();
        }
    }
    void ScriptContext::LoadState(const ValueType *state, size_t count)
    {
        count = std::min(count, m_varlist.size());

        for (size_t i=0; i < count; i++)
        {
            m_varlist[i]->SetValue(state[i]);
        }

    }

    
    void ScriptContext::ResetStats()
    {
//        for (auto wptr : m_codelist)
//        {
//            CompiledCodePtr ptr = wptr.lock();
//            if (ptr) {
//                ptr->ResetStats();
//            }
//        }
        
    }
    void ScriptContext::DumpStats(int verbose)
    {
    }
    
    void ScriptContext::DebugUI()
    {
//        if (!ImGui::CollapsingHeader(this->GetName().c_str()))
//            return;
        for (auto var : m_varlist)
        {
            if (var->IsReadByKernel || var->IsWrittenByKernel)
                ImGui::InputScalar(var->GetName().c_str(),  ImGuiDataType_Double, var->GetValuePtr()  );
        }
        
        
//        if (m_codelist.empty())
//            return;
//
//        if (!ImGui::CollapsingHeader(GetName().c_str()))
//            return;
//
//
//        if (ImGui::CollapsingHeader("Variables"))
//        {
//            for (auto var : m_list)
//            {
//                if (var->IsRead ||  var->IsWritten)
//                {
//                    ImGui::InputScalar(var->GetName().c_str(),  ImGuiDataType_Double, var->GetValuePtr());
//
//                    //ImGui::Text("var %s=%f read:%d written:%d\n", var->GetName().c_str(), var->GetValue(), var->IsRead, var->IsWritten);
//                }
//            }
//        }
        
        
//        for (auto wptr : m_codelist)
//        {
//            CompiledCodePtr ptr = wptr.lock();
//            if (ptr) {
//                ptr->DebugUI();
//            }
//        }
        
    }
    

}}
