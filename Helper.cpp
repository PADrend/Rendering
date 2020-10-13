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
#include "Core/Device.h"

#include <iostream>
#include <iomanip>
#if defined(ANDROID)
#include <android/log.h>
#endif /* defined(ANDROID) */

#ifdef _WIN32
#ifndef WIN32
#define WIN32
#endif
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#endif

#include "extern/renderdoc_app.h"

#include <Util/Macros.h>
#include <Util/TypeConstant.h>
#include <Util/Resources/ResourceFormat.h>

namespace Rendering {

bool isExtensionSupported(const char * extension) {
	return Device::getDefault()->isExtensionSupported(extension);
}

static RENDERDOC_API_1_4_0* getAPI() {
  static RENDERDOC_API_1_4_0* rdoc_api = nullptr;
  if(!rdoc_api) {    
#if defined(_WIN32)
    if(HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
      pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
      int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
      if(ret != 1) {
        WARN("Could not load RenderDoc API");
        rdoc_api = nullptr;
      }
    }
#elif defined(__linux__)
    if(void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
      pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
      int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
      if(ret != 1) {
        WARN("Could not load RenderDoc API");
        rdoc_api = nullptr;
      }
    }
#endif
  }
  return rdoc_api;
}

void triggerCapture() {
  auto rdoc_api = getAPI();  
  if(rdoc_api) {
    rdoc_api->TriggerCapture();
  } else {
    WARN("RenderDoc API is not loaded!");
  }
}

void startCapture() {
  auto rdoc_api = getAPI();
  if(rdoc_api) {
    rdoc_api->StartFrameCapture(nullptr, nullptr);
  } else {
    WARN("RenderDoc API is not loaded!");
  }
}

void endCapture() {
  auto rdoc_api = getAPI();
  if(rdoc_api) {
    rdoc_api->EndFrameCapture(nullptr, nullptr);
  }
}

const Util::FileLocator& getDataLocator() {
	static Util::FileLocator locator = []() {
		Util::FileLocator locator;

		// First check for a environment variable
		if(std::getenv("RENDERING_DATA_DIR"))
			locator.addSearchPath(std::string(std::getenv("RENDERING_DATA_DIR")) + "/"); // environment variable
	#ifdef RENDERING_DATA_DIR
		locator.addSearchPath(std::string(RENDERING_DATA_DIR) + "/"); // compiler defined
	#endif
		std::string exePath = Util::FileName(Util::Utils::getExecutablePath()).getDir(); // executable path
		locator.addSearchPath(exePath + "/../share/Rendering/");
		locator.addSearchPath(exePath + "/modules/Rendering/data/");
		locator.addSearchPath(exePath + "/../modules/Rendering/data/");
		return locator;
	}();
	return locator;
}

#define GL_BOOL 0x8B56
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT 0x1405
#define GL_BYTE 0x1400
#define GL_UNSIGNED_SHORT 0x1403
#define GL_SHORT 0x1402
#define GL_INT 0x1404
#define GL_FLOAT 0x1406
#define GL_INT_2_10_10_10_REV 0x8D9F
#define GL_DOUBLE 0x140A
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_HALF_FLOAT 0x140B

unsigned int getGLTypeSize(uint32_t type) {
	switch (type) {
		case GL_BOOL: return sizeof(uint8_t);
		case GL_UNSIGNED_BYTE: return sizeof(uint8_t);
		case GL_BYTE: return sizeof(int8_t);
		case GL_UNSIGNED_SHORT: return sizeof(uint16_t);
		case GL_SHORT: return sizeof(int16_t);
		case GL_UNSIGNED_INT: return sizeof(uint32_t);
		case GL_INT: return sizeof(int32_t);
		case GL_FLOAT: return sizeof(float);
		case GL_DOUBLE: return sizeof(double);
		case GL_UNSIGNED_INT_24_8: return sizeof(uint32_t);
		case GL_HALF_FLOAT: return sizeof(uint16_t);
		case GL_INT_2_10_10_10_REV: return sizeof(uint32_t);
		default: return 0;
	}
}

uint32_t getGLType(Util::TypeConstant type) {	
	switch(static_cast<Util::TypeConstant>(type)) {
		case Util::TypeConstant::UINT8: return GL_UNSIGNED_BYTE;
		case Util::TypeConstant::UINT16: return GL_UNSIGNED_SHORT;
		case Util::TypeConstant::UINT32: return GL_UNSIGNED_INT;
		case Util::TypeConstant::UINT64: return 0; // unsupported
		case Util::TypeConstant::INT8: return GL_BYTE;
		case Util::TypeConstant::INT16: return GL_SHORT;
		case Util::TypeConstant::INT32: return GL_INT;
		case Util::TypeConstant::INT64: return 0; // unsupported
		case Util::TypeConstant::FLOAT: return GL_FLOAT;
		case Util::TypeConstant::DOUBLE: return GL_DOUBLE;
		case Util::TypeConstant::HALF: return GL_HALF_FLOAT;
	}
}

Util::TypeConstant getAttributeType(uint32_t glType) {
	switch (glType) {
		case GL_UNSIGNED_BYTE: return Util::TypeConstant::UINT8;
		case GL_UNSIGNED_SHORT: return Util::TypeConstant::UINT16;
		case GL_UNSIGNED_INT: return Util::TypeConstant::UINT32;
		case GL_BYTE: return Util::TypeConstant::INT8;
		case GL_SHORT: return Util::TypeConstant::INT16;
		case GL_INT: return Util::TypeConstant::INT32;
		case GL_FLOAT: return Util::TypeConstant::FLOAT;
		case GL_DOUBLE: return Util::TypeConstant::DOUBLE;
		case GL_HALF_FLOAT: return Util::TypeConstant::HALF;
		default: return Util::TypeConstant::UINT8;
	}
}

}
