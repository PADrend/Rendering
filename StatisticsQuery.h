/*
	This file is part of the Rendering library.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STATISTICSQUERY_H
#define RENDERING_STATISTICSQUERY_H

#include "QueryObject.h"

namespace Rendering {
class RenderingContext;

/**
 * Wrapper class for pipeline statistics queries.
 *
 * @author Benjamin Eikel
 * @date 2016-01-08
 * @ingroup rendering_helper
 */
class StatisticsQuery : public QueryObject {
	private:
		StatisticsQuery(QueryType statisticsType);
	public:
		StatisticsQuery(StatisticsQuery &&) = default;

		/**
		 * Create a statistics query to count the number of vertices transferred to the GL.
		 */
		static StatisticsQuery createVerticesSubmittedQuery();

		/**
		 * Create a statistics query to count the number of primitives transferred to the GL.
		 */
		static StatisticsQuery createPrimitivesSubmittedQuery();

		/**
		 * Create a statistics query to count the number of times the vertex shader has been invoked.
		 */
		static StatisticsQuery createVertexShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of patches processed by the tessellation control shader stage.
		 */
		static StatisticsQuery createTessControlShaderPatchesQuery();

		/**
		 * Create a statistics query to count the number of times the tessellation evaluation shader has been invoked.
		 */
		static StatisticsQuery createTessEvaluationShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of times the geometry shader has been invoked.
		 */
		static StatisticsQuery createGeometryShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of primitives emitted by the geometry shader.
		 */
		static StatisticsQuery createGeometryShaderPrimitivesEmittedQuery();

		/**
		 * Create a statistics query to count the number of times the fragment shader has been invoked.
		 */
		static StatisticsQuery createFragmentShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of times the compute shader has been invoked.
		 */
		static StatisticsQuery createComputeShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of primitives that were processed in the primitive clipping stage.
		 */
		static StatisticsQuery createClippingInputPrimitivesQuery();

		/**
		 * Create a statistics query to count the number of primitives that were output by the primitive clipping stage 
		 * and are further processed by the rasterization stage.
		 */
		static StatisticsQuery createClippingOutputPrimitivesQuery();

		/**
		 * Create a statistics query to measure the time between GPU commands
		 */
		static StatisticsQuery createTimeElapsedQuery();

		/**
		 * Create a statistics query to measure the current time
		 */
		static StatisticsQuery createTimestampQuery();
};
}

#endif /* RENDERING_STATISTICSQUERY_H */
