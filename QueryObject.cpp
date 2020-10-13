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

#include <Util/Macros.h>
#include <algorithm>
#include <deque>

#include <vulkan/vulkan.hpp>

namespace Rendering {

class QueryCommand : public Command {
PROVIDES_TYPE_NAME(QueryCommand)
public:
	enum Mode {
		Begin, End
	};
	QueryCommand(Mode mode, const Query& query) : mode(mode), query(query) {}

	bool compile(CompileContext& context) override;
private:
	Mode mode;
	Query query;
};

//-------------

bool QueryCommand::compile(CompileContext& context) {
	vk::CommandBuffer vkCmd(context.cmd);
	vk::QueryPool pool(context.device->getQueryPool()->getPoolHandle(query));
	switch(mode) {
		case Mode::Begin:
			vkCmd.resetQueryPool(pool, query.id, 1);
			vkCmd.beginQuery(pool, query.id, {});
			break;
		case Mode::End:
			vkCmd.endQuery(pool, query.id);
			break;
	}
	return true;
}

//---------------------------------------

class TimeElapsedQueryCommand : public Command {
PROVIDES_TYPE_NAME(TimeElapsedQueryCommand)
public:
	enum Mode {
		Begin, End, Timestamp
	};
	TimeElapsedQueryCommand(Mode mode, const Query& query, const Query& endQuery) : mode(mode), query(query), endQuery(endQuery) {}

	bool compile(CompileContext& context) override;
private:
	Mode mode;
	Query query;
	Query endQuery;
};

//-------------

bool TimeElapsedQueryCommand::compile(CompileContext& context) {
	vk::CommandBuffer vkCmd(context.cmd);
	vk::QueryPool pool(context.device->getQueryPool()->getPoolHandle(query));
	switch(mode) {
		case Mode::Begin:
		case Mode::Timestamp:
			vkCmd.resetQueryPool(pool, query.id, 1);
			vkCmd.resetQueryPool(pool, endQuery.id, 1);
			vkCmd.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, pool, query.id);
			break;
		case Mode::End:
			vkCmd.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, pool, endQuery.id);
			break;
	}
	return true;
}

//---------------------------------------

QueryObject::QueryObject(QueryType _queryType) {
	query.type = _queryType;
	query = Device::getDefault()->getQueryPool()->request(query.type);
	if(query.type == QueryType::TimeElapsed)
		endQuery = Device::getDefault()->getQueryPool()->request(query.type);
}

QueryObject::~QueryObject() {
	if(isValid())
		query.pool->free(query);
}

bool QueryObject::isResultAvailable(RenderingContext& rc) const {
	if(!isValid())
		return false;
	auto pool = query.pool->getPoolHandle(query);
	vk::Device vkDevice(pool);
	vk::QueryPool vkPool(pool);
	uint32_t result;
	return vkDevice.getQueryPoolResults(vkPool, (query.type == QueryType::TimeElapsed) ? endQuery.id : query.id, 1, sizeof(result), &result, 0, {}) == vk::Result::eSuccess;
}

uint32_t QueryObject::getResult(RenderingContext& rc) const {
	if(!isValid())
		return 0;
	if(!isResultAvailable(rc))
		rc.flush();
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

uint64_t QueryObject::getResult64(RenderingContext& rc) const {
	if(!isValid())
		return 0;
	if(!isResultAvailable(rc))
		rc.flush();
	auto pool = rc.getDevice()->getQueryPool()->getPoolHandle(query);
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

void QueryObject::begin(RenderingContext& rc) const {
	WARN_AND_RETURN_IF(query.type == QueryType::Timestamp, "QueryObject: begin() is not allowed for Timestamp queries.",);
	auto cmd = rc.getCommandBuffer();
	cmd->endRenderPass();
	if(query.type == QueryType::TimeElapsed) {
		cmd->addCommand(new TimeElapsedQueryCommand(TimeElapsedQueryCommand::Begin, query, endQuery));
	} else {
		cmd->addCommand(new QueryCommand(QueryCommand::Begin, query));
	}
}

void QueryObject::end(RenderingContext& rc) const {
	WARN_AND_RETURN_IF(query.type == QueryType::Timestamp, "QueryObject: end() is not allowed for Timestamp queries.",);
	if(!isValid())
		return;
	auto cmd = rc.getCommandBuffer();
	cmd->endRenderPass();
	if(query.type == QueryType::TimeElapsed) {
		cmd->addCommand(new TimeElapsedQueryCommand(TimeElapsedQueryCommand::End, query, endQuery));
	} else {
		cmd->addCommand(new QueryCommand(QueryCommand::End, query));
	}
}

void QueryObject::queryCounter(RenderingContext& rc) const {
	WARN_AND_RETURN_IF(query.type != QueryType::Timestamp, "QueryObject: queryCounter() is only allowed for Timestamp queries.",);
	auto cmd = rc.getCommandBuffer();
	cmd->endRenderPass();
	cmd->addCommand(new TimeElapsedQueryCommand(TimeElapsedQueryCommand::Timestamp, query, endQuery));
}

}
