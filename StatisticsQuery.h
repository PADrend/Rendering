/*
	This file is part of the Rendering library.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
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
 * Wrapper class for OpenGL pipeline statistics queries.
 *
 * @see https://www.opengl.org/registry/specs/ARB/pipeline_statistics_query.txt
 * @author Benjamin Eikel
 * @date 2016-01-08
 * @ingroup rendering_helper
 */
class StatisticsQuery : public QueryObject {
	private:
		StatisticsQuery(uint32_t statisticsType);
	public:
		StatisticsQuery(StatisticsQuery &&) = default;

		/**
		 * Create a statistics query to count the number of vertices transferred to the GL.
		 * 
		 * @see @c GL_VERTICES_SUBMITTED_ARB
		 */
		static StatisticsQuery createVerticesSubmittedQuery();

		/**
		 * Create a statistics query to count the number of primitives transferred to the GL.
		 * 
		 * @see @c GL_PRIMITIVES_SUBMITTED_ARB
		 */
		static StatisticsQuery createPrimitivesSubmittedQuery();

		/**
		 * Create a statistics query to count the number of times the vertex shader has been invoked.
		 * 
		 * @see @c GL_VERTEX_SHADER_INVOCATIONS_ARB
		 */
		static StatisticsQuery createVertexShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of patches processed by the tessellation control shader stage.
		 * 
		 * @see @c GL_TESS_CONTROL_SHADER_PATCHES_ARB
		 */
		static StatisticsQuery createTessControlShaderPatchesQuery();

		/**
		 * Create a statistics query to count the number of times the tessellation evaluation shader has been invoked.
		 * 
		 * @see @c GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB
		 */
		static StatisticsQuery createTessEvaluationShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of times the geometry shader has been invoked.
		 * 
		 * @see @c GL_GEOMETRY_SHADER_INVOCATIONS
		 */
		static StatisticsQuery createGeometryShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of primitives emitted by the geometry shader.
		 * 
		 * @see @c GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB
		 */
		static StatisticsQuery createGeometryShaderPrimitivesEmittedQuery();

		/**
		 * Create a statistics query to count the number of times the fragment shader has been invoked.
		 * 
		 * @see @c GL_FRAGMENT_SHADER_INVOCATIONS_ARB
		 */
		static StatisticsQuery createFragmentShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of times the compute shader has been invoked.
		 * 
		 * @see @c GL_COMPUTE_SHADER_INVOCATIONS_ARB
		 */
		static StatisticsQuery createComputeShaderInvocationsQuery();

		/**
		 * Create a statistics query to count the number of primitives that were processed in the primitive clipping stage.
		 * 
		 * @see @c GL_CLIPPING_INPUT_PRIMITIVES_ARB
		 */
		static StatisticsQuery createClippingInputPrimitivesQuery();

		/**
		 * Create a statistics query to count the number of primitives that were output by the primitive clipping stage 
		 * and are further processed by the rasterization stage.
		 * 
		 * @see @c GL_CLIPPING_OUTPUT_PRIMITIVES_ARB
		 */
		static StatisticsQuery createClippingOutputPrimitivesQuery();

		/**
		 * Create a statistics query to measure the time between GPU commands
		 * 
		 * @see @c GL_TIME_ELAPSED
		 */
		static StatisticsQuery createTimeElapsedQuery();

		/**
		 * Create a statistics query to measure the current GL time
		 * 
		 * @see @c GL_TIME_ELAPSED
		 */
		static StatisticsQuery createTimestampQuery();
};
}

#endif /* RENDERING_STATISTICSQUERY_H */
