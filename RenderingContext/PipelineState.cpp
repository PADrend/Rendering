/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "PipelineState.h"
#include "../BufferObject.h"
#include "../GLHeader.h"
#include "../Helper.h"

#ifdef WIN32
#include <GL/wglew.h>
#endif

namespace Rendering {

static GLenum convertStencilAction(StencilParameters::action_t action) {
	switch(action) {
		case StencilParameters::KEEP:
			return GL_KEEP;
		case StencilParameters::ZERO:
			return GL_ZERO;
		case StencilParameters::REPLACE:
			return GL_REPLACE;
		case StencilParameters::INCR:
			return GL_INCR;
		case StencilParameters::INCR_WRAP:
			return GL_INCR_WRAP;
		case StencilParameters::DECR:
			return GL_DECR;
		case StencilParameters::DECR_WRAP:
			return GL_DECR_WRAP;
		case StencilParameters::INVERT:
			return GL_INVERT;
		default:
			break;
	}
	throw std::invalid_argument("Invalid StencilParameters::action_t enumerator");
}

PipelineState::StateDiff_t PipelineState::makeDiff(const PipelineState& target, bool forced) {
	StateDiff_t diff;
	
	if(forced) {
		diff.set();
		return diff;
	}
	
	diff.set(VIEWPORT_BIT, viewportChanged(target));
	diff.set(SCISSOR_BIT, scissorParametersChanged(target));
	diff.set(FBO_BIT, fboChanged(target));
	diff.set(PROGRAM_BIT, shaderChanged(target));
	diff.set(VERTEX_FORMAT_BIT, vertexFormatChanged(target));
	diff.set(VERTEX_BINDING_BIT, vertexBindingChanged(target));
	
	// Blending
	diff.set(BLEND_BIT, blendingParametersChanged(target));
	if(diff.test(BLEND_BIT)) {
		const BlendingParameters & actualParams = getBlendingParameters();
		const BlendingParameters & targetParams = target.getBlendingParameters();
		diff.set(BLEND_ENABLED_BIT, actualParams.isEnabled() != targetParams.isEnabled());
		diff.set(BLEND_FUNC_BIT, 	actualParams.getBlendFuncSrcRGB() != targetParams.getBlendFuncSrcRGB() ||
															actualParams.getBlendFuncDstRGB() != targetParams.getBlendFuncDstRGB() ||
															actualParams.getBlendFuncSrcAlpha() != targetParams.getBlendFuncSrcAlpha() ||
															actualParams.getBlendFuncDstAlpha() != targetParams.getBlendFuncDstAlpha());
		diff.set(BLEND_COLOR_BIT, actualParams.getBlendColor() != targetParams.getBlendColor());
		diff.set(BLEND_EQUATION_BIT, 	actualParams.getBlendEquationRGB() != targetParams.getBlendEquationRGB() ||
																	actualParams.getBlendEquationAlpha() != targetParams.getBlendEquationAlpha());
	}
	
	diff.set(COLOR_BUFFER_BIT, colorBufferParametersChanged(target));
	diff.set(CULL_FACE_BIT, cullFaceParametersChanged(target));
	diff.set(DEPTH_BUFFER_BIT, depthBufferParametersChanged(target));
	diff.set(LINE_PARAM_BIT, lineParametersChanged(target));
	diff.set(POLYGON_MODE_BIT, polygonModeParametersChanged(target));
	diff.set(POLYGON_OFFSET_BIT, polygonOffsetParametersChanged(target));
	
	// stencil
	diff.set(STENCIL_BIT, stencilParametersChanged(target));
	if(diff.test(STENCIL_BIT)) {
		const StencilParameters & actualParams = getStencilParameters();
		const StencilParameters & targetParams = target.getStencilParameters();
		diff.set(STENCIL_ENABLED_BIT, actualParams.isEnabled() != targetParams.isEnabled());
		diff.set(STENCIL_FUNC_BIT, actualParams.differentFunctionParameters(targetParams));
		diff.set(STENCIL_OP_BIT, actualParams.differentActionParameters(targetParams));
	}
	diff.set(TEXTURE_BINDING_BIT, texturesChanged(target));
	
	return diff;
}

void PipelineState::apply(const StateDiff_t& diff) {
	GET_GL_ERROR();
	
	// Shader
	if(diff.test(PROGRAM_BIT)) {
		glUseProgram(program);
		GET_GL_ERROR();
	}
	
	// Blending
	if(diff.test(BLEND_BIT)) {
		if(diff.test(BLEND_ENABLED_BIT)) {
			if(blendingParameters.isEnabled()) {
				glEnable(GL_BLEND);
			} else {
				glDisable(GL_BLEND);
			}
		}
		if(diff.test(BLEND_FUNC_BIT)) {
			glBlendFuncSeparate(BlendingParameters::functionToGL(blendingParameters.getBlendFuncSrcRGB()),
								BlendingParameters::functionToGL(blendingParameters.getBlendFuncDstRGB()),
								BlendingParameters::functionToGL(blendingParameters.getBlendFuncSrcAlpha()),
								BlendingParameters::functionToGL(blendingParameters.getBlendFuncDstAlpha()));
		}
		if(diff.test(BLEND_COLOR_BIT)) {
			glBlendColor(blendingParameters.getBlendColor().getR(),
						 blendingParameters.getBlendColor().getG(),
						 blendingParameters.getBlendColor().getB(),
						 blendingParameters.getBlendColor().getA());
		}
		if(diff.test(BLEND_EQUATION_BIT)) {
			glBlendEquationSeparate(BlendingParameters::equationToGL(blendingParameters.getBlendEquationRGB()),
									BlendingParameters::equationToGL(blendingParameters.getBlendEquationAlpha()));
		}
		GET_GL_ERROR();
	}

	// ColorBuffer
	if(diff.test(COLOR_BUFFER_BIT)) {
		glColorMask(
			colorBufferParameters.isRedWritingEnabled() ? GL_TRUE : GL_FALSE,
			colorBufferParameters.isGreenWritingEnabled() ? GL_TRUE : GL_FALSE,
			colorBufferParameters.isBlueWritingEnabled() ? GL_TRUE : GL_FALSE,
			colorBufferParameters.isAlphaWritingEnabled() ? GL_TRUE : GL_FALSE
		);
		GET_GL_ERROR();
	}

	// CullFace
	if(diff.test(CULL_FACE_BIT)) {
		if(cullFaceParameters.isEnabled()) {
			glEnable(GL_CULL_FACE);
		} else {
			glDisable(GL_CULL_FACE);
		}
		switch(cullFaceParameters.getMode()) {
			case CullFaceParameters::CULL_BACK:
				glCullFace(GL_BACK);
				break;
			case CullFaceParameters::CULL_FRONT:
				glCullFace(GL_FRONT);
				break;
			case CullFaceParameters::CULL_FRONT_AND_BACK:
				glCullFace(GL_FRONT_AND_BACK);
				break;
			default:
				throw std::invalid_argument("Invalid CullFaceParameters::cullFaceMode_t enumerator");
		}
		GET_GL_ERROR();
	}

	// DepthBuffer
	if(diff.test(DEPTH_BUFFER_BIT)) {
		if(depthBufferParameters.isTestEnabled()) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}
		if(depthBufferParameters.isWritingEnabled()) {
			glDepthMask(GL_TRUE);
		} else {
			glDepthMask(GL_FALSE);
		}
		glDepthFunc(Comparison::functionToGL(depthBufferParameters.getFunction()));
		GET_GL_ERROR();
	}

	// Line
	if(diff.test(LINE_PARAM_BIT)) {
		glLineWidth(std::min(lineParameters.getWidth(), 1.0f)); // deprecated for line width > 1
		GET_GL_ERROR();
	}

	// stencil
	if(diff.test(STENCIL_BIT)) {
		if(diff.test(STENCIL_ENABLED_BIT)) {
			if(stencilParameters.isEnabled()) {
				glEnable(GL_STENCIL_TEST);
			} else {
				glDisable(GL_STENCIL_TEST);
			}
		}
		if(diff.test(STENCIL_FUNC_BIT)) {
			glStencilFunc(Comparison::functionToGL(stencilParameters.getFunction()), stencilParameters.getReferenceValue(), stencilParameters.getBitMask().to_ulong());
		}
		if(diff.test(STENCIL_OP_BIT)) {
			glStencilOp(convertStencilAction(stencilParameters.getFailAction()),
						convertStencilAction(stencilParameters.getDepthTestFailAction()),
						convertStencilAction(stencilParameters.getDepthTestPassAction()));
		}
		GET_GL_ERROR();
	}

	// polygonMode
	if(diff.test(POLYGON_MODE_BIT)) {
		glPolygonMode(GL_FRONT_AND_BACK, PolygonModeParameters::modeToGL(polygonModeParameters.getMode()));
		GET_GL_ERROR();
	}

	// PolygonOffset
	if(diff.test(POLYGON_OFFSET_BIT)) {
		if(polygonOffsetParameters.isEnabled()) {
			glEnable(GL_POLYGON_OFFSET_FILL);
			glEnable(GL_POLYGON_OFFSET_LINE);
			glEnable(GL_POLYGON_OFFSET_POINT);
			glPolygonOffset(polygonOffsetParameters.getFactor(), polygonOffsetParameters.getUnits());
		} else {
			glDisable(GL_POLYGON_OFFSET_FILL);
			glDisable(GL_POLYGON_OFFSET_LINE);
			glDisable(GL_POLYGON_OFFSET_POINT);
		}
		GET_GL_ERROR();
	}

	// Textures
	if(diff.test(TEXTURE_BINDING_BIT)) {
		glBindTextures(0, MAX_TEXTURES, boundGLTextures.data());
		/*for(uint_fast8_t unit = 0; unit < MAX_TEXTURES; ++unit) {
			const auto & texture = getTexture(unit);			
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
			if( texture ) {
				glBindTexture(texture->getGLTextureType(), texture->getGLId());
				BufferObject* buffer = texture->getBufferObject();
				if(buffer)
					glTexBuffer( GL_TEXTURE_BUFFER, texture->getFormat().pixelFormat.glInternalFormat, buffer->getGLId() );
			} else if( oldTexture ) {
				glBindTexture(oldTexture->getGLTextureType(), 0);
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}*/
		GET_GL_ERROR();
	}
	
	// Viewport
	if(diff.test(VIEWPORT_BIT)) {
		glViewport(viewport.getX(), viewport.getY(), viewport.getWidth(), viewport.getHeight());
		GET_GL_ERROR();
	}
	
	// Scissor
	if(diff.test(SCISSOR_BIT)) {
		if(scissor.isEnabled()) {
			const Geometry::Rect_i & scissorRect = scissor.getRect();
			glScissor(scissorRect.getX(), scissorRect.getY(), scissorRect.getWidth(), scissorRect.getHeight());
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
		GET_GL_ERROR();
	}
		
	// FBO
	if(diff.test(FBO_BIT)) {
		if(fbo.isNull()) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			fbo->bind();
		}
		GET_GL_ERROR();
	}
		
	// Vertex Format
	if(diff.test(VERTEX_FORMAT_BIT)) {
		for(uint_fast8_t location=0; location<PipelineState::MAX_VERTEXATTRIBS; ++location) {
			const auto& format = getVertexFormat(location);
			const auto& attr = format.first;
			if(attr.empty()) {
				glDisableVertexAttribArray(location);
			} else {
				glEnableVertexAttribArray(location);				
				glVertexAttribBinding(location, format.second);		
				if(attr.getConvertToFloat()) 
					glVertexAttribFormat(location, attr.getNumValues(), attr.getDataType(), attr.getNormalize() ? GL_TRUE : GL_FALSE, attr.getOffset());
				else
					glVertexAttribIFormat(location, attr.getNumValues(), attr.getDataType(), attr.getOffset());
			}			
		}
		GET_GL_ERROR();
	}
	
	if(diff.test(VERTEX_BINDING_BIT)) {
		for(uint_fast8_t i = 0; i<PipelineState::MAX_VERTEXBINDINGS; ++i) {
			const auto& binding = getVertexBinding(i);
			uint32_t buffer, offset, stride, divisor;
			std::tie(buffer, offset, stride, divisor) = binding;
			
		  glVertexBindingDivisor(i, divisor);
			glBindVertexBuffer(i, buffer, offset, stride);
		}
		GET_GL_ERROR();
	}
}

}
