

#pragma once

#include <stdint.h>
#include <math.h>
#include "MDcommon.h"


struct Sample16
{
    int16_t ch[2];
};



struct Sample
{
    float ch[2];
    
    float left() const  {return ch[0];}
    float right() const {return ch[1];}
    
    float volume() const {return ch[0] + ch[1];}
    
    static inline float ConvertS16ToFloat(const int16_t s, float scale = 1.0f)
    {
        float v = ((float)s) * scale / 32768.0f;
        if (v >  1.0f) v =  1.0f;
        if (v < -1.0f) v = -1.0f;
        return v;
    }
    
    
    
    static inline Sample Convert(const float &s, float scale = 1.0f)
    {
        float ch0 = s * scale;
        return Sample {ch0, ch0};
    }
    
    
    static inline Sample Convert(const int16_t &s, float scale = 1.0f)
    {
        float ch0 = ConvertS16ToFloat(s, scale);
        return Sample {ch0, ch0};
    }
    
    static inline Sample Convert(const Sample16 &s, float scale = 1.0f)
    {
        float ch0 = ConvertS16ToFloat(s.ch[0], scale);
        float ch1 = ConvertS16ToFloat(s.ch[1], scale);
        return Sample {ch0, ch1};
    }
    
    
    Sample()
    : ch{0.0f, 0.0f}
    {
    }
    
    
    Sample(float ch0, float ch1)
    : ch{ch0, ch1}
    {
    }
    
    
    Sample &operator *=(float factor)
    {
        ch[0] *= factor;
        ch[1] *= factor;
        return *this;
    }
    
    Sample &operator +=(const Sample &rhs)
    {
        ch[0] += rhs.ch[0];
        ch[1] += rhs.ch[1];
        return *this;
    }
    
    Sample &operator -=(const Sample &rhs)
    {
        ch[0] -= rhs.ch[0];
        ch[1] -= rhs.ch[1];
        return *this;
    }
    
    Sample &operator *=(const Sample &rhs)
    {
        ch[0] *= rhs.ch[0];
        ch[1] *= rhs.ch[1];
        return *this;
    }
    
};



static inline Sample operator *(const Sample &s, float factor)
{
    return Sample { s.ch[0] * factor, s.ch[1] * factor};
}

static inline Sample operator /(const Sample &s, float factor)
{
    return Sample { s.ch[0] / factor, s.ch[1] / factor};
}


static inline Sample operator +(const Sample &a, const Sample &b)
{
    return Sample { a.ch[0] + b.ch[0], a.ch[1] + b.ch[1]};
}

static inline Sample operator -(const Sample &a, const Sample &b)
{
    return Sample { a.ch[0] - b.ch[0], a.ch[1] - b.ch[1]};
}






