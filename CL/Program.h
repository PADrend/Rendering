/*
 * Program.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifndef PROGRAM_H_
#define PROGRAM_H_

#include "Context.h"
#include "Device.h"

#include <CL/cl.hpp>

#include <vector>
#include <string>

namespace Rendering {
namespace CL {

class Program {
public:
	enum BuildStatus_t { None, Error, Success, InProgress };

	Program(const Context& context, const std::string& source);
	virtual ~Program() = default;

	bool build(const std::vector<Device>& devices, const std::string& parameters = "");

	BuildStatus_t getBuildStatus(const Device& device);
	std::string getBuildOptions(const Device& device);
	std::string getBuildLog(const Device& device);

	cl::Program program;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* PROGRAM_H_ */
