#include "FrameBuffer.h"
#include "GlErrors.h"

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/glext.h>

#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

static int msaa=1;

void FrameBufferUtils::Create(FrameBuffer& buffer, EAGLContext* oglContext, id<EAGLDrawable> drawable)
{
    //Create our viewFrame and render Buffers.
    glGenFramebuffersOES(1, &buffer.viewFramebuffer);
    glGenRenderbuffersOES(1, &buffer.viewRenderbuffer);
    
    //Bind the buffers.
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, buffer.viewFramebuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.viewRenderbuffer);
    [oglContext renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:drawable];
    
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, buffer.viewRenderbuffer);
    
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &buffer.m_width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &buffer.m_height);
    
    glGenRenderbuffersOES(1, &buffer.depthBuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.depthBuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, buffer.m_width ,buffer.m_height);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, buffer.depthBuffer);
    
    buffer.msaaFramebuffer=0;
        //Generate our MSAA Frame and Render buffers
        glGenFramebuffersOES(1, &buffer.msaaFramebuffer);
        glGenRenderbuffersOES(1, &buffer.msaaRenderBuffer);
    
        //Bind our MSAA buffers
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, buffer.msaaFramebuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.msaaRenderBuffer);
    
        // Generate the msaaDepthBuffer.
        // 4 will be the number of pixels that the MSAA buffer will use in order to make one pixel on the render buffer.
        glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER_OES, 4, GL_RGB5_A1_OES, buffer.m_width,buffer.m_height);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, buffer.msaaRenderBuffer);
        glGenRenderbuffersOES(1, &buffer.msaaDepthBuffer);
    
        //Bind the msaa depth buffer.
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.msaaDepthBuffer);
        glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER_OES, 4, GL_DEPTH_COMPONENT16_OES, buffer.m_width ,buffer.m_height);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, buffer.msaaDepthBuffer);
    
}


void FrameBufferUtils::Destroy(FrameBuffer& buffer)
{
	glDeleteFramebuffersOES(1, &buffer.viewFramebuffer);
	glDeleteRenderbuffersOES(1, &buffer.viewRenderbuffer);
    glDeleteRenderbuffersOES(1, &buffer.depthBuffer);
    if (buffer.msaaFramebuffer) {
        glDeleteFramebuffersOES(1, &buffer.msaaFramebuffer);
        glDeleteRenderbuffersOES(1, &buffer.msaaRenderBuffer);
        glDeleteRenderbuffersOES(1, &buffer.msaaDepthBuffer);
    }
}


void FrameBufferUtils::Set(const FrameBuffer& buffer)
{
    if ((settings[GLOB_FXMSAA].detail.mdz_boolswitch.switch_value)&&(buffer.msaaFramebuffer)) {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, buffer.msaaFramebuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.msaaRenderBuffer);
	} else {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, buffer.viewFramebuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.viewRenderbuffer);
    }
    CHECK_GL_ERRORS();
    glViewport(0, 0, buffer.m_width, buffer.m_height);	
}

void FrameBufferUtils::SwapBuffer(const FrameBuffer& buffer,EAGLContext *eaglContext) {
    if ((settings[GLOB_FXMSAA].detail.mdz_boolswitch.switch_value)&&(buffer.msaaFramebuffer)) {
        GLenum attachments[] = {GL_DEPTH_ATTACHMENT_OES};
        glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE, 1, attachments);
        //Bind both MSAA and View FrameBuffers.
        glBindFramebufferOES(GL_READ_FRAMEBUFFER_APPLE, buffer.msaaFramebuffer);
        glBindFramebufferOES(GL_DRAW_FRAMEBUFFER_APPLE, buffer.viewFramebuffer);
        // Call a resolve to combine both buffers
        glResolveMultisampleFramebufferAPPLE();
        // Present final image to screen
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, buffer.viewRenderbuffer);
    }
    [eaglContext presentRenderbuffer:GL_RENDERBUFFER_OES];
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

