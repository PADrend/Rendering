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

#include <Util/References.h>

#include <string>
#include <tuple>

namespace Rendering {
namespace CL {
class Platform;
class Device;
class Context;
class Program;
class Kernel;
class CommandQueue;
class Buffer;
class Image;
class Sampler;
class Memory;

typedef Util::Reference<Platform> PlatformRef;
typedef Util::Reference<Device> DeviceRef;
typedef Util::Reference<Context> ContextRef;
typedef Util::Reference<Program> ProgramRef;
typedef Util::Reference<Kernel> KernelRef;
typedef Util::Reference<CommandQueue> CommandQueueRef;
typedef Util::Reference<Buffer> BufferRef;
typedef Util::Reference<Image> ImageRef;
typedef Util::Reference<Sampler> SamplerRef;
typedef Util::Reference<Memory> MemoryRef;

template<uint32_t ...>
struct seq {};

template<uint32_t I, uint32_t ...S>
struct gens : gens<I-1, I-1, S...> {};

template<uint32_t ...S>
struct gens<0, S...> {
	typedef seq<S...> type;
};

inline bool validate() { return true; }

template<typename First, typename... Rest>
inline bool validate(First first, Rest... rest) {
	return first && validate(rest...);
}

const std::string getErrorString(int error);

/**
 * Finds the first platform and device associated with the desired device type (e.g. Device::TYPE_CPU or Device::TYPE_GPU)
 *
 * @param device_type The desired device type.
 * @return a tuple containing references to a platform object and a device object.
 */
std::tuple<PlatformRef, DeviceRef> getFirstPlatformAndDeviceFor(uint32_t device_type);

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_CLUTILS_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
