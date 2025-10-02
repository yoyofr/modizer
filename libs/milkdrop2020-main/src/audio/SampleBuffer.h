

#pragma once

#include <stdint.h>
#include <math.h>
#include <vector>
#include "MDcommon.h"
#include "Sample.h"

template<typename T>
class SampleBuffer
{
public:
    
    
    T Get(int index) const
    {
        if (index < 0)
        {
            if (m_samples.empty()) return T();
            return m_samples.front();
        }
        if (index >= m_samples.size())
        {
            if (m_samples.empty()) return T();
            return m_samples.back();
        }
        
        return m_samples[index];
    }
    
    T operator[](int index) const
    {
        return Get(index);
    }
    
    
    T GetSample(float fpos) const
    {
        float fpos0 = truncf(fpos);
        float frac = fpos - fpos0;
        int p0 = (int)fpos0;
        int p1 = p0 + 1;
        
        float f0 = 1.0f - frac;
        float f1 = frac;
        
        T sample = Get(p0) * f0 + Get(p1) * f1;
        return sample;
    }
    
    void push_back(T value)
    {
        m_samples.push_back(value);
    }
    
    const std::vector<T> &GetVector() const
    {
        return m_samples;
    }
    
    void Assign(const std::vector<T> &v)
    {
        m_samples = v;
    }
    
    using Titerator = typename std::vector<T>::iterator;
    
    void Assign(Titerator start,Titerator end)
    {
        size_t total = end - start;
        m_samples.clear();
        m_samples.reserve(total);
        
        m_samples.assign(start, end);
    }
    
    void clear()
    {
        m_samples.clear();
    }
    
    
    void reserve(size_t capacity)
    {
        m_samples.reserve(capacity);
    }
    
    size_t size() const
    {
        return m_samples.size();
    }
    
    void resize(size_t size)
    {
        m_samples.resize(size);
    }
    
    const T * data() const
    {
        return m_samples.data();
    }
    
    T * data()
    {
        return m_samples.data();
    }
    
    template<typename TFactor>
    void Modulate(TFactor factor)
    {
        for (auto &it : m_samples)
        {
            it *= factor;
        }
    }
    
    void ApplyKernel(SampleBuffer<T> &outBuffer, float k0, float k1, float k2, float k3) const
    {
        if (m_samples.empty())
            return;
        
        outBuffer.reserve(m_samples.size());
        
        T s0 = m_samples.front();
        T s1 = s0;
        T s2 = s1;
        T s3 = s2;
        
        for (const auto &it : m_samples)
        {
            s3 = s2;
            s2 = s1;
            s1 = s0;
            s0 = it;
            
            T o = (s0 * k0) + (s1 * k1) + (s2 * k2) + (s3 * k3);
            outBuffer.push_back(o);
        }
    }
    
    
    void ApplyKernel(float k0, float k1, float k2, float k3)
    {
        if (m_samples.empty())
            return;
        
        T s0 = m_samples.front();
        T s1 = s0;
        T s2 = s1;
        T s3 = s2;
        
        for (auto &it : m_samples)
        {
            s3 = s2;
            s2 = s1;
            s1 = s0;
            s0 = it;
            
            T out = (s0 * k0) + (s1 * k1) + (s2 * k2) + (s3 * k3);
            it = out;
        }
    }
    
    
    void Smooth(float factor)
    {
        float f0 = 1.0f - factor;
        float f1 = f0 * factor;
        float f2 = f1 * factor;
        float f3 = f2 * factor;
        ApplyKernel(f0, f1, f2, f3);
    }
    
    
    void Smooth(SampleBuffer<T> &out_data, float factor)
    {
        float f0 = 1.0f - factor;
        float f1 = f0 * factor;
        float f2 = f1 * factor;
        float f3 = f2 * factor;
        ApplyKernel(out_data, f0, f1, f2, f3);
    }
    
    float GetSampleRate() const {return m_sampleRate;}
    void SetSampleRate(float rate) {m_sampleRate = rate;}

private:
    float m_sampleRate = 0;
    std::vector<T> m_samples;
};






