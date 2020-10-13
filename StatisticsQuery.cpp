/*
	This file is part of the Rendering library.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StatisticsQuery.h"
#include "Helper.h"
#include <cstdint>
#include <stdexcept>

namespace Rendering {

StatisticsQuery::StatisticsQuery(uint32_t statisticsType) : QueryObject(statisticsType) {
	if (!isExtensionSupported("GL_ARB_pipeline_statistics_query")) {
		throw std::runtime_error("Fatal error: OpenGL extension GL_ARB_pipeline_statistics_query is not supported.");
	}
}

StatisticsQuery StatisticsQuery::createVerticesSubmittedQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_VERTICES_SUBMITTED_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createPrimitivesSubmittedQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_PRIMITIVES_SUBMITTED_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createVertexShaderInvocationsQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createTessControlShaderPatchesQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_TESS_CONTROL_SHADER_PATCHES_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createTessEvaluationShaderInvocationsQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createGeometryShaderInvocationsQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_GEOMETRY_SHADER_INVOCATIONS);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createGeometryShaderPrimitivesEmittedQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createFragmentShaderInvocationsQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createComputeShaderInvocationsQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_COMPUTE_SHADER_INVOCATIONS_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createClippingInputPrimitivesQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_CLIPPING_INPUT_PRIMITIVES_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createClippingOutputPrimitivesQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_CLIPPING_OUTPUT_PRIMITIVES_ARB);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createTimeElapsedQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_TIME_ELAPSED);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

StatisticsQuery StatisticsQuery::createTimestampQuery() {
#if defined(LIB_GL)
	return StatisticsQuery(GL_TIMESTAMP);
#elif defined(LIB_GLESv2)
	return StatisticsQuery(0);
#endif
}

}
