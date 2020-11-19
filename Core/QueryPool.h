/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_QUERYPOOL_H_
#define RENDERING_CORE_QUERYPOOL_H_

#include "Common.h"
#include <Util/ReferenceCounter.h>

#include <map>
#include <vector>
#include <deque>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class QueryPool;
using QueryPoolRef = Util::Reference<QueryPool>;

enum class QueryType {
	Occlusion = 0,
	InputAssemblyVertices,
	InputAssemblyPrimitives,
	VertexShaderInvocations,
	GeometryShaderInvocations,
	GeometryShaderPrimitives,
	ClippingInvocations,
	ClippingPrimitives,
	FragmentShaderInvocations,
	TessellationControlShaderPatches,
	TessellationEvaluationShaderInvocations,
	ComputeShaderInvocations,
	Timestamp,
	TimeElapsed,
};

//---------------------------

struct Query {
	int32_t id = -1;
	uint32_t poolId = 0;
	QueryType type;
	QueryPoolRef pool;
};

//---------------------------

class QueryPool : public Util::ReferenceCounter<QueryPool> {
public:
	using Ref = Util::Reference<QueryPool>;
	RENDERINGAPI static Ref create(const DeviceRef& device, uint32_t batchSize);
	RENDERINGAPI ~QueryPool();
	QueryPool(QueryPool&& o) = default;
	QueryPool(const QueryPool& o) = delete;

	RENDERINGAPI Query request(QueryType type);
	RENDERINGAPI void free(Query& query);

	RENDERINGAPI QueryPoolHandle getPoolHandle(Query query) const;

private:
	QueryPool(const DeviceRef& device, uint32_t batchSize);
	Util::WeakPointer<Device> device;
	uint32_t batchSize;

	struct PoolEntry {
		QueryPoolHandle handle;
		std::deque<uint32_t> freeIds;
	};

	std::map<QueryType, std::vector<PoolEntry>> pools;
	
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_QUERYPOOL_H_ */