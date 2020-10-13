/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_FENCE_H_
#define RENDERING_CORE_FENCE_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>

#include <deque>

namespace Rendering {
class Queue;
using QueueRef = Util::Reference<Queue>;

class Fence : public Util::ReferenceCounter<Fence> {
public:
	using Ref = Util::Reference<Fence>;
	static Ref create();
	~Fence();
	Fence(Fence&& o) = default;
	Fence(const Fence& o) = delete;

	//! Wait until the GPU reached the current CPU value.
	void wait();

	//! Insert a signal command in the command queue & increase the CPU value.
	uint64_t signal(const QueueRef& queue);

	//! Retrieve the current GPU value.
	uint64_t getGpuValue();

	//! Retrieve the current CPU value.
	uint64_t getCpuValue() const { return cpuValue; }
private:
	Fence() {};
	uint64_t cpuValue = 0;
	uint64_t gpuValue = 0;
	std::deque<FenceHandle> fenceQueue;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_FENCE_H_ */