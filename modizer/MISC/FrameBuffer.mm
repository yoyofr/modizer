#include "FrameBuffer.h"
#include "GlErrors.h"

#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/glext.h>


void FrameBufferUtils::Create(FrameBuffer& buffer, int width, int height)
{
	buffer.m_width = width;
	buffer.m_height = height;
	
	glGenFramebuffersOES(1, &buffer.m_frameBufferHandle);
    glGenRenderbuffersOES(1, &buffer.m_colorBufferHandle);
	glGenRenderbuffersOES(1, &buffer.m_depthBufferHandle);
	CHECK_GL_ERRORS();
	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.m_colorBufferHandle);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_RGB5_A1_OES, buffer.m_width, buffer.m_height);
	CHECK_GL_ERRORS();
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.m_depthBufferHandle);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, buffer.m_width, buffer.m_height);
	CHECK_GL_ERRORS();
	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.m_colorBufferHandle);
	
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, buffer.m_frameBufferHandle);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, buffer.m_colorBufferHandle);  
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, buffer.m_depthBufferHandle);
	CHECK_GL_ERRORS();
}


void FrameBufferUtils::Create(FrameBuffer& buffer, EAGLContext* oglContext, id<EAGLDrawable> drawable)
{
	glGenFramebuffersOES(1, &buffer.m_frameBufferHandle);
    glGenRenderbuffersOES(1, &buffer.m_colorBufferHandle);
	glGenRenderbuffersOES(1, &buffer.m_depthBufferHandle);
	CHECK_GL_ERRORS();
	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.m_colorBufferHandle);
    [oglContext renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:drawable];
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &buffer.m_width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &buffer.m_height);
	CHECK_GL_ERRORS();
	//NSLog(@"Bind: %d %d %d", drawable, backingWidth, backingHeight);
	assert(buffer.m_width > 0 && buffer.m_height > 0);
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.m_depthBufferHandle);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, buffer.m_width, buffer.m_height);
	CHECK_GL_ERRORS();
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.m_colorBufferHandle);
	
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, buffer.m_frameBufferHandle);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, buffer.m_colorBufferHandle);  
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, buffer.m_depthBufferHandle);
	CHECK_GL_ERRORS();
	
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) 
	{
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		assert(0);
    }	
}


void FrameBufferUtils::Destroy(FrameBuffer& buffer)
{
	glDeleteFramebuffersOES(1, &buffer.m_frameBufferHandle);
	glDeleteRenderbuffersOES(1, &buffer.m_colorBufferHandle);
	glDeleteRenderbuffersOES(1, &buffer.m_depthBufferHandle);
}


void FrameBufferUtils::Set(const FrameBuffer& buffer)
{
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, buffer.m_frameBufferHandle);	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.m_colorBufferHandle);
	CHECK_GL_ERRORS();
	
    glViewport(0, 0, buffer.m_width, buffer.m_height);	
}



UIImage* FrameBufferUtils::CreateImageFromFramebuffer(int width, int height)
{
	const int renderTargetWidth = width;
	const int renderTargetHeight = height;
	const int renderTargetSize = renderTargetWidth*renderTargetHeight*4;
	
	uint8_t* imageBuffer = (uint8_t*)malloc(renderTargetSize);
	glReadPixels(0,0,renderTargetWidth,renderTargetHeight,GL_RGBA,GL_UNSIGNED_BYTE, imageBuffer);
	
	const int rowSize = renderTargetWidth*4;
	
	CGDataProviderRef ref = CGDataProviderCreateWithData(NULL, imageBuffer, renderTargetSize, NULL);
	CGImageRef iref = CGImageCreate(renderTargetWidth, renderTargetHeight, 8, 32, rowSize, 
									CGColorSpaceCreateDeviceRGB(), 
									kCGImageAlphaLast | kCGBitmapByteOrderDefault, ref, 
									NULL, true, kCGRenderingIntentDefault);
	
	uint8_t* contextBuffer = (uint8_t*)malloc(renderTargetSize);
	memset(contextBuffer, 0, renderTargetSize);
	CGContextRef context = CGBitmapContextCreate(contextBuffer, renderTargetWidth, renderTargetHeight, 8, rowSize, 
												 CGImageGetColorSpace(iref), 
												 kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Big);
	CGContextTranslateCTM(context, 0.0, renderTargetHeight);
	CGContextScaleCTM(context, 1.0, -1.0);
	CGContextDrawImage(context, CGRectMake(0.0, 0.0, renderTargetWidth, renderTargetHeight), iref);	
	CGImageRef outputRef = CGBitmapContextCreateImage(context);
	
	UIImage* image = [[UIImage alloc] initWithCGImage:outputRef];
	
	CGImageRelease(outputRef);
	CGContextRelease(context);
	CGImageRelease(iref);
	CGDataProviderRelease(ref);
	
	free(contextBuffer);
	free(imageBuffer);
	return image;
}

