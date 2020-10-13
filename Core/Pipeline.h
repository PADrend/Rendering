/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_PIPELINE_H_
#define RENDERING_CORE_PIPELINE_H_

#include "Common.h"
#include "../RenderingContext/PipelineState.h"

#include <Util/ReferenceCounter.h>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class PipelineCache;
using PipelineCacheRef = Util::Reference<PipelineCache>;

//---------------

class Pipeline : public Util::ReferenceCounter<Pipeline> {
public:

	using Ref = Util::Reference<Pipeline>;
	Pipeline(Pipeline &&) = default;
	Pipeline(const Pipeline &) = delete;
	~Pipeline();

	void setState(const PipelineState& value) { state = value; }
	const PipelineState& getState() const { return state; }

	PipelineType getType() const { return type; }
	const PipelineHandle& getApiHandle() const { return handle; }

	size_t getHash() const { return hash; };
private:
	friend class PipelineCache;
	explicit Pipeline(PipelineType type, const PipelineState& state, const Ref& parent = nullptr);
	bool initGraphics(const PipelineCacheRef& cache);
	bool initCompute(const PipelineCacheRef& cache);
	bool init(const PipelineCacheRef& cache);
	const PipelineType type;
	PipelineState state;
	Ref parent;
	PipelineHandle handle;
	size_t hash = 0;
};

//---------------

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_PIPELINE_H_ */