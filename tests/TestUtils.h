/*
 * TestUtils.h
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#include <Util/UI/Window.h>
#include "../RenderDevice.h"
#include "../RenderFrameContext.h"

#include <memory>

class TestUtils {
public:
	static Util::Reference<Util::UI::Window> window;
	static Rendering::RenderDeviceHandle device;
	static Rendering::RenderFrameContextHandle frameContext;
	//static std::unique_ptr<Rendering::RenderingContext> context;
};

#endif /* TESTUTILS_H_ */
