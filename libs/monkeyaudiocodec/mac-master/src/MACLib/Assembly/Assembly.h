#ifndef APE_ASSEMBLY_H
#define APE_ASSEMBLY_H

extern "C" 
{
    typedef int (DotProductFunc) (const short * pA, const short * pB, int nOrder);
    typedef void (AdaptFunc) (short * pM, const short * pAdapt, int nDirection, int nOrder);

    typedef DotProductFunc *DotProductFuncPtr;
    typedef AdaptFunc *AdaptFuncPtr;

    extern DotProductFuncPtr CalculateDotProduct;
    extern AdaptFuncPtr Adapt;

    int asmInit();

}

#endif // #ifndef APE_ASSEMBLY_H

