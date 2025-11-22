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
    BorderData(float r, float g, float b, float a,float isOnBorder,float thickness)
    : m_rg( ((int)(r*255.0)<<8)+(int)(g*255.0) )
    , m_ba( ((int)(b*255.0)<<8)+(int)(a*255.0) )
    , m_isOnBorder(isOnBorder)
    , m_thickness(thickness){};
    
    /**
     * Returns the border's r value.
     * @return The border's r value.
     */
    auto R() const -> float
    {
        return (float)((((int)m_rg)>>8)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's r value.
     * @param r The new r value.
     */
    void SetR(float r)
    {
        int i_rg=(int)m_rg;
        i_rg = ((int)(r*255.0)<<8)+(i_rg&0XFF);
        m_rg=i_rg;
    }
    
    /**
     * Returns the border's g value.
     * @return The border's g value.
     */
    auto G() const -> float
    {
        return (float)((((int)m_rg)>>0)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's g value.
     * @param g The new g value.
     */
    void SetG(float g)
    {
        int i_rg=(int)m_rg;
        i_rg = (int)(g*255.0)+(i_rg&0XFF00);
        m_rg=i_rg;
    }
    
    /**
     * Returns the border's b value.
     * @return The border's b value.
     */
    auto B() const -> float
    {
        return (float)((((int)m_ba)>>8)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's b value.
     * @param b The new b value.
     */
    void SetB(float b)
    {
        int i_ba=(int)m_ba;
        i_ba = ((int)(b*255.0)<<8)+(i_ba&0XFF);
        m_ba=i_ba;
    }
    
    /**
     * Returns the border's a value.
     * @return The border's a value.
     */
    auto A() const -> float
    {
        return (float)((((int)m_ba)>>0)&0xFF)/255.0;
    }
    
    /**
     * Sets the border's a value.
     * @param a The new a value.
     */
    void SetA(float a)
    {
        int i_ba=(int)m_ba;
        i_ba = (int)(a*255.0)+(i_ba&0XFF00);
        m_ba=i_ba;
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
    float m_rg{}; //!< The border's r&g value (stored as 0Xrrgg).
    float m_ba{}; //!< The border's g&a value (store as 0xbbaa).
    float m_isOnBorder{}; //!< The border's isOnBorder flag.
    float m_thickness{}; //!< The border's thickness.
};

} // namespace Renderer
} // namespace libprojectM
