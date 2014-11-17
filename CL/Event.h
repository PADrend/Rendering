/*
 * Event.h
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef CL_EVENT_H_
#define CL_EVENT_H_

#include <memory>

namespace cl {
class Event;
}

namespace Rendering {
namespace CL {

class Event {
public:
	Event();
	virtual ~Event() = default;

	void wait();

	cl::Event* _internal() const { return event.get(); }
private:
	std::unique_ptr<cl::Event> event;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* CL_EVENT_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
