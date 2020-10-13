/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDS_QUERYCOMMANDS_H_
#define RENDERING_CORE_COMMANDS_QUERYCOMMANDS_H_

#include "Command.h"
#include "../QueryPool.h"

namespace Rendering {

//------------------------------------------

class ResetQueryCommand : public Command {
PROVIDES_TYPE_NAME(ResetQueryCommand)
public:
	enum Mode { Begin, End };
	ResetQueryCommand(const Query& query) : query(query) {}
	bool compile(CompileContext& context) override;
private:
	Query query;
	uint32_t count;
};

//------------------------------------------

class ResetQueryPoolCommand : public Command {
PROVIDES_TYPE_NAME(ResetQueryPoolCommand)
public:
	enum Mode { Begin, End };
	ResetQueryPoolCommand(const QueryPoolHandle& pool, uint32_t first, uint32_t count) : pool(pool), first(first), count(count) {}
	bool compile(CompileContext& context) override;
private:
	QueryPoolHandle pool;
	uint32_t first;
	uint32_t count;
};

//------------------------------------------

class QueryCommand : public Command {
PROVIDES_TYPE_NAME(QueryCommand)
public:
	enum Mode { Begin, End };
	QueryCommand(Mode mode, const Query& query) : mode(mode), query(query) {}
	bool compile(CompileContext& context) override;
private:
	Mode mode;
	Query query;
};

//------------------------------------------

class TimeElapsedQueryCommand : public Command {
PROVIDES_TYPE_NAME(TimeElapsedQueryCommand)
public:
	enum Mode { Begin, End, Timestamp };
	TimeElapsedQueryCommand(Mode mode, const Query& beginQuery, const Query& endQuery) : mode(mode), beginQuery(beginQuery), endQuery(endQuery) {}
	bool compile(CompileContext& context) override;
private:
	Mode mode;
	Query beginQuery;
	Query endQuery;
};

//------------------------------------------

} /* Rendering */
#endif /* end of include guard: RENDERING_CORE_COMMANDS_QUERYCOMMANDS_H_ */
