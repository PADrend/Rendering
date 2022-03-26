/*
	This file is part of the Rendering library.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StatisticsQuery.h"
#include "Helper.h"
#include <cstdint>
#include <stdexcept>

namespace Rendering {

StatisticsQuery::StatisticsQuery(QueryType statisticsType) : QueryObject(statisticsType) { }

StatisticsQuery StatisticsQuery::createVerticesSubmittedQuery() {
	return StatisticsQuery(QueryType::InputAssemblyVertices);
}

StatisticsQuery StatisticsQuery::createPrimitivesSubmittedQuery() {
	return StatisticsQuery(QueryType::InputAssemblyPrimitives);
}

StatisticsQuery StatisticsQuery::createVertexShaderInvocationsQuery() {
	return StatisticsQuery(QueryType::VertexShaderInvocations);
}

StatisticsQuery StatisticsQuery::createTessControlShaderPatchesQuery() {
	return StatisticsQuery(QueryType::TessellationControlShaderPatches);
}

StatisticsQuery StatisticsQuery::createTessEvaluationShaderInvocationsQuery() {
	return StatisticsQuery(QueryType::TessellationEvaluationShaderInvocations);
}

StatisticsQuery StatisticsQuery::createGeometryShaderInvocationsQuery() {
	return StatisticsQuery(QueryType::GeometryShaderInvocations);
}

StatisticsQuery StatisticsQuery::createGeometryShaderPrimitivesEmittedQuery() {
	return StatisticsQuery(QueryType::GeometryShaderPrimitives);
}

StatisticsQuery StatisticsQuery::createFragmentShaderInvocationsQuery() {
	return StatisticsQuery(QueryType::FragmentShaderInvocations);
}

StatisticsQuery StatisticsQuery::createComputeShaderInvocationsQuery() {
	return StatisticsQuery(QueryType::ComputeShaderInvocations);
}

StatisticsQuery StatisticsQuery::createClippingInputPrimitivesQuery() {
	return StatisticsQuery(QueryType::ClippingInvocations);
}

StatisticsQuery StatisticsQuery::createClippingOutputPrimitivesQuery() {
	return StatisticsQuery(QueryType::ClippingPrimitives);
}

StatisticsQuery StatisticsQuery::createTimeElapsedQuery() {
	return StatisticsQuery(QueryType::TimeElapsed);
}

StatisticsQuery StatisticsQuery::createTimestampQuery() {
	return StatisticsQuery(QueryType::Timestamp);
}

}
