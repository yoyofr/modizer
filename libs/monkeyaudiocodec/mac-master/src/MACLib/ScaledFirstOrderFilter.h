#ifndef APE_SCALEDFIRSTORDERFILTER_H
#define APE_SCALEDFIRSTORDERFILTER_H

template <int MULTIPLY, int SHIFT> class CScaledFirstOrderFilter
{
public:
    
    __forceinline void Flush()
    {
        m_nLastValue = 0;
    }

    __forceinline int Compress(const int nInput)
    {
        int nRetVal = nInput - ((m_nLastValue * MULTIPLY) >> SHIFT);
        m_nLastValue = nInput;
        return nRetVal;
    }

    __forceinline int Decompress(const int nInput)
    {
        m_nLastValue = nInput + ((m_nLastValue * MULTIPLY) >> SHIFT);
        return m_nLastValue;
    }

protected:
    
    int m_nLastValue;
};

#endif // #ifndef APE_SCALEDFIRSTORDERFILTER_H
