#pragma once

#include <assert.h>
#include <stdlib.h>
#include <memory>
//s#include <thread>
#include <atomic>
#include "matrix.h"



struct Color4F
{
    float r,g,b,a;
  
    inline Color4F()  {}

//    inline Color4F() :r(0), g(0), b(0), a(0) {}
    //inline Color4F(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
    inline Color4F(double r, double g, double b, double a) :
    r((float)r), g((float)g), b((float)b), a((float)a) {}
    
    
    #define F32_TO_INT8_SAT(_VAL)        (((uint32_t)((_VAL) * 255.0f))&0xFF)

    
    static inline uint32_t ToU32(float r, float g, float b, float a)
    {
        uint32_t o;
        o  = F32_TO_INT8_SAT(r) << 0;
        o |= F32_TO_INT8_SAT(g) << 8;
        o |= F32_TO_INT8_SAT(b) << 16;
        o |= F32_TO_INT8_SAT(a) << 24;
        return o;
    }
     
     inline uint32_t ToU32() const
     {
         return ToU32(r,g,b,a);
     }
};


struct Size2D
{
    int width;
    int height;
    
    inline Size2D() {}
    inline Size2D(int w, int h) : width(w), height(h) {}
};

template<typename T>
struct InstanceCounter
{
    static std::atomic<int> &GetInstanceCount()
    {
        static std::atomic<int> count;
        return count;
    }
    
    static void Increment()
    {
        GetInstanceCount()++;
    }
    
    
    static void Decrement()
    {
        GetInstanceCount()--;
    }
    
};


#define COLOR_ARGB(a,r,g,b) Color4F::ToU32(r,g,b,a)
#define COLOR_RGBA(r,g,b,a) Color4F::ToU32(r,g,b,a)

