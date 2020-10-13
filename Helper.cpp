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

}
