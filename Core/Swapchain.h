/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_SWAPCHAIN_H_
#define RENDERING_CORE_SWAPCHAIN_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>
#include <Geometry/Vec2.h>

#include <memory>
#include <vector>

namespace Util {
namespace UI {
class Window;
using WindowRef = Util::Reference<Window>;
} /* UI */
} /* Util */

namespace Rendering {
class Device;
class FBO;
using DeviceRef = Util::Reference<Device>;
using FBORef = Util::Reference<FBO>;


class Swapchain : public Util::ReferenceCounter<Swapchain> {
public:
	using Ref = Util::Reference<Swapchain>;
	
	~Swapchain();
	uint32_t acquireNextIndex();

	bool resize(uint32_t width, uint32_t height);
	
	const FBORef& getCurrentFBO() const { return fbos[currentIndex]; }
	const FBORef& getFBO(uint32_t index) const { return fbos[index]; }
	uint32_t getCurrentIndex() const { return currentIndex; }
	uint32_t getSize() const { return imageCount; }

	const SwapchainHandle& getApiHandle() const { return handle; }
private:
	friend class Device;
	explicit Swapchain(const DeviceRef& device, const Geometry::Vec2i& extent);
	bool init();
	bool updateFramebuffers();

	Util::WeakPointer<Device> device;
	SwapchainHandle handle;

	Geometry::Vec2i extent;
	uint32_t imageCount;
	uint32_t currentIndex = 0;
	std::vector<FBORef> fbos;
	std::vector<FenceHandle> presentFences;
	uint32_t currentFence;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_SWAPCHAIN_H_ */
