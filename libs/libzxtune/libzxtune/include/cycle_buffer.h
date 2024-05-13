/**
*
* @file
*
* @brief  Simple cycle buffer template
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <cstring>
#ifdef EMSCRIPTEN
#include <algorithm>
#endif
//boost includes
#include <boost/noncopyable.hpp>

template<class T>
class CycleBuffer : public boost::noncopyable
{
public:
  explicit CycleBuffer(std::size_t size)
    : BufferStart(new T[size])
    , BufferEnd(BufferStart + size)
    , FreeCursor(BufferStart)
    , FreeAvailable(size)
    , BusyCursor(BufferStart)
    , BusyAvailable(0)
  {
  }

  ~CycleBuffer()
  {
    delete[] BufferStart;
  }

  void Reset()
  {
    FreeCursor = BufferStart;
    FreeAvailable = BufferEnd - BufferStart;
    BusyCursor = BufferStart;
    BusyAvailable = 0;
  }

  std::size_t Put(const T* src, std::size_t count)
  {
    const std::size_t toPut = std::min(count, FreeAvailable);
    const T* input = src;
    for (std::size_t rest = toPut; rest != 0; )
    {
      const std::size_t availFree = BusyCursor <= FreeCursor
        ? BufferEnd - FreeCursor
        : BusyCursor - FreeCursor;
      const std::size_t toCopy = std::min(availFree, rest);
      std::memcpy(FreeCursor, input, toCopy * sizeof(T));
      input += toCopy;
      rest -= toCopy;
      FreeCursor += toCopy;
      if (FreeCursor == BufferEnd)
      {
        FreeCursor = BufferStart;
      }
    }
    FreeAvailable -= toPut;
    BusyAvailable += toPut;
    return toPut;
  }

  std::size_t Peek(std::size_t count, const T*& part1Start, std::size_t& part1Size, const T*& part2Start, std::size_t& part2Size) const
  {
    if (const std::size_t toPeek = std::min(count, BusyAvailable))
    {
      part1Start = BusyCursor;
      part1Size = toPeek;
      part2Start = 0;
      part2Size = 0;
      if (BusyCursor >= FreeCursor)
      {
        //possible split
        const std::size_t availAtEnd = BufferEnd - BusyCursor;
        if (toPeek > availAtEnd)
        {
          part1Size = availAtEnd;
          part2Start = BufferStart;
          part2Size = toPeek - availAtEnd;
        }
      }
      return toPeek;
    }
    return 0;
  }

  std::size_t Consume(std::size_t count)
  {
    const std::size_t toConsume = std::min(count, BusyAvailable);
    BusyCursor += toConsume;
    while (BusyCursor >= BufferEnd)
    {
      BusyCursor = BufferStart + (BusyCursor - BufferEnd);
    }
    FreeAvailable += toConsume;
    BusyAvailable -= toConsume;
    return toConsume;
  }
private:
  T* const BufferStart;
  const T* const BufferEnd;
  T* FreeCursor;
  std::size_t FreeAvailable;
  const T* BusyCursor;
  std::size_t BusyAvailable;
};
