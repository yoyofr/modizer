#ifndef st_FrameBuffer_h_
#define st_FrameBuffer_h_

@protocol EAGLDrawable;
@class EAGLContext;
@class UIImage;

struct FrameBuffer
{
	uint m_frameBufferHandle;
	uint m_colorBufferHandle;
	uint m_depthBufferHandle;
	int m_width;
	int m_height;
};


namespace FrameBufferUtils
{
	void Create(FrameBuffer& buffer, int width, int height);
	void Create(FrameBuffer& buffer, EAGLContext* eaglContext, id<EAGLDrawable> drawable);
	void Destroy(FrameBuffer& buffer);
	void Set(const FrameBuffer& buffer);
	
	UIImage* CreateImageFromFramebuffer(int width, int height);
}


#endif
