/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "QueryObject.h"
#include "Helper.h"
#include "Context/RenderingContext.h"
#include "Core/Device.h"
#include "Core/CommandBuffer.h"
#include "Core/Commands/QueryCommands.h"

#include <Util/Macros.h>
#include <algorithm>
#include <deque>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

//---------------------------------------

QueryObject::QueryObject(QueryType _queryType) : QueryObject(Device::getDefault(), _queryType) {}

//---------------------------------------

QueryObject::QueryObject(const DeviceRef& device, QueryType _queryType) {
	query.type = _queryType;
	query = device->getQueryPool()->request(query.type);
	if(query.type == QueryType::TimeElapsed)
		endQuery = device->getQueryPool()->request(query.type);
}

//------------------

QueryObject::~QueryObject() {
	if(isValid())
		query.pool->free(query);
}

//------------------

bool QueryObject::isResultAvailable() const {
	if(!isValid())
		return false;
	auto pool = query.pool->getPoolHandle(query);
	vk::Device vkDevice(pool);
	vk::QueryPool vkPool(pool);
	uint32_t result;
	return vkDevice.getQueryPoolResults(vkPool, (query.type == QueryType::TimeElapsed) ? endQuery.id : query.id, 1, sizeof(result), &result, 0, {}) == vk::Result::eSuccess;
}

//------------------

uint32_t QueryObject::getResult() const {
	WARN_AND_RETURN_IF(!isValid(), "QueryObject: getResult() Invalid query.", 0);
	auto pool = query.pool->getPoolHandle(query);
	vk::Device vkDevice(pool);
	vk::QueryPool vkPool(pool);
	vk::QueryResultFlags flags = vk::QueryResultFlagBits::eWait;
	uint32_t result;
	vkDevice.getQueryPoolResults(vkPool, query.id, 1, sizeof(uint32_t), &result, sizeof(uint32_t), flags);
	if(query.type == QueryType::TimeElapsed) {
		uint32_t endResult;
		vkDevice.getQueryPoolResults(vkPool, endQuery.id, 1, sizeof(uint32_t), &endResult, sizeof(uint32_t), flags);
		result = endResult - result;
	}
	return result;
}

//------------------

uint32_t QueryObject::getResult(RenderingContext& rc) const {
	if(!isResultAvailable())
		rc.flush();
	return getResult();
}

//------------------

uint64_t QueryObject::getResult64() const {
	WARN_AND_RETURN_IF(!isValid(), "QueryObject: getResult64() Invalid query.", 0);
	auto pool = query.pool->getPoolHandle(query);
	vk::Device vkDevice(pool);
	vk::QueryPool vkPool(pool);
	vk::QueryResultFlags flags = vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64;
	uint64_t result;
	vkDevice.getQueryPoolResults(vkPool, query.id, 1, sizeof(uint64_t), &result, sizeof(uint64_t), flags);
	if(query.type == QueryType::TimeElapsed) {
		uint64_t endResult;
		vkDevice.getQueryPoolResults(vkPool, endQuery.id, 1, sizeof(uint64_t), &endResult, sizeof(uint64_t), flags);
		result = endResult - result;
	}
	return result;
}

//------------------

uint64_t QueryObject::getResult64(RenderingContext& rc) const {
	if(!isResultAvailable())
		rc.flush();
	return getResult64();
}

//------------------

void QueryObject::begin(const CommandBufferRef& cmd) const {
	WARN_AND_RETURN_IF(query.type == QueryType::Timestamp, "QueryObject: begin() is not allowed for Timestamp queries.",);
	cmd->endRenderPass();
	if(query.type == QueryType::TimeElapsed) {
		cmd->addCommand(new TimeElapsedQueryCommand(TimeElapsedQueryCommand::Begin, query, endQuery));
	} else {
		cmd->addCommand(new QueryCommand(QueryCommand::Begin, query));
	}
}

//------------------

void QueryObject::begin(RenderingContext& rc) const {
	begin(rc.getCommandBuffer());
}

//------------------

void QueryObject::end(const CommandBufferRef& cmd) const {
	WARN_AND_RETURN_IF(query.type == QueryType::Timestamp, "QueryObject: end() is not allowed for Timestamp queries.",);
	if(!isValid())
		return;
	if(query.type == QueryType::TimeElapsed) {
		cmd->addCommand(new TimeElapsedQueryCommand(TimeElapsedQueryCommand::End, query, endQuery));
	} else {
		cmd->addCommand(new QueryCommand(QueryCommand::End, query));
	}
}

//------------------

void QueryObject::end(RenderingContext& rc) const {
	end(rc.getCommandBuffer());
}

//------------------

void QueryObject::reset(const CommandBufferRef& cmd) const {
	if(!isValid())
		return;
	cmd->endRenderPass();
	cmd->addCommand(new ResetQueryCommand(query));
	if(query.type == QueryType::TimeElapsed) {
		cmd->addCommand(new ResetQueryCommand(endQuery));
	}
}

//------------------

void QueryObject::reset(RenderingContext& rc) const {
	reset(rc.getCommandBuffer());
}

//------------------

void QueryObject::timestamp(const CommandBufferRef& cmd) const {
	WARN_AND_RETURN_IF(query.type != QueryType::Timestamp, "QueryObject: timestamp() is only allowed for Timestamp queries.",);
	cmd->addCommand(new TimeElapsedQueryCommand(TimeElapsedQueryCommand::Timestamp, query, endQuery));
}

//------------------

void QueryObject::timestamp(RenderingContext& rc) const {
	timestamp(rc.getCommandBuffer());
}

//------------------

}
