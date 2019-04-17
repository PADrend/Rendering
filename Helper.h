/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_HELPER_H
#define RENDERING_HELPER_H

#include <cstdint>
#include <iosfwd>

namespace Rendering {

/** @addtogroup helper
 * @{
 */
 
void enableGLErrorChecking();
void disableGLErrorChecking();
void checkGLError(const char * file, int line);

/**
 * Return a human-readable description for the given OpenGL type.
 *
 * @param type Valid values are GL_BOOL, GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_FLOAT, and GL_DOUBLE.
 * @return String description of the type, or an empty string if the type is invalid.
 */
const char * getGLTypeString(uint32_t type);

/**
 * Return the size of the given OpenGL type.
 *
 * @param type Valid values are GL_BOOL, GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_FLOAT, and GL_DOUBLE.
 * @return Size in bytes, or zero if the type is invalid.
 */
uint32_t getGLTypeSize(uint32_t type);

/**
 * Write information about the current OpenGL context to the given stream.
 * 
 * @param output Output stream that the data is written to
 * @see @c glGetString
 */
void outputGLInformation(std::ostream & output);

/**
 * Return the supported graphics language version.
 *
 * @return Null-terminated string containing the supported graphics language version
 * @see constant @c GL_VERSION of function @c glGetString
 * @see @c glewIsSupported
 */
const char * getGraphicsLanguageVersion();

/**
 * Return the supported shading language version.
 *
 * @return Null-terminated string containing the supported shading language version
 * @see constant @c GL_SHADING_LANGUAGE_VERSION of function @c glGetString
 */
const char * getShadingLanguageVersion();

/**
 * Check for support of a specific OpenGL extension.
 *
 * @param extension Null-terminated string containing the name of the requested extension
 * @return @c true if the requested extension is supported, @c false otherwise.
 * @see @c glewIsSupported
 */
bool isExtensionSupported(const char * extension);

/**
 * Read a single value from the depth buffer.
 * 
 * @see @c glReadPixels
 */
float readDepthValue(int32_t x, int32_t y);

/**
 * Enable debug output that can be used to find errors or performance problems.
 * 
 * @see OpenGL extension @c GL_ARB_debug_output
 */
void enableDebugOutput();

/**
 * Disable the debug output again.
 * 
 * @see enableDebugOutput()
 */
void disableDebugOutput();

/**
 * Return the maximum allowed buffer binding locations for a specified buffer target.
 * 
 * @param target The buffer target
 * @see @c glGetIntegerv
 */
int32_t getMaxBufferBindings(uint32_t target);

/**
 * Return the maximum allowed combined texture units.
 * 
 * @see @c glGetIntegerv
 */
int32_t getMaxTextureBindings();

/**
 * Return the maximum allowed image units.
 * 
 * @see @c glGetIntegerv
 */
int32_t getMaxImageBindings();

//! @}
}

#define GET_GL_ERROR() \
	Rendering::checkGLError(__FILE__, __LINE__);

#endif /* RENDERING_HELPER_H */
