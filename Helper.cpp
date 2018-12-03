/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Helper.h"
#include "GLHeader.h"
#include <iostream>
#include <iomanip>
#if defined(ANDROID)
#include <android/log.h>
#endif /* defined(ANDROID) */

namespace Rendering {

static bool GLErrorChecking = false;

void enableGLErrorChecking() {
	GLErrorChecking = true;
}

void disableGLErrorChecking() {
	GLErrorChecking = false;
}

static const char * getGLErrorString(GLenum errorFlag) {
	switch (errorFlag) {
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
#ifdef LIB_GL
		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
		case GL_TABLE_TOO_LARGE:
			return "GL_TABLE_TOO_LARGE";
#endif /* LIB_GL */
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		default:
			return "Unknown error";
	}
}

void checkGLError(const char * file, int line) {
	if(!GLErrorChecking) {
		return;
	}
	// Call glGetError() in a loop, because there might be multiple recorded errors.
	GLenum errorFlag = glGetError();
	while (errorFlag != GL_NO_ERROR) {
#if defined(ANDROID)
		__android_log_print(ANDROID_LOG_WARN, "RenderingMobile", "GL ERROR (%i):%s at %s:%i", errorFlag, getGLErrorString(errorFlag), file, line);
#else
		std::cerr << "GL ERROR (0x" << std::hex << errorFlag << "):" << getGLErrorString(errorFlag) << " at " << file << ":" << std::dec << line << std::endl;
#endif
		errorFlag = glGetError();
	}
}

const char * getGLTypeString(uint32_t type) {
	switch (static_cast<GLenum>(type)) {
		case GL_BOOL:
			return "bool";
		case GL_UNSIGNED_BYTE:
			return "uchar";
		case GL_BYTE:
			return "char";
		case GL_UNSIGNED_SHORT:
			return "ushort";
		case GL_SHORT:
			return "short";
		case GL_UNSIGNED_INT:
			return "uint";
		case GL_INT:
			return "int";
		case GL_FLOAT:
			return "float";
#ifdef LIB_GL
		case GL_DOUBLE:
			return "double";
		case GL_UNSIGNED_INT_24_8_EXT:
			return "uint_24_8_EXT";
		case GL_HALF_FLOAT:
			return "half";
		case GL_INT_2_10_10_10_REV:
			return "int_2_10_10_10_REV";
#endif
		default:
			return "";
	}
}

unsigned int getGLTypeSize(uint32_t type) {
	switch (static_cast<GLenum>(type)) {
		case GL_BOOL:
			return sizeof(GLboolean);
		case GL_UNSIGNED_BYTE:
			return sizeof(GLubyte);
		case GL_BYTE:
			return sizeof(GLbyte);
		case GL_UNSIGNED_SHORT:
			return sizeof(GLushort);
		case GL_SHORT:
			return sizeof(GLshort);
		case GL_UNSIGNED_INT:
			return sizeof(GLuint);
		case GL_INT:
			return sizeof(GLint);
		case GL_FLOAT:
			return sizeof(GLfloat);
	#ifdef LIB_GL
		case GL_DOUBLE:
			return sizeof(GLdouble);
		case GL_UNSIGNED_INT_24_8_EXT:
			return sizeof(GLuint);
		case GL_HALF_FLOAT:
			return sizeof(GLhalf);
		case GL_INT_2_10_10_10_REV:
			return sizeof(GLint);
	#endif /* LIB_GL */
		default:
			return 0;
	}
}

void outputGLInformation(std::ostream & output) {
	output << "OpenGL vendor: " << glGetString(GL_VENDOR) << '\n';
	output << "OpenGL renderer: " << glGetString(GL_RENDERER) << '\n';
	output << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
	output << "OpenGL shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';
}

const char * getGraphicsLanguageVersion() {
	return reinterpret_cast<const char *>(glGetString(GL_VERSION));
}

const char * getShadingLanguageVersion() {
	return reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
}

bool isExtensionSupported(const char * extension) {
#if defined(LIB_GLEW)
	return glewIsSupported(extension);
#else
	return false;
#endif
}

float readDepthValue(int32_t x, int32_t y) {
	GLfloat z;
	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
	return z;
}

#define STDCALL 

#if defined(LIB_GLEW) && defined(LIB_GL) && defined(GL_ARB_debug_output)
#if defined(_WIN32)
#define STDCALL __attribute__((__stdcall__))
#endif

static void debugCallback(GLenum, GLenum, GLuint, GLenum, GLsizei , const char *, const void * ) STDCALL;
// the following alias function is required as different versions of glew define GLDEBUGPROCARB differently:
//   with void*userParam OR const void*userParam
static void debugCallback(GLenum, GLenum, GLuint, GLenum, GLsizei , const char *, void * ) STDCALL;

static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char *message, void *userParam ){
	debugCallback(source,type,id,severity,length,message,static_cast<const void*>(userParam));
}

static void debugCallback(GLenum source,
						  GLenum type,
						  GLuint id,
						  GLenum severity,
						  GLsizei /*length*/,
						  const char * message,
                          const void * /*userParam*/) {
	std::cerr << "GL DEBUG source=";
	switch(source) {
		case GL_DEBUG_SOURCE_API_ARB:
			std::cerr << "GL";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
			std::cerr << "GLSL";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
			std::cerr << "GLX/WGL";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
			std::cerr << "ThirdParty";
			break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB:
			std::cerr << "Application";
			break;
		case GL_DEBUG_SOURCE_OTHER_ARB:
		default:
			std::cerr << "Other";
			break;
	}
	std::cerr << " type=";
	switch(type) {
		case GL_DEBUG_TYPE_ERROR_ARB:
			std::cerr << "Error";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
			std::cerr << "DeprecatedBehaviour";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
			std::cerr << "UndefinedBehaviour";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:
			std::cerr << "Performance";
			break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:
			std::cerr << "Portability";
			break;
		case GL_DEBUG_TYPE_OTHER_ARB:
		default:
			std::cerr << "Other";
			break;
	}
	std::cerr << " id=" << id << " severity=";
	switch(severity) {
		case GL_DEBUG_SEVERITY_HIGH_ARB:
			std::cerr << "High";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB:
			std::cerr << "Medium";
			break;
		case GL_DEBUG_SEVERITY_LOW_ARB:
		default:
			std::cerr << "Low";
			break;
	}
	std::cerr << " message=" << message << std::endl;
}
#endif

void enableDebugOutput() {
#if defined(LIB_GLEW) && defined(LIB_GL) && defined(GL_ARB_debug_output)
	if(!glewIsSupported("GL_ARB_debug_output")) {
		std::cerr << "GL_ARB_debug_output is not supported" << std::endl;
		return;
	}
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	glDebugMessageCallbackARB(&debugCallback, nullptr);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageInsertARB(GL_DEBUG_SOURCE_THIRD_PARTY_ARB, GL_DEBUG_TYPE_OTHER_ARB, 1, GL_DEBUG_SEVERITY_LOW_ARB, -1, "Rendering: Debugging enabled");
#else
	std::cerr << "GL_ARB_debug_output is not supported" << std::endl;
#endif
}

void disableDebugOutput() {
#if defined(LIB_GLEW) && defined(LIB_GL) && defined(GL_ARB_debug_output)
	if(!glewIsSupported("GL_ARB_debug_output")) {
		std::cerr << "GL_ARB_debug_output is not supported" << std::endl;
		return;
	}
	glDebugMessageInsertARB(GL_DEBUG_SOURCE_THIRD_PARTY_ARB, GL_DEBUG_TYPE_OTHER_ARB, 2, GL_DEBUG_SEVERITY_LOW_ARB, -1, "Rendering: Debugging disabled");
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
	glDebugMessageCallbackARB(nullptr, nullptr);
	glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
#else
	std::cerr << "GL_ARB_debug_output is not supported" << std::endl;
#endif
}

}
