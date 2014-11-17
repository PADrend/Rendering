/*
 * Platform.h
 *
 *  Created on: Nov 12, 2014
 *      Author: sascha
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <vector>
#include <string>

#include <CL/cl.hpp>

#include "Device.h"

namespace Rendering {
namespace CL {

class Platform {
public:
	Platform() {};
	Platform(const cl::Platform& platform);
	virtual ~Platform() = default;

	std::string getExtensions() const;
	std::string getName() const;
	std::string getProfile() const;
	std::string getVendor() const;
	std::string getVersion() const;

	std::vector<Device> getDevices() const;

	/**
	 * Returns a list of available platforms
	 */
	static void get(std::vector<Platform>* platforms);
private:
	friend class Context;
	cl::Platform platform;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* PLATFORM_H_ */
