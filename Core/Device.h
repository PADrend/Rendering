/*
	This file is part of the Rendering library.
  Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DEVICE_H_
#define RENDERING_DEVICE_H_

#include "Common.h"
#include "Queue.h"

#include <Util/ReferenceCounter.h>
#include <memory>
#include <vector>
#include <set>

namespace Util {
namespace UI {
class Window;
using WindowRef = Util::Reference<Window>;
} /* UI */
} /* Util */

namespace Rendering {
class FBO;
using FBORef = Util::Reference<FBO>;
class Swapchain;
using SwapchainRef = Util::Reference<Swapchain>;
class CommandPool;
using CommandPoolRef = Util::Reference<CommandPool>;
class ResourceCache;
using ResourceCacheRef = Util::Reference<ResourceCache>;
class DescriptorPool;
using DescriptorPoolRef = Util::Reference<DescriptorPool>;

/** Represents a GPU Device	
	@ingroup rendering_core
*/
class Device : public Util::ReferenceCounter<Device> {
public:
	using Ref = Util::Reference<Device>;

	struct Configuration {
		Configuration(std::string n, uint32_t apiMajor=1, uint32_t apiMinor=0, bool debug=false) :
			name(n), apiVersionMajor(apiMajor), apiVersionMinor(apiMinor), debugMode(debug) { }
		std::string name;
		uint32_t apiVersionMajor;
		uint32_t apiVersionMinor;
		bool debugMode;
		std::vector<std::string> validationLayers;
	};
	
	static Ref create(Util::UI::WindowRef window, const Configuration& config);
	static Ref getDefault();
	~Device();
	
	void present();
	
	//! @name Extensions & limits
	//! @{
	bool isExtensionSupported(const std::string& extension) const;
	uint32_t getMaxFramebufferAttachments() const;
	uint32_t getMaxPushConstantSize() const;
	//! @}
	
	const Util::UI::WindowRef& getWindow() const;	
	const SwapchainRef& getSwapchain() const;
	const Queue::Ref& getQueue(QueueFamily family, uint32_t index=0) const;
	const Queue::Ref& getQueue(uint32_t familyIndex, uint32_t index=0) const;
	std::set<Queue::Ref> getQueues() const;
	const PipelineCacheHandle& getPipelineCache() const;
	const ResourceCacheRef& getResourceCache() const;
	const DescriptorPoolRef& getDescriptorPool() const;
	
	//! @name API Handles
	//! @{
	
	const SurfaceHandle& getSurface() const;
	const InstanceHandle& getInstance() const;
	const AllocatorHandle& getAllocator() const;
	const DeviceHandle& getApiHandle() const;
	
	//! @}
private:
	Device(Util::UI::WindowRef window);
	bool init(const Configuration& config);
	
	struct InternalData;
	std::unique_ptr<InternalData> internal;
};
	
} /* Rendering */

#endif /* end of include guard: RENDERING_DEVICE_H_ */
