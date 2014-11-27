/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_KERNEL_H_
#define RENDERING_CL_KERNEL_H_

#include "CLUtils.h"
#include "Memory/Buffer.h"
#include "Memory/Image.h"
#include "Memory/Sampler.h"

#include <Util/ReferenceCounter.h>

#include <string>
#include <memory>
#include <array>

namespace cl {
class Kernel;
}

namespace Rendering {
namespace CL {
class Program;
class Device;
class Context;
class Sampler;

class Kernel : public Util::ReferenceCounter<Kernel>  {
public:
	Kernel(Program* program, const std::string& name);
	~Kernel();
	Kernel(const Kernel& kernel);
//	Kernel(Kernel&& kernel);
//	Kernel& operator=(Kernel&&);

//	bool setArg(uint32_t index, float value);

	bool setArg(uint32_t index, size_t size, void* ptr);

	template<typename T>
	inline bool setArg(uint32_t index, T value) {
		return setArg(index, sizeof(T), &value);
	}

	template<uint32_t I, typename T>
	inline bool setArg(T value) {
		return setArg(I, value);
	}

	template<typename... Args>
	inline bool setArgs(Args&&... args) {
		return setArgs(typename gens<sizeof...(Args)>::type(), args...);
	}

	std::string getAttributes() const;
	std::string getFunctionName() const;
	uint32_t getNumArgs() const;

	std::string getArgName(uint32_t index) const;
	std::string getArgTypeName(uint32_t index) const;

//	std::array<size_t, 3> getGlobalWorkSize(Device* device) const;
	size_t getWorkGroupSize(Device* device) const;
	std::array<size_t, 3> getCompileWorkGroupSize(Device* device) const;
	uint64_t getLocalMemSize(Device* device) const;
	size_t getPreferredWorkGroupSizeMultiple(Device* device) const;
	uint64_t getPrivateMemSize(Device* device) const;

	Program* getProgram() const { return program.get(); }

	cl::Kernel* _internal() const { return kernel.get(); }
private:
	bool _setArg(uint32_t index, Memory* value);
	bool _setArg(uint32_t index, Sampler* value);

	template<uint32_t ...S, typename... Args>
	inline bool setArgs(seq<S...>, Args&&... args) {
		return validate(setArg<S>(args)...);
	}

	std::unique_ptr<cl::Kernel> kernel;
	ProgramRef program;
};

template<>
inline bool Kernel::setArg<BufferRef>(uint32_t index, BufferRef value) {
	return _setArg(index, value.get());
}

template<>
inline bool Kernel::setArg<ImageRef>(uint32_t index, ImageRef value) {
	return _setArg(index, value.get());
}

template<>
inline bool Kernel::setArg<MemoryRef>(uint32_t index, MemoryRef value) {
	return _setArg(index, value.get());
}

template<>
inline bool Kernel::setArg<SamplerRef>(uint32_t index, SamplerRef value) {
	return _setArg(index, value.get());
}

template<>
inline bool Kernel::setArg<Memory*>(uint32_t index, Memory* value) {
	return _setArg(index, value);
}

template<>
inline bool Kernel::setArg<Buffer*>(uint32_t index, Buffer* value) {
	return _setArg(index, value);
}

template<>
inline bool Kernel::setArg<Image*>(uint32_t index, Image* value) {
	return _setArg(index, value);
}

template<>
inline bool Kernel::setArg<Sampler*>(uint32_t index, Sampler* value) {
	return _setArg(index, value);
}

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_KERNEL_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
