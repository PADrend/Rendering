/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_INTERNAL_VKPIPELINE_H_
#define RENDERING_CORE_INTERNAL_VKPIPELINE_H_

#include "../Common.h"
#include "../Device.h"
#include "../ResourceCache.h"
#include "../../FBO.h"
#include "../../Shader/Shader.h"
#include "../../State/ShaderLayout.h"
#include "../../State/PipelineState.h"

namespace Rendering {

ApiBaseHandle::Ref createDescriptorSetLayoutHandle(Device* device, const ShaderResourceLayoutSet& layoutSet);

ApiBaseHandle::Ref createPipelineLayoutHandle(Device* device, const ShaderLayout& layout);

ApiBaseHandle::Ref createComputePipelineHandle(Device* device, const PipelineState& state, VkPipeline parent);

ApiBaseHandle::Ref createGraphicsPipelineHandle(Device* device, const PipelineState& state, VkPipeline parent);

ApiBaseHandle::Ref createPipelineHandle(Device* device, const PipelineState& state, VkPipeline parent);

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_INTERNAL_VKPIPELINE_H_ */