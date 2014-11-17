/*
 * CLUtils.h
 *
 *  Created on: Nov 17, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef CLUTILS_H_
#define CLUTILS_H_

#include <string>

namespace Rendering {
namespace CL {

const std::string getErrorString(int error);

} /* namespace CL */
} /* namespace Rendering */

#endif /* CLUTILS_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
