
#pragma once

#include <assert.h>
#include <stdlib.h>

class RefCounted
{
public:
	int	 AddRef() const
	{
		return ++mRefCount;
	}

	int	Release() const
	{
		assert(mRefCount > 0);        
        int count = --mRefCount;
		if (count == 0)
		{
			delete this;
		}
		return count;
	}

protected:
	inline RefCounted()
		: mRefCount(0)
	{

	}

	virtual ~RefCounted()
	{
		// we tried to destroy an object that has active references
		assert(mRefCount == 0);
	}

private:
	mutable int			mRefCount;
};


//
//
//

template <class T>
class AutoRef
{
public:
	inline AutoRef()
		: mPtr(NULL)
	{
	}

	inline AutoRef(T *ptr)
		: mPtr(ptr)
	{
		if (mPtr != NULL)
			mPtr->AddRef();
	}

	// copy constructor
	inline 	AutoRef(const AutoRef<T> &other)
		: mPtr(other.mPtr)
	{
		if (mPtr != NULL)
			mPtr->AddRef();
	}

	inline ~AutoRef()
	{
		Clear();
	}

	// assignment operator
	inline void operator= (T *ptr)
	{
		SetPtr(ptr);
	}

	// assignment operator
	inline void operator= (const AutoRef &other)
	{
		SetPtr(other.mPtr);
	}

	inline void Clear()
	{
		if (mPtr)
			mPtr->Release();
		mPtr = NULL;
	}

	void SetPtr(T *ptr)
	{
		if (mPtr != ptr)
		{
			if (mPtr != NULL)
				mPtr->Release();
			mPtr = ptr;
			if (mPtr != NULL)
				mPtr->AddRef();
		}
	}

	inline T* Get() const
	{
		return mPtr;
	}

	inline T** GetAddressOf()
	{
		Clear();
		return &mPtr;
	}

	// no auto decrement
	T * Detach()
	{
		T * ptr = mPtr;
		mPtr = NULL;
		return ptr;
	}

	inline operator T *() const
	{
		return mPtr;
	}

	inline T * operator ->()
	{
		return mPtr;
	}
	
	inline T & operator *()
	{
		return *mPtr;
	}

private:
	T *			mPtr;
};


