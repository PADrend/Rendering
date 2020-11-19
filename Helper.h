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

#include <Util/TypeConstant.h>
#include <Util/IO/FileLocator.h>

#include <cstdint>
#include <iosfwd>

#define GET_GL_ERROR()

namespace Rendering {
	
//! @defgroup rendering_helper Helper

/** @addtogroup rendering_helper
 * @{
 */

/**
 * Triggers a capture of the current GL state in RenderDoc.
 * @note Requires RenderDoc
 */
RENDERINGAPI void triggerCapture();

/**
 * Starts capturing of the GL state in RenderDoc.
 * @note Requires RenderDoc
 */
RENDERINGAPI void startCapture();

/**
 * Ends the active capture.
 * @note Requires RenderDoc
 */
RENDERINGAPI void endCapture();

/**
 * Gets the default locator for finding the "data/" directory.
 */
RENDERINGAPI const Util::FileLocator& getDataLocator();


[[deprecated]]
static void enableGLErrorChecking() {}

[[deprecated]]
static void disableGLErrorChecking() {}

[[deprecated]]
static void checkGLError(const char * file, int line) {}

[[deprecated]]
RENDERINGAPI uint32_t getGLTypeSize(uint32_t type);

[[deprecated]]
RENDERINGAPI uint32_t getGLType(Util::TypeConstant type);

[[deprecated]]
RENDERINGAPI Util::TypeConstant getAttributeType(uint32_t glType);

[[deprecated]]
static const char * getGLTypeString(uint32_t type) { return Util::getTypeString(getAttributeType(type)).c_str(); }

[[deprecated]]
static void outputGLInformation(std::ostream & output) {}

[[deprecated]]
static const char * getGraphicsLanguageVersion() { return ""; }

[[deprecated]]
static const char * getShadingLanguageVersion() { return ""; }

[[deprecated("Use Device::isExtensionSuported instead.")]]
RENDERINGAPI bool isExtensionSupported(const char * extension);

[[deprecated]]
static float readDepthValue(int32_t x, int32_t y) { return 0; }

[[deprecated("Should be enabled during device creation.")]]
static void enableDebugOutput() {}

[[deprecated]]
static void disableDebugOutput() {}

[[deprecated]]
static void pushDebugGroup(const std::string& name) {}

[[deprecated]]
static void popDebugGroup() {}

//! @}
}

#endif /* RENDERING_HELPER_H */
