#ifndef st_datastructures_Queue_h
#define st_datastructures_Queue_h


namespace st
{

template< typename T >
class Queue 
{
public:
	void PushBack(const T& element);
	void PopFront();

	int GetCount() const;
	int GetCapacity() const;

	void Reset();
	int GetIndexOf(const T& element) const;

	const T& operator[](int index) const;
	T& operator[](int index);

protected:
	Queue(int capacity, T* storage);
	~Queue();

private:
	const int m_capacity;
	int m_count;
	int m_headIndex;
	int m_tailIndex;
	T* m_storage;
};


template< typename T >
inline Queue< T >::Queue(const int capacity, T* storage)
	: m_capacity(capacity)
	, m_count(0)
	, m_headIndex(0)
	, m_tailIndex(0)
	, m_storage(storage)
{
}

template< typename T >
inline Queue< T >::~Queue()
{
}


template< typename T >
inline void Queue< T >::PushBack(const T& element)
{
	//ST_ASSERT_MSG(m_count < m_capacity, "m_count(%d) < m_capacity(%d)", m_count, m_capacity);
	m_storage[m_tailIndex] = element;
	++m_count;
	m_tailIndex = (m_tailIndex+1) % m_capacity;
}


template< typename T >
inline void Queue< T >::PopFront()
{
	//ST_ASSERT(m_count > 0);
	--m_count;
	m_headIndex = (m_headIndex+1) % m_capacity;
}


template< typename T >
inline int Queue< T >::GetCapacity() const
{
	return m_capacity;
}

template< typename T >
inline int Queue< T >::GetCount() const
{
	return m_count;
}


template< typename T >
inline void Queue< T >::Reset()
{
	m_count = 0;
	m_headIndex = 0;
	m_tailIndex = 0;
}


template< typename T >
inline int Queue< T >::GetIndexOf(const T& element) const
{
	const int count = m_count;
	for (int i = 0; i < count; ++i)
	{
		if (element == (*this)[i])
			return i;
	}

	return -1;
}


template< typename T >
inline const T& Queue< T >::operator[](const int index) const
{
	//ST_ASSERT(index >= 0 && index < m_count);
	const int elementIndex = (m_headIndex + index) % m_capacity;
	return m_storage[elementIndex];
}

template< typename T >
inline T& Queue< T >::operator[](const int index)
{
	//ST_ASSERT(index >= 0 && index < m_count);	
	const int elementIndex = (m_headIndex + index) % m_capacity;
	return m_storage[elementIndex];
}



//////////


template< typename T, int Capacity >
class QueueStorage : public Queue< T >
{
public:
	QueueStorage();

private:
	T m_localStorage[Capacity];
};


//lint -e{1401}
template< typename T, int Capacity >
QueueStorage< T, Capacity >::QueueStorage()
	: Queue<T>(Capacity, m_localStorage)
{
}


}

#endif
