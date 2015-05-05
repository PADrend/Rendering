/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Event.h"

COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wpedantic)
COMPILER_WARN_OFF(-Wold-style-cast)
COMPILER_WARN_OFF(-Wcast-qual)
COMPILER_WARN_OFF(-Wshadow)
COMPILER_WARN_OFF(-Wstack-protector)
#include <CL/cl.hpp>
COMPILER_WARN_POP

namespace Rendering {
namespace CL {

Event::Event(cl::Event* _event) : Util::ReferenceCounter<Event>(), event(_event) {}

Event::Event() : Util::ReferenceCounter<Event>(), event(new cl::Event()) {}

Event::~Event() = default;

Event::Event(const Event& _event) : Util::ReferenceCounter<Event>(), event(new cl::Event(*_event.event.get())) { }

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

int32_t Event::getStatus() const {
	return event->getInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>();
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
