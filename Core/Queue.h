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

#include <memory>

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
	
	~Queue() = default;
	
	bool submit(const CommandBufferRef& commands);
	bool present();
	bool supports(QueueFamily type) const;

	uint32_t getIndex() const { return index; }
	uint32_t getFamilyIndex() const { return familyIndex; }
	
	CommandBufferHandle requestCommandBuffer(bool primary=true);
	void freeCommandBuffer(const CommandBufferHandle& bufferHandle, bool primary);

	DeviceRef getDevice() { return device.get(); }
	const QueueHandle& getApiHandle() const { return handle; }
	const CommandPoolHandle& getCommandPool() const { return commandPoolHandle; }
private:
	friend class Device;
	explicit Queue(const DeviceRef& device, uint32_t familyIndex, uint32_t index);
	bool init();
	CommandBufferHandle createCommandBuffer(bool primary);

	Util::WeakPointer<Device> device;
	QueueHandle handle;
	uint32_t familyIndex;
	uint32_t index;
	QueueFamily capabilities;
	CommandPoolHandle commandPoolHandle;
	Util::ObjectPool<CommandBufferHandle, uint32_t> commandPool;
};

inline QueueFamily operator | (QueueFamily lhs, QueueFamily rhs) {
	return static_cast<QueueFamily>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline QueueFamily operator & (QueueFamily lhs, QueueFamily rhs) {
	return static_cast<QueueFamily>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool Queue::supports(QueueFamily type) const { return (capabilities & type) != QueueFamily::None; }

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_QUEUE_H_ */
