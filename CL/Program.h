/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_PROGRAM_H_
#define RENDERING_CL_PROGRAM_H_

#include <vector>
#include <string>
#include <memory>

namespace cl {
class Program;
}

namespace Rendering {
namespace CL {
class Context;
class Device;

class Program {
public:
	enum BuildStatus_t { None, Error, Success, InProgress };

	Program(Context* context, const std::vector<std::string>& sources);
	~Program();
	Program(const Program& program);
	Program(Program&& program);
	Program& operator=(Program&&);

	bool build(const std::vector<Device>& devices, const std::string& options = "");

	BuildStatus_t getBuildStatus(const Device& device) const;
	std::string getBuildOptions(const Device& device) const;
	std::string getBuildLog(const Device& device) const;

	std::vector<char*> getBinaries() const;
	std::vector<size_t> getBinarySizes() const;
	std::vector<Device> getDevices() const;
	uint32_t getNumDevices() const;
	std::string getKernelNames() const;
	uint32_t getNumKernels() const;
	std::string getSource() const;

	Context* getContext() const;

	cl::Program* _internal() const { return program.get(); }
private:
	std::unique_ptr<cl::Program> program;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_PROGRAM_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
