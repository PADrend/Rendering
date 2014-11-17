/*
 * Event.h
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#ifndef CL_EVENT_H_
#define CL_EVENT_H_

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

class Event {
public:
	Event();
	virtual ~Event() = default;

	cl::Event event;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* CL_EVENT_H_ */
