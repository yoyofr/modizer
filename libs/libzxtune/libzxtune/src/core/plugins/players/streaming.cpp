/**
* 
* @file
*
* @brief  Streamed modules support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "streaming.h"
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
  const uint_t STREAM_CHANNELS = 1;

  class StreamStateCursor : public TrackState
  {
  public:
    typedef boost::shared_ptr<StreamStateCursor> Ptr;

    explicit StreamStateCursor(Information::Ptr info)
      : Info(info)
      , CurFrame()
    {
      Reset();
    }

    //status functions
    virtual uint_t Position() const
    {
      return 0;
    }

    virtual uint_t Pattern() const
    {
      return 0;
    }

    virtual uint_t PatternSize() const
    {
      return 0;
    }

    virtual uint_t Line() const
    {
      return 0;
    }

    virtual uint_t Tempo() const
    {
      return 1;
    }

    virtual uint_t Quirk() const
    {
      return 0;
    }

    virtual uint_t Frame() const
    {
      return CurFrame;
    }

    virtual uint_t Channels() const
    {
      return STREAM_CHANNELS;
    }

    //navigation
    bool IsValid() const
    {
      return CurFrame < Info->FramesCount();
    }

    void Reset()
    {
      CurFrame = 0;
    }

    void ResetToLoop()
    {
      CurFrame = Info->LoopFrame();
    }

    void NextFrame()
    {
      if (IsValid())
      {
        ++CurFrame;
      }
    }
  private:
    const Information::Ptr Info;
    uint_t CurFrame;
  };

  class StreamInfo : public Information
  {
  public:
    StreamInfo(uint_t frames, uint_t loopFrame)
      : TotalFrames(frames)
      , Loop(loopFrame)
    {
    }
    virtual uint_t PositionsCount() const
    {
      return 1;
    }
    virtual uint_t LoopPosition() const
    {
      return 0;
    }
    virtual uint_t PatternsCount() const
    {
      return 0;
    }
    virtual uint_t FramesCount() const
    {
      return TotalFrames;
    }
    virtual uint_t LoopFrame() const
    {
      return Loop;
    }
    virtual uint_t ChannelsCount() const
    {
      return STREAM_CHANNELS;
    }
    virtual uint_t Tempo() const
    {
      return 1;
    }
  private:
    const uint_t TotalFrames;
    const uint_t Loop;
  };

  class StreamStateIterator : public StateIterator
  {
  public:
    explicit StreamStateIterator(Information::Ptr info)
      : Cursor(boost::make_shared<StreamStateCursor>(info))
    {
    }

    //iterator functions
    virtual void Reset()
    {
      Cursor->Reset();
    }

    virtual bool IsValid() const
    {
      return Cursor->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      Cursor->NextFrame();
      if (!Cursor->IsValid() && looped)
      {
        Cursor->ResetToLoop();
      }
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return Cursor;
    }
  private:
    const StreamStateCursor::Ptr Cursor;
  };

  Information::Ptr CreateStreamInfo(uint_t frames, uint_t loopFrame)
  {
    return boost::make_shared<StreamInfo>(frames, loopFrame);
  }

  StateIterator::Ptr CreateStreamStateIterator(Information::Ptr info)
  {
    return boost::make_shared<StreamStateIterator>(info);
  }
}
