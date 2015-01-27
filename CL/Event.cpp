/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Event.h"

#pragma warning(push, 0)
#include <CL/cl.hpp>
#pragma warning(pop)

namespace Rendering {
namespace CL {

Event::Event(cl::Event* event) : event(event) {}

Event::Event() : event(new cl::Event()) {}

Event::~Event() = default;

Event::Event(const Event& event) : event(new cl::Event(*event.event.get())) { }

//Event::Event(Event&& event) = default;
//
//Event& Event::operator=(Event&&) = default;

void Event::wait() {
	event->wait();
}

uint64_t Event::getProfilingCommandQueued() const {
	return event->getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>();
}

uint64_t Event::getProfilingCommandSubmit() const {
	return event->getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
}

uint64_t Event::getProfilingCommandStart() const {
	return event->getProfilingInfo<CL_PROFILING_COMMAND_START>();
}

uint64_t Event::getProfilingCommandEnd() const {
	return event->getProfilingInfo<CL_PROFILING_COMMAND_END>();
}

void Event::setCallback(CallbackFn_t fun) {

	struct fnWrapper{
		CallbackFn_t function;
		static void callback(cl_event event, cl_int status, void * userData) {
			fnWrapper* wrapper = static_cast<fnWrapper*>(userData);
			// for some reason this is required, otherwise the event will be destroyed
			::clRetainEvent(event);
			Event e(new cl::Event(event));
			wrapper->function(e, status);
			delete wrapper;
		}
	};

//	fnWrapper wrapper{fun};
	event->setCallback(CL_COMPLETE, fnWrapper::callback, new fnWrapper{fun});
}

void Event::waitForEvents(const std::vector<Event*>& events) {
	std::vector<cl::Event> cl_events;
	for(auto e : events)
		cl_events.push_back(*e->_internal());
	cl::Event::waitForEvents(cl_events);
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */
