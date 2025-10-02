
#pragma once


#include <string>
#include <queue>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <functional>
#include "MDcommon.h"
#include "ImageReader.h"

namespace render {

struct Vertex
{
	float x, y, z;
    union {
        uint32_t Diffuse;     // diffuse color
        uint32_t rgba;
    };
	float tu, tv;           // DYNAMIC
	float rad, ang;         // STATIC
    
    void Clear()
    {
        x = y = z = 0;
        rgba = 0;
        tu = tv = 0;
        rad = ang = 0;
    }
};


enum class PixelFormat
{
    Invalid,
    A8Unorm,
    RGBA8Unorm,
    RGBA8Unorm_sRGB,
    BGRA8Unorm,
    BGRA8Unorm_sRGB,
    RGBA16Unorm,
    RGBA16Snorm,
    RGBA16Float,
    RGBA32Float,
    BGR10A2Unorm,
    BGR10_XR,           // ios only
    BGR10_XR_sRGB,      // ios only
};

const char *PixelFormatToString(PixelFormat format);
int PixelFormatGetPixelSize(PixelFormat format);
bool PixelFormatIsHDR(PixelFormat format);


enum PrimitiveType 
{
	PRIMTYPE_POINTLIST = 1,
	PRIMTYPE_LINELIST = 2,
	PRIMTYPE_LINESTRIP = 3,
	PRIMTYPE_TRIANGLELIST = 4,
	PRIMTYPE_TRIANGLESTRIP = 5,
	PRIMTYPE_TRIANGLEFAN = 6,
	PRIMTYPE_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
};

enum SamplerAddress
{
	SAMPLER_WRAP,
	SAMPLER_CLAMP
};

enum SamplerFilter
{
	SAMPLER_POINT,
	SAMPLER_LINEAR,
	SAMPLER_ANISOTROPIC
};

enum BlendFactor
{
	BLEND_ZERO = 1,
	BLEND_ONE = 2,
	BLEND_SRCCOLOR = 3,
	BLEND_INVSRCCOLOR = 4,
	BLEND_SRCALPHA = 5,
	BLEND_INVSRCALPHA = 6,
	BLEND_DESTALPHA = 7,
	BLEND_INVDESTALPHA = 8,
	BLEND_DESTCOLOR = 9,
	BLEND_INVDESTCOLOR = 10,
};

class Context;
class Texture;
class Shader;
class ShaderSampler;
class ShaderConstant;

using ContextPtr = std::shared_ptr<Context>;
using ShaderSamplerPtr = std::shared_ptr<ShaderSampler>;
using ShaderConstantPtr = std::shared_ptr<ShaderConstant>;
using ShaderPtr = std::shared_ptr<Shader>;
using TexturePtr = std::shared_ptr<Texture>;

//using IndexType = uint16_t;
using IndexType = uint32_t;




class ImageData
{
public:
    ImageData(int width, int height, PixelFormat format)
    : format(format),
        width(width), height(height),
        pitch(width * PixelFormatGetPixelSize(format)),
        size(pitch * height)
    {
        data = new uint8_t[size];
    }
    
    ~ImageData()
    {
        delete[] data;
    }
    
    
    
    PixelFormat   format;
    int           width;
    int           height;
    size_t        pitch;
    size_t        size;
    uint8_t *     data;
};

using ImageDataPtr = std::shared_ptr<ImageData>;

class Texture
{
protected:
	Texture()  = default;
    
public:
    virtual ~Texture() = default;

    virtual TexturePtr GetSharedPtr() = 0;
    virtual const std::string &GetName() = 0;
    virtual Size2D GetSize() = 0;    
    virtual PixelFormat GetFormat() = 0;

    

    int GetWidth() {
        return GetSize().width;
    }
    int GetHeight() {
        return GetSize().height;
    }
    
};


class DynamicBuffer
{
protected:
    DynamicBuffer() = default;
    virtual ~DynamicBuffer() = default;
public:

    virtual void WriteBytes(const void *data, size_t alignment, size_t length) = 0;
    virtual size_t GetBase() const = 0;
    
    template<typename T>
    void Write(const T *data, size_t count)
    {
      size_t length = count * sizeof(data[0]);
      WriteBytes(data, alignof(T), length);
    }
};


class ShaderSampler
{
protected:
	ShaderSampler() = default;
    virtual ~ShaderSampler() = default;
    
public:
	virtual const std::string &GetName() = 0;

    virtual void SetTexture(TexturePtr texture)
    {
        SetTexture(texture, SAMPLER_WRAP, SAMPLER_LINEAR);
    }

    virtual void SetTexture(TexturePtr texture, SamplerAddress address, SamplerFilter filter) = 0;
};


class ShaderConstant
{
protected:
	ShaderConstant() = default;
    virtual ~ShaderConstant() = default;

public:
	virtual const std::string &GetName() = 0;
    virtual void SetFloat(float f) = 0;
    virtual void SetVector(const Vector4 &v) = 0;
    virtual void SetMatrix(const Matrix44 &m) = 0;
};

enum class ShaderType
{
    Vertex,
    Fragment
};

struct ShaderSource
{
    bool IsVertex() const   {return type == ShaderType::Vertex;}
    bool IsFragment() const {return type == ShaderType::Fragment;}
    
    ShaderType  type;
    std::string path;
    std::string code;
    std::string functionName;
    std::string profile;
    std::string language;
};


class Shader
{
public:
	Shader()
    {
        InstanceCounter<Shader>::Increment();
    }
	virtual ~Shader()
    {
        InstanceCounter<Shader>::Decrement();
    }


    virtual bool CompileAndLink(const std::initializer_list<ShaderSource> &sourceList)
    {
        for (auto source : sourceList)
        {
            Compile(source);
        }
        return Link();
    }

protected:
    virtual bool Compile(const ShaderSource &source) = 0;
	virtual bool Link() = 0;
public:
    virtual std::string GetErrors() = 0;

    
	virtual int			GetSamplerCount() = 0;
	virtual ShaderSamplerPtr   GetSampler(int index) = 0;

	virtual int			GetConstantCount() = 0;
	virtual ShaderConstantPtr	GetConstant(int index) = 0;

	virtual ShaderConstantPtr	GetConstant(const char *name)
	{
		int count = GetConstantCount();
		for (int i = 0; i < count; i++)
		{
			ShaderConstantPtr constant = GetConstant(i);
			if (constant->GetName() == name) {
				return constant;
			}
		}
		return NULL;
	}
private:

};


struct DisplayInfo
{
    Size2D      size = Size2D(0, 0);
    PixelFormat format = render::PixelFormat::Invalid;
    float       refreshRate = 60.0f;
    float       scale = 1;
    int         samples = 1;
    float       maxEDR = 1.0f;
    bool        srgb = false;
    bool        hdr = false;
    std::string colorSpace;
};


enum class LoadAction
{
    Load,
    Clear,
    Discard
};

// rendering context
class Context
{
protected:
    Context()  = default;
public:
    virtual ~Context() = default;


    int     GetDisplayMultisampleCount() const           { return m_displayInfo.samples; }

    virtual float GetDisplayScale() const           { return m_displayInfo.scale; }
    virtual float GetDisplayRate() const            { return m_displayInfo.refreshRate; }
    virtual Size2D GetDisplaySize() const           { return m_displayInfo.size; }
    virtual PixelFormat GetDisplayFormat() const    { return m_displayInfo.format; }
    virtual float GetDisplayEDRMax() const          { return m_displayInfo.maxEDR;}
    
    virtual float GetTexelOffset() = 0;
	virtual const std::string &GetDriver() = 0;
	virtual const std::string &GetDesc() = 0;
    virtual const std::string &GetShadingLanguage() = 0;
    
    virtual void PushLabel(const char *name) {}
    virtual void PopLabel() {}
    
    virtual bool IsThreaded() { return false; }

	virtual TexturePtr CreateRenderTarget(const char *name, int width, int height, PixelFormat format) = 0;
	virtual TexturePtr CreateTexture(const char *name, int width, int height, PixelFormat format, const void *data) = 0;
	virtual ShaderPtr  CreateShader(const char *name) = 0;

    virtual void GetImageDataAsync(TexturePtr texture, std::function<void(ImageDataPtr)> callback) = 0;
   

    
	virtual void SetShader(ShaderPtr shader) = 0;

    virtual void SetDepthEnable(bool enable) = 0;
	virtual void SetBlend(BlendFactor sourceFactor, BlendFactor destFactor) = 0;
    virtual void SetBlendDisable() = 0;

    virtual void SetTransform(const Matrix44 &m) = 0;
    virtual const Matrix44 &GetTransform() const = 0;

    virtual void SetDisplayInfo(DisplayInfo info);
    virtual const DisplayInfo &GetDisplayInfo() {return m_displayInfo;}
    virtual void OnSetDisplayInfo() {}

	virtual void Present() = 0;
	virtual void BeginScene() = 0;
	virtual void EndScene() = 0;

    virtual void NextFrame() {}
    
    virtual void Flush() {}

    virtual Size2D GetRenderTargetSize() = 0;


    virtual void SetRenderTarget(TexturePtr texture, const char *passName = nullptr, LoadAction action = LoadAction::Load, Color4F clearColor = Color4F(0,0,0,0) ) = 0;


	virtual void SetPointSize(float size) = 0;
	//virtual void Draw(PrimitiveType primType, int count, const Vertex *v) = 0;
    
    virtual void SetScissorRect(int x, int y, int w, int h) = 0;
    virtual void SetScissorDisable() = 0;
    virtual void SetViewport(int x, int y, int w, int h) = 0;
    
    
    virtual void UploadIndexData(int indexCount, const IndexType *indices) = 0;
    virtual void UploadVertexData(size_t size, const Vertex *data) = 0;
    virtual void DrawArrays(PrimitiveType primType, int vertexStart, int vertexCount) = 0;
    virtual void DrawIndexed(PrimitiveType primType, int indexOffset, int indexCount) = 0;

    
    virtual bool UploadTextureData(TexturePtr texture, const void *data, int width, int height, int pitch) {return false;}

    
    virtual void DrawArrays(PrimitiveType primType, int vertexCount, const Vertex *v)
    {
        UploadVertexData(vertexCount, v);
        DrawArrays(primType, 0, vertexCount);
    }


    void DrawArrays(PrimitiveType primType, const std::vector<Vertex> &vertexData)
    {
       DrawArrays(primType, (int) vertexData.size(), vertexData.data() );
    }
    
    void UploadIndexData(const std::vector<IndexType> &indexData)
    {
        UploadIndexData((int)indexData.size(), indexData.data());
    }

    void UploadVertexData(const std::vector<Vertex> &vertexData)
    {
        UploadVertexData( (int) vertexData.size(), vertexData.data() );
    }
    
    virtual bool IsSupported(PixelFormat format) { return false; }

    TexturePtr CreateTextureFromFileInMemory(const char *name, const uint8_t *data, size_t size)
    {
        ImageDataPtr image = ImageLoadFromFileInMemory(data, size);
        if (!image) {
            return nullptr;
        }
        
        return CreateTexture(name, image->width, image->height, image->format, image->data);
    }

    TexturePtr CreateTextureFromFile(const char *name, const char *path)
    {
        ImageDataPtr image = ImageLoadFromFile(path);
        if (!image) {
            return nullptr;
        }
        
        return CreateTexture(name, image->width, image->height, image->format, image->data);
    }


protected:
    
    DisplayInfo             m_displayInfo;

};

} // render
 
