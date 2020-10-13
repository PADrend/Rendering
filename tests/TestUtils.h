/*
 * TestUtils.h
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#include <Util/UI/Window.h>
#include "../Core/Device.h"

#include <memory>

class TestUtils {
public:
	static Util::Reference<Util::UI::Window> window;
	static Rendering::Device::Ref device;
};

#endif /* TESTUTILS_H_ */
