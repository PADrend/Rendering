/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_PIPELINE_CACHE_H_
#define RENDERING_CORE_PIPELINE_CACHE_H_

#include "Common.h"
#include "Pipeline.h"
#include "../RenderingContext/PipelineState.h"

#include <Util/ReferenceCounter.h>

#include <unordered_map>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class PipelineCache : public Util::ReferenceCounter<PipelineCache> {
public:
	using Ref = Util::Reference<PipelineCache>;
	PipelineCache(PipelineCache &&) = default;
	PipelineCache(const PipelineCache &) = delete;
	~PipelineCache();

	Pipeline::Ref requestPipeline(PipelineType type, const PipelineState& state, const Pipeline::Ref& parent = nullptr);
	Pipeline::Ref requestGraphicsPipeline(const PipelineState& state, const Pipeline::Ref& parent = nullptr);
	Pipeline::Ref requestComputePipeline(const ShaderRef& shader, const std::string& entrypoint = "main", const Pipeline::Ref& parent = nullptr);

	const PipelineCacheHandle& getApiHandle() const { return handle; }

	uint32_t getSize() const { return cache.size(); }
	void clear() { cache.clear(); }
	DeviceRef getDevice() const { return device.get(); }
private:
	friend class Device;
	PipelineCache(const DeviceRef& device);
	bool init();

	Util::WeakPointer<Device> device;
	PipelineCacheHandle handle;
	std::unordered_map<size_t, Pipeline::Ref> cache;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_PIPELINE_CACHE_H_ */