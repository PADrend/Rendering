/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_INTERNAL_VKFRAMEBUFFER_H_
#define RENDERING_CORE_INTERNAL_VKFRAMEBUFFER_H_

#include "../Common.h"
#include "../Device.h"
#include "../ImageView.h"
#include "../../State/PipelineState.h"
#include "../../FBO.h"
#include "../../Texture/Texture.h"

#include <Util/Graphics/Color.h>

namespace Rendering {

ApiBaseHandle::Ref createRenderPassHandle(Device* device, const FramebufferFormat& state, bool clearColor, bool clearDepth, bool clearStencil, const std::vector<ResourceUsage>& lastColorUsages, ResourceUsage lastDepthUsage);

ApiBaseHandle::Ref createFramebufferHandle(Device* device, FBO* fbo, VkRenderPass renderpass);

} /* Rendering */


#endif /* end of include guard: RENDERING_CORE_INTERNAL_VKFRAMEBUFFER_H_ */