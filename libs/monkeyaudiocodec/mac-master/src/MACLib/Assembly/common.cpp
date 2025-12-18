#include "All.h"
#include "Assembly.h"

extern "C"
{

    DotProductFuncPtr CalculateDotProduct;
    AdaptFuncPtr Adapt;

    DotProductFunc CalculateDotProduct_c;
    AdaptFunc Adapt_c;

    DotProductFunc CalculateDotProduct_x86_mmx;
    AdaptFunc Adapt_x86_mmx;

    DotProductFunc CalculateDotProduct_x86_64_mmx;
    AdaptFunc Adapt_x86_64_mmx;

    int asmInit() {
        Adapt = Adapt_c;
        CalculateDotProduct = CalculateDotProduct_c;

#ifdef ENABLE_ASSEMBLY

#ifdef ARCH_X86
        Adapt = Adapt_x86_mmx;
        CalculateDotProduct = CalculateDotProduct_x86_mmx;
#endif

#ifdef ARCH_X86_64
        Adapt = Adapt_x86_64_mmx;
        CalculateDotProduct = CalculateDotProduct_x86_64_mmx;
#endif

#endif
        return 0;
    }

void Adapt_c(short * pM, const short * pAdapt, int nDirection, int nOrder)
{
    nOrder >>= 4;

    if (nDirection < 0) 
    {    
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ += *pAdapt++;)
        }
    }
    else if (nDirection > 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ -= *pAdapt++;)
        }
    }
}

int CalculateDotProduct_c(const short * pA, const short * pB, int nOrder)
{
    int nDotProduct = 0;
    nOrder >>= 4;

    while (nOrder--)
    {
        EXPAND_16_TIMES(nDotProduct += *pA++ * *pB++;)
    }
    
    return nDotProduct;
}

}
