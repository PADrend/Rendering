/*
 * Platform.h
 *
 *  Created on: Nov 12, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <vector>
#include <string>
#include <memory>

namespace cl {
class Platform;
}

namespace Rendering {
namespace CL {
class Device;

class Platform {
public:
	Platform(cl::Platform* platform);
	virtual ~Platform() = default;

	std::string getExtensions() const;
	std::string getName() const;
	std::string getProfile() const;
	std::string getVendor() const;
	std::string getVersion() const;

	std::vector<Device*> getDevices() const;

	/**
	 * Returns a list of available platforms
	 */
	static void get(std::vector<Platform*>& platforms);

	cl::Platform* _internal() const { return platform.get(); };
private:
	std::unique_ptr<cl::Platform> platform;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* PLATFORM_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
