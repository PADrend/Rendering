/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_PLATFORM_H_
#define RENDERING_CL_PLATFORM_H_

#include "CLUtils.h"

#include <Util/ReferenceCounter.h>

#include <vector>
#include <string>
#include <memory>

namespace cl {
class Platform;
}

namespace Rendering {
namespace CL {
class Device;

class Platform : public Util::ReferenceCounter<Platform> {
public:
//	Platform();
	Platform(cl::Platform* platform);
	~Platform();
	Platform(const Platform& platform);
//	Platform(Platform&& platform);
//	Platform& operator=(Platform&&);

	std::string getExtensions() const;
	std::string getName() const;
	std::string getProfile() const;
	std::string getVendor() const;
	std::string getVersion() const;

	std::vector<DeviceRef> getDevices();

	/**
	 * Returns a list of available platforms
	 */
	static std::vector<PlatformRef> get();

	cl::Platform* _internal() const { return platform.get(); };
private:
	std::unique_ptr<cl::Platform> platform;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_PLATFORM_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
