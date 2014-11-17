/*
 * TestUtils.h
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#include <Util/UI/Window.h>

#include <memory>

class TestUtils {
public:
	static std::unique_ptr<Util::UI::Window> window;
};

#endif /* TESTUTILS_H_ */
