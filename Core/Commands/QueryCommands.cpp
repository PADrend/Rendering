/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "QueryCommands.h"
#include "../Device.h"
#include "../internal/VkUtils.h"

#include <Util/Macros.h>

namespace Rendering {

//------------------------------------------

bool ResetQueryPoolCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!pool, "Cannot reset query pool. Invalid pool.", false);
	vk::CommandBuffer vkCmd(context.cmd);
	vkCmd.resetQueryPool(static_cast<vk::QueryPool>(pool), first, count);
	return true;
}

//------------------------------------------

bool ResetQueryCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(query.id<0 || !query.pool, "Cannot reset query. Invalid query.", false);
	vk::CommandBuffer vkCmd(context.cmd);
	vk::QueryPool pool(context.device->getQueryPool()->getPoolHandle(query));
	vkCmd.resetQueryPool(pool, query.id, 1);
	return true;
}

//------------------------------------------

bool QueryCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(query.id<0 || !query.pool, "Cannot create query. Invalid query.", false);
	vk::CommandBuffer vkCmd(context.cmd);
	vk::QueryPool pool(context.device->getQueryPool()->getPoolHandle(query));
	switch(mode) {
		case Mode::Begin:
			vkCmd.beginQuery(pool, query.id, {});
			break;
		case Mode::End:
			vkCmd.endQuery(pool, query.id);
			break;
	}
	return true;
}

//------------------------------------------

bool TimeElapsedQueryCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(beginQuery.id<0 || !beginQuery.pool, "Cannot create time elapsed query. Invalid query.", false);
	vk::CommandBuffer vkCmd(context.cmd);
	vk::QueryPool pool(context.device->getQueryPool()->getPoolHandle(beginQuery));
	switch(mode) {
		case Mode::Begin:
		case Mode::Timestamp:
			vkCmd.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, pool, beginQuery.id);
			break;
		case Mode::End:
			vkCmd.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, pool, endQuery.id);
			break;
	}
	return true;
}

} /* Rendering */