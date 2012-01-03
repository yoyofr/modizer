// Simple smart pointer class.

#ifndef SIDPLAY1_SMART_H
#define SIDPLAY1_SMART_H

typedef unsigned long int ulint;

template <class T>
class smartPtrBase
{
 public:  // ----------------------------------------------------------------
    
    // --- constructors ---
    
    smartPtrBase(T* buffer, ulint bufferLen, bool bufOwner = false) : dummy(0)
    {
        doFree = bufOwner;
        if ( bufferLen >= 1 )
        {
            pBufCurrent = ( bufBegin = buffer );
            bufEnd = bufBegin + bufferLen;
            bufLen = bufferLen;
            status = true;
        }
        else
        {
            pBufCurrent = ( bufBegin = ( bufEnd = 0 ));
            bufLen = 0;
            status = false;
        }
    }
    
    // --- destructor ---
    
    virtual ~smartPtrBase()
    {
        if ( doFree && (bufBegin != 0) )
        {
#if defined(_MSC_VER)
            delete[] (void*)bufBegin;
#else
            delete[] bufBegin;
#endif
        }
    }
    
    // --- public member functions ---
    
    virtual T* tellBegin()  { return bufBegin; }
    virtual ulint tellLength()  { return bufLen; }
    virtual ulint tellPos()  { return (ulint)(pBufCurrent-bufBegin); }

    virtual bool checkIndex(ulint index)
    {
        return ((pBufCurrent+index)<bufEnd);
    }
    
    virtual bool reset()
    {
        if ( bufLen >= 1 )
        {
            pBufCurrent = bufBegin;
            return (status = true);
        }
        else
        {
            return (status = false);
        }
    }

    virtual bool good()
    {
        return (pBufCurrent<bufEnd);
    }
    
    virtual bool fail()  
    {
        return (pBufCurrent==bufEnd);
    }
    
    virtual void operator ++()
    {
        if ( good() )
        {
            pBufCurrent++;
        }
        else
        {
            status = false;
        }
    }
    
    virtual void operator ++(int)
    {
        if ( good() )
        {
            pBufCurrent++;
        }
        else
        {
            status = false;
        }
    }
    
    virtual void operator --()
    {
        if ( !fail() )
        {
            pBufCurrent--;
        }
        else
        {
            status = false;
        }
    }
    
    virtual void operator --(int)
    {
        if ( !fail() )
        {
            pBufCurrent--;
        }
        else
        {
            status = false;
        }
    }
    
    virtual void operator +=(ulint offset)
    {
        if (checkIndex(offset))
        {
            pBufCurrent += offset;
        }
        else
        {
            status = false;
        }
    }
    
    virtual void operator -=(ulint offset)
    {
        if ((pBufCurrent-offset) >= bufBegin)
        {
            pBufCurrent -= offset;
        }
        else
        {
            status = false;
        }
    }
    
    T operator*()
    {
        if ( good() )
        {
            return *pBufCurrent;
        }
        else
        {
            status = false;
            return dummy;
        }
    }

    T& operator [](ulint index)
    {
        if (checkIndex(index))
        {
            return pBufCurrent[index];
        }
        else
        {
            status = false;
            return dummy;
        }
    }

    virtual operator bool()  { return status; }
    
 protected:  // -------------------------------------------------------------
    
    T* bufBegin;
    T* bufEnd;
    T* pBufCurrent;
    ulint bufLen;
    bool status;
    bool doFree;
    T dummy;
};


template <class T>
class smartPtr : public smartPtrBase<T>
{
 public:  // ----------------------------------------------------------------
    
    // --- constructors ---
    
    smartPtr(T* buffer, ulint bufferLen, bool bufOwner = false)
        : smartPtrBase<T>(buffer, bufferLen, bufOwner)
    {
    }
    
    smartPtr()
        : smartPtrBase<T>(0,0)
    {
    }

    void setBuffer(T* buffer, ulint bufferLen)
    {
        if ( bufferLen >= 1 )
        {
            this->pBufCurrent = ( this->bufBegin = buffer );
            this->bufEnd = this->bufBegin + bufferLen;
            this->bufLen = bufferLen;
            this->status = true;
        }
        else
        {
            this->pBufCurrent = this->bufBegin = this->bufEnd = 0;
            this->bufLen = 0;
            this->status = false;
        }
    }
};

#endif  /* SIDPLAY1_SMART_H */
