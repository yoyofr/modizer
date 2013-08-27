#ifndef st_FrameBuffer_h_
#define st_FrameBuffer_h_

@protocol EAGLDrawable;
@class EAGLContext;
@class UIImage;

struct FrameBuffer
{
	int m_width;
	int m_height;
    
    //Buffer definitions for the view.
    GLuint viewRenderbuffer,viewFramebuffer,depthBuffer;
    //Buffer definitions for the MSAA
    GLuint msaaFramebuffer,msaaRenderBuffer,msaaDepthBuffer;
    
	
};


namespace FrameBufferUtils
{
	void Create(FrameBuffer& buffer, EAGLContext* eaglContext, id<EAGLDrawable> drawable);
	void Destroy(FrameBuffer& buffer);
	void Set(const FrameBuffer& buffer);
    void SwapBuffer(const FrameBuffer& buffer,EAGLContext *eaglContext);
	
	UIImage* CreateImageFromFramebuffer(int width, int height);
}


#endif
