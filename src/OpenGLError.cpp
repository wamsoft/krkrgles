
#include "OpenGLHeader.h"
#include <stdio.h>


bool _CheckGLErrorAndLog(const char *file, const int lineno, const char* funcname) 
{
	GLenum error_code = glGetError();
	if( error_code == GL_NO_ERROR ) return true;
	if( funcname != nullptr ) {
		char buff[256+1];
		snprintf( buff, 256, "%s:%d %s error:%08x", file, lineno, funcname, error_code);
		TVPAddLog( buff );
	}
	switch( error_code ) {
	case GL_INVALID_ENUM: TVPAddLog( TJS_W( "GL error : GL_INVALID_ENUM." ) ); break;
	case GL_INVALID_VALUE: TVPAddLog( TJS_W( "GL error : GL_INVALID_VALUE." ) ); break;
	case GL_INVALID_OPERATION: TVPAddLog( TJS_W( "GL error : GL_INVALID_OPERATION." ) ); break;
	case GL_OUT_OF_MEMORY: TVPAddLog( TJS_W( "GL error : GL_OUT_OF_MEMORY." ) ); break;
	case GL_INVALID_FRAMEBUFFER_OPERATION: TVPAddLog( TJS_W( "GL error : GL_INVALID_FRAMEBUFFER_OPERATION." ) ); break;
	default: break;
	}
	return false;
}
