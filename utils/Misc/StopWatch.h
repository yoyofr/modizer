

#pragma once

#include <chrono>

class StopWatch
{
public:
    using Clock = std::chrono::high_resolution_clock;
    
    
    StopWatch(bool running = true)
    :
        mIsRunning(running),
        mElapsed(0),
        mLastTicks(Clock::now())
    {
    }
    
    static StopWatch StartNew()
    {
        return StopWatch(true);
    }
    
    bool IsRunning()
    {
        return mIsRunning;
    }
    
    void Start()
    {
        if (!mIsRunning)
        {
            mLastTicks = Clock::now();
            mIsRunning = true;
        }
    }
    
    void  Stop()
    {
        if (mIsRunning)
        {
            // update elapsed time
            Clock::time_point time = Clock::now();
            mElapsed += (time - mLastTicks);
            mLastTicks = time;
            mIsRunning = false;
        }
    }
    
    void Reset()
    {
        mElapsed = Clock::duration::zero();
        mLastTicks = Clock::now();
    }
    
    void Restart()
    {
        Reset();
        Start();
    }
    
    Clock::duration      GetElapsed() const
    {
        if (mIsRunning)
        {
            // get time
            Clock::time_point time = Clock::now();
            
            // update elapsed time
            return mElapsed + (time - mLastTicks);
        }
        else
        {
            // not running
            return mElapsed;
        }
    }
    
    float      GetElapsedSeconds() const
    {
        return std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1> >>(GetElapsed()).count();
    }
    
    float      GetElapsedMilliSeconds() const
    {
        return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(GetElapsed()).count();
    }
    
    float      GetElapsedMicroSeconds() const
    {
        return std::chrono::duration_cast<std::chrono::duration<float, std::micro> >(GetElapsed()).count();
    }
    
    
private:
    bool                mIsRunning;
    Clock::duration     mElapsed;
    Clock::time_point   mLastTicks;
    
};


