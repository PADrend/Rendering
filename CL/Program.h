/*
 * Program.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef PROGRAM_H_
#define PROGRAM_H_

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

	Program(Context* context, const std::string& source);
	virtual ~Program() = default;

	bool build(const std::vector<Device*>& devices, const std::string& parameters = "");

	BuildStatus_t getBuildStatus(Device* device);
	std::string getBuildOptions(Device* device);
	std::string getBuildLog(Device* device);

	cl::Program* _internal() const { return program.get(); }
private:
	std::unique_ptr<cl::Program> program;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* PROGRAM_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
