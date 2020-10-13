/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDPOOL_H_
#define RENDERING_CORE_COMMANDPOOL_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>

#include <vector>
#include <deque>
#include <functional>

namespace Rendering {
class Device;
class CommandBuffer;
using CommandBufferRef = Util::Reference<CommandBuffer>;
using DeviceRef = Util::Reference<Device>;

//-------------------------------------------------------

class CommandPool : public Util::ReferenceCounter<CommandPool> {
public:
	using Ref = Util::Reference<CommandPool>;
	
	~CommandPool();
	CommandBufferRef requestCommandBuffer(bool primary=true);
	
	const CommandPoolHandle& getApiHandle() const { return handle; }
	uint32_t getQueueFamily() const { return queueFamily; }
private:
	friend class Device;
	explicit CommandPool(const DeviceRef& device, uint32_t queueFamily);

	Util::WeakPointer<Device> device;
	uint32_t queueFamily;
	CommandPoolHandle handle;
	std::deque<CommandBufferRef> active;
	std::vector<CommandBufferRef> freePrimary;
	std::vector<CommandBufferRef> freeSecondary;
};


} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMANDPOOL_H_ */
