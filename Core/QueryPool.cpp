/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "QueryPool.h"
#include "Device.h"
#include "CommandBuffer.h"
#include "Commands/QueryCommands.h"

#include <vulkan/vulkan.hpp>

#include <numeric>

namespace Rendering {

//---------------

static vk::QueryType getVkQueryType(QueryType type) {
	switch(type) {
		case QueryType::Occlusion: return vk::QueryType::eOcclusion;
		case QueryType::TimeElapsed:
		case QueryType::Timestamp: return vk::QueryType::eTimestamp;
		default: return vk::QueryType::ePipelineStatistics;
	}
}

//---------------

static vk::QueryPipelineStatisticFlags getVkStatisticsFlags(QueryType type) {
	switch(type) {
		case QueryType::InputAssemblyVertices: return vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices;
		case QueryType::InputAssemblyPrimitives: return vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives;
		case QueryType::VertexShaderInvocations: return vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations;
		case QueryType::GeometryShaderInvocations: return vk::QueryPipelineStatisticFlagBits::eGeometryShaderInvocations;
		case QueryType::GeometryShaderPrimitives: return vk::QueryPipelineStatisticFlagBits::eGeometryShaderPrimitives;
		case QueryType::ClippingInvocations: return vk::QueryPipelineStatisticFlagBits::eClippingInvocations;
		case QueryType::ClippingPrimitives: return vk::QueryPipelineStatisticFlagBits::eClippingPrimitives;
		case QueryType::FragmentShaderInvocations: return vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations;
		case QueryType::TessellationControlShaderPatches: return vk::QueryPipelineStatisticFlagBits::eTessellationControlShaderPatches;
		case QueryType::TessellationEvaluationShaderInvocations: return vk::QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations;
		case QueryType::ComputeShaderInvocations: return vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations;
		default: return {};
	}
}

//---------------

QueryPool::Ref QueryPool::create(const DeviceRef& device, uint32_t batchSize) {
	return new QueryPool(device, batchSize);
}

//---------------

QueryPool::~QueryPool() = default;

//---------------

QueryPool::QueryPool(const DeviceRef& device, uint32_t batchSize) : device(device.get()), batchSize(batchSize) { }

//---------------

Query QueryPool::request(QueryType type) {
	Query query;
	query.type = type;
	query.id = -1;
	query.pool = this;
	auto& pool = pools[type];
	for(uint32_t i=0; i<pool.size(); ++i) {
		if(!pool[i].freeIds.empty()) {
			query.poolId = i;
			query.id = static_cast<int32_t>(pool[i].freeIds.front());
			pool[i].freeIds.pop_front();
			return query;
		}
	}

	// Create new pool
	vk::Device vkDevice(device->getApiHandle());
	PoolEntry entry;
	entry.handle = QueryPoolHandle::create(vkDevice.createQueryPool({{},
		getVkQueryType(type), batchSize,
		getVkStatisticsFlags(type)
	}), vkDevice);

	if(entry.handle) {
		CommandBuffer::Ref cmds = CommandBuffer::create(device->getQueue(QueueFamily::Graphics));
		cmds->addCommand(new ResetQueryPoolCommand(entry.handle, 0, batchSize));
		cmds->submit();

		entry.freeIds.resize(batchSize);
		std::iota(entry.freeIds.begin(), entry.freeIds.end(), 0);
		query.id = entry.freeIds.front();
		entry.freeIds.pop_front();
		query.poolId = pools[type].size();
		pools[type].emplace_back(std::move(entry));
	}
	return query;
}

//---------------

void QueryPool::free(Query& query) {
	if(query.id < 0)
		return;
	PoolEntry& entry = pools[query.type][query.poolId];
	entry.freeIds.emplace_back(query.id);
	//if(pools[query.type].back().freeIds.size() >= batchSize) {
	//	pools[query.type].pop_back();
	//}
	query.id = -1;
	query.pool = nullptr;
}

//---------------

QueryPoolHandle QueryPool::getPoolHandle(Query query) const {
	if(query.id < 0)
		return nullptr;
	return pools.at(query.type).at(query.poolId).handle;
}

//---------------

} /* Rendering */