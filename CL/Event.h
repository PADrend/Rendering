/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_EVENT_H_
#define RENDERING_CL_EVENT_H_

#include "CLUtils.h"

#include <Util/ReferenceCounter.h>

#include <memory>
#include <vector>
#include <functional>

namespace cl {
class Event;
}

namespace Rendering {
namespace CL {

class Event : public Util::ReferenceCounter<Event> {
protected:
	Event(cl::Event* event);
public:
	typedef std::function<void(const Event&, int32_t)> CallbackFn_t;

	Event();
	virtual ~Event();
	Event(const Event& event);
//	Event(Event&& event);
//	Event& operator=(Event&&);

	/**
	 * Waits on the host thread for commands identified by this event object to complete.
	 */
	void wait();

	/**
	 * Returns profiling information for the command associated with event if profiling is enabled.
	 * @return A 64-bit value that describes the current device time counter in nanoseconds when the command identified by event is enqueued in a command-queue by the host.
	 */
	uint64_t getProfilingCommandQueued() const;
	/**
	 * Returns profiling information for the command associated with event if profiling is enabled.
	 * @return A 64-bit value that describes the current device time counter in nanoseconds when the command identified by event that has been enqueued is submitted by the host to the device associated with the command-queue.
	 */
	uint64_t getProfilingCommandSubmit() const;
	/**
	 * Returns profiling information for the command associated with event if profiling is enabled.
	 * @return A 64-bit value that describes the current device time counter in nanoseconds when the command identified by event starts execution on the device.
	 */
	uint64_t getProfilingCommandStart() const;
	/**
	 * Returns profiling information for the command associated with event if profiling is enabled.
	 * @return A 64-bit value that describes the current device time counter in nanoseconds when the command identified by event has finished execution on the device.
	 */
	uint64_t getProfilingCommandEnd() const;

	/**
	 * Registers a user callback function for a specific command execution status.
	 *
	 * \warning Using lambda functions with by-reference capture do not work,
	 * because OpenCL makes a copy of the function and loosing the references (results in a segmentation fault).
	 *
	 * @param fun The event callback function that can be registered by the application.
	 * 			  This callback function may be called asynchronously by the OpenCL implementation.
	 * 			  It is the application's responsibility to ensure that the callback function is thread-safe.
	 */
	void setCallback(CallbackFn_t fun);

	/**
	 * Waits on the host thread for commands identified by event objects to complete.
	 * @param events The events specified in events act as synchronization points.
	 */
	static void waitForEvents(const std::vector<Event*>& events);

	cl::Event* _internal() const { return event.get(); }
protected:
	std::unique_ptr<cl::Event> event;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_EVENT_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
