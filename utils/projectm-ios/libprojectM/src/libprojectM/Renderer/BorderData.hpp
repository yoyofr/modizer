#pragma once

#include <projectM-opengl.h>

#include <cstdint>
#include <cmath>

namespace libprojectM {
namespace Renderer {

/**
 * @brief A border with RGBA channels and a flag to detect borders when drawing a customshape.
 */
class BorderData
{
public:
    BorderData() = default;
    
    /**
     * Constructs a border with the given values.
     * @param r The border's r value.
     * @param g The border's g value.
     * @param b The border's b value.
     * @param a The border's a value.
     */
    BorderData(float r, float g, float b, float a,float isOnBorder,float zOrder,bool additive,bool texture)
    : m_rgb( ((int)((r<=1?r:1)*255.0)<<16)+((int)((g<=1?g:1)*255.0)<<8)+((int)((b<=1?b:1)*255.0)<<0) )
    , m_a( (int)((a<=1?a:1)*255.0) | (int)(additive?0x100:0) | (int)(texture?0x200:0) )
    , m_isOnBorder(isOnBorder)
    , m_thickness(zOrder){};
    
    /**
     * Returns the border's r value.
     * @return The border's r value.
     */
    auto R() const -> float
    {
        return (float)((((int)m_rgb)>>16)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's r value.
     * @param r The new r value.
     */
    void SetR(float r)
    {
        int i_rgb=(int)m_rgb;
        i_rgb = ((int)(r*255.0)<<16)+(i_rgb&0XFFFF);
        m_rgb=i_rgb;
    }
    
    /**
     * Returns the border's g value.
     * @return The border's g value.
     */
    auto G() const -> float
    {
        return (float)((((int)m_rgb)>>8)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's g value.
     * @param g The new g value.
     */
    void SetG(float g)
    {
        int i_rgb=(int)m_rgb;
        i_rgb = ((int)(g*255.0)<<8)+(i_rgb&0XFF00FF);
        m_rgb=i_rgb;
    }
    
    /**
     * Returns the border's b value.
     * @return The border's b value.
     */
    auto B() const -> float
    {
        return (float)((((int)m_rgb)>>0)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's b value.
     * @param b The new b value.
     */
    void SetB(float b)
    {
        int i_rgb=(int)m_rgb;
        i_rgb = ((int)(b*255.0)<<0)+(i_rgb&0XFFFF00);
        m_rgb=i_rgb;
    }
    
    /**
     * Returns the border's a value.
     * @return The border's a value.
     */
    auto A() const -> float
    {
        return (float)((((int)m_a)>>0)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's a value.
     * @param a The new a value.
     */
    void SetA(float a)
    {
        int i_a=(int)m_a;
        i_a = (int)(a*255.0)+(i_a&0XFFFF00);
        m_a=i_a;
    }
    
    /**
     * Returns the border's isOnBorder value
     * @return The border's isOnBorder value.
     */
    auto IsOnBorder() const -> float
    {
        return m_isOnBorder;
    }
    
    /**
     * Sets the border's isOnBorder value.
     * @param isOnBorder The new isOnBorder value.
     */
    void SetIsOnBoarder(float isOnBorder)
    {
        m_isOnBorder = isOnBorder;
    }
    
    /**
     * Returns the border's thickness value
     * @return The border's thickness value.
     */
    auto thickness() const -> float
    {
        return m_thickness;
    }
    
    /**
     * Sets the border's thickness value.
     * @param thickness The new thickness value.
     */
    void SetThickness(float thickness)
    {
        m_thickness = thickness;
    }
    
    /**
     * @brief Initializes the attribute array pointer for this storage type.
     * @param attributeIndex the attribute index to use.
     */
    static void InitializeAttributePointer(uint32_t attributeIndex)
    {
        glVertexAttribPointer(attributeIndex, sizeof(BorderData) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(BorderData), nullptr);
    }
    
private:
    float m_rgb{}; //!< The border's r&g value (stored as 0Xrrgg).
    float m_a{}; //!< The border's g&a value (store as 0xbbaa).
    float m_isOnBorder{}; //!< The border's isOnBorder flag.
    float m_thickness{}; //!< The border's thickness.
};

} // namespace Renderer
} // namespace libprojectM
