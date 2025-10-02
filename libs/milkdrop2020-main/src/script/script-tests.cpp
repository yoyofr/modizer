

#if 0
//#include "ns-eel2/ns-eel.h"

#include "script.h"
/*

SCENARIO("Scripting Tests")
{
    {
        using namespace Script;
        
        
        Var time = 0.0;
        
        IContextPtr  context =  mdpx::CreateContext("test-context");
        
        context->RegVar("time", time);
        
        
        IKernelPtr kernel = context->Compile("test-kernel",
                         "time=time+1;");
        
        printf("time %f\n", time);
        kernel->Execute();
        printf("time %f\n", time);
        
        kernel->DumpStats();

    }
    
    
}
*/

SCENARIO("ns-eel2 Tests")
{
    using namespace Script;

    {
        NSEEL_init();
        
        //Var time = 0.0;
        
        NSEEL_VMCTX context = NSEEL_VM_alloc(); // return a handle
        IContextPtr  context2 =  mdpx::CreateContext("test-context");

        
        EEL_F *t1 = NSEEL_VM_regvar(context, "t1"); // register a variable (before compilation)
        EEL_F *t2 = NSEEL_VM_regvar(context, "t2");
        EEL_F *t3 = NSEEL_VM_regvar(context, "t3");

        context2->RegVar("t1", *t1);
        context2->RegVar("t2", *t2);
        context2->RegVar("t3", *t3);

        
//        std::string code = "time=time+1;";
        std::string code = "t3=(t1 / t2) + 1;";

        
        
        IKernelPtr kernel = context2->Compile("test-kernel", code);
        NSEEL_CODEHANDLE ch =  NSEEL_code_compile(context, (char *)code.c_str());
        
        *t1 = -1.0;
        *t2 = 0.0;
        *t3 = 0.0;

        NSEEL_code_execute(ch);
        printf("t3 %f\n", *t3);

        
        *t1 = -1.0;
        *t2 = 0.0;
        *t3 = 0.0;
        kernel->Execute();
        printf("t3 %f\n", *t3);
        
        
        NSEEL_code_free(ch);
        NSEEL_VM_free(context);

        




//
//        kernel->DumpStats();
        
        
    }
    
    
}

#endif
