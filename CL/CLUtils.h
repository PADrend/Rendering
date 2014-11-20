/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_CLUTILS_H_
#define RENDERING_CL_CLUTILS_H_

#include <string>
#include <tuple>

namespace Rendering {
namespace CL {
class Platform;
class Device;

const std::string getErrorString(int error);

std::tuple<Platform, Device> getFirstPlatformAndDeviceFor(uint32_t device_type);

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_CLUTILS_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
