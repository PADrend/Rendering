/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_QUEUE_H_
#define RENDERING_CORE_QUEUE_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>
#include <Util/Factory/ObjectPool.h>

#include <deque>
#include <mutex>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class CommandBuffer;
using CommandBufferRef = Util::Reference<CommandBuffer>;
class Swapchain;
using SwapchainRef = Util::Reference<Swapchain>;

class Queue : public Util::ReferenceCounter<Queue> {
public:
	using Ref = Util::Reference<Queue>;
	
	~Queue();
	
	bool submit(const CommandBufferRef& commands);
	bool submit(const FenceHandle& fence);
	bool present();
	void wait();

	bool supports(QueueFamily type) const { return (capabilities & type) != QueueFamily::None; }

	uint32_t getIndex() const { return index; }
	uint32_t getFamilyIndex() const { return familyIndex; }
	
	CommandBufferHandle requestCommandBuffer(bool primary=true, uint32_t threadId=0);
	void freeCommandBuffer(const CommandBufferHandle& bufferHandle, bool primary, uint32_t threadId=0);

	DeviceRef getDevice() { return device.get(); }
	const QueueHandle& getApiHandle() const { return handle; }
private:
	friend class Device;
	explicit Queue(const DeviceRef& device, uint32_t familyIndex, uint32_t index);
	bool init();
	void clearPending();
	CommandBufferHandle createCommandBuffer(CommandPoolHandle pool, bool primary);

	Util::WeakPointer<Device> device;
	QueueHandle handle;
	uint32_t familyIndex;
	uint32_t index;
	QueueFamily capabilities;
	Util::ObjectPool<CommandBufferHandle, int32_t> commandPool;

	struct PendingEntry {
		CommandBufferRef cmd;
		FenceHandle fence;
	};
	std::deque<PendingEntry> pendingQueue;
	std::mutex submitMutex;
	std::mutex poolMutex;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_QUEUE_H_ */
