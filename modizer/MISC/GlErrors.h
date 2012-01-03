#ifndef st_GlErrors_h
#define st_GlErrors_h

#include <assert.h>


#ifndef DISTRIBUTION

#define CHECK_GL_ERRORS() \
do {                                                                                            	\
    GLenum error = glGetError();                                                                	\
    if(error != GL_NO_ERROR) {                                                                   	\
		NSLog(@"OpenGL: %s [error %d]", __FUNCTION__, (int)error);					\
		assert(0); \
	} \
} while(false)

#else

#define CHECK_GL_ERRORS()

#endif



#endif
