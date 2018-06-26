/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StatusHandler_glCore.h"
#include "StatusHandler_UBO.h"
#include "PipelineState.h"
#include "RenderingStatus.h"
#include "../../BufferObject.h"
#include "../../GLHeader.h"
#include "../../Helper.h"

#ifdef WIN32
#include <GL/wglew.h>
#endif

namespace Rendering {
namespace StatusHandler_glCore{

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

void apply(PipelineState & target, const PipelineState & actual, bool forced) {
	GET_GL_ERROR();
			
	// Shader
	bool shaderChanged = false;
	if(forced || target.shaderChanged(actual)) {
		const auto& shader = actual.getShader();
		if(shader.isNull()) {
			glUseProgram(0);
		} else if(shader->_enable()) {
			shader->applyUniforms(forced);
			shaderChanged = true;
		} else {
			WARN("Can't enable shader.");
			glUseProgram(0);
		}
		target.setShader(shader);
	}
	GET_GL_ERROR();
	
	// Blending
	if(forced || target.blendingParametersChanged(actual)) {
		const BlendingParameters & targetParams = target.getBlendingParameters();
		const BlendingParameters & actualParams = actual.getBlendingParameters();
		if(forced || targetParams.isEnabled() != actualParams.isEnabled()) {
			if(actualParams.isEnabled()) {
				glEnable(GL_BLEND);
			} else {
				glDisable(GL_BLEND);
			}
		}
		if(forced ||
				targetParams.getBlendFuncSrcRGB() != actualParams.getBlendFuncSrcRGB() ||
				targetParams.getBlendFuncDstRGB() != actualParams.getBlendFuncDstRGB() ||
				targetParams.getBlendFuncSrcAlpha() != actualParams.getBlendFuncSrcAlpha() ||
				targetParams.getBlendFuncDstAlpha() != actualParams.getBlendFuncDstAlpha()) {
			glBlendFuncSeparate(BlendingParameters::functionToGL(actualParams.getBlendFuncSrcRGB()),
								BlendingParameters::functionToGL(actualParams.getBlendFuncDstRGB()),
								BlendingParameters::functionToGL(actualParams.getBlendFuncSrcAlpha()),
								BlendingParameters::functionToGL(actualParams.getBlendFuncDstAlpha()));
		}
		if(forced || targetParams.getBlendColor() != actualParams.getBlendColor()) {
			glBlendColor(actualParams.getBlendColor().getR(),
						 actualParams.getBlendColor().getG(),
						 actualParams.getBlendColor().getB(),
						 actualParams.getBlendColor().getA());
		}
		if(forced ||
				targetParams.getBlendEquationRGB() != actualParams.getBlendEquationRGB() ||
				targetParams.getBlendEquationAlpha() != actualParams.getBlendEquationAlpha()) {
			glBlendEquationSeparate(BlendingParameters::equationToGL(actualParams.getBlendEquationRGB()),
									BlendingParameters::equationToGL(actualParams.getBlendEquationAlpha()));
		}
		target.updateBlendingParameters(actual);
	}
	GET_GL_ERROR();

	// ColorBuffer
	if(forced || target.colorBufferParametersChanged(actual)) {
		glColorMask(
			actual.getColorBufferParameters().isRedWritingEnabled() ? GL_TRUE : GL_FALSE,
			actual.getColorBufferParameters().isGreenWritingEnabled() ? GL_TRUE : GL_FALSE,
			actual.getColorBufferParameters().isBlueWritingEnabled() ? GL_TRUE : GL_FALSE,
			actual.getColorBufferParameters().isAlphaWritingEnabled() ? GL_TRUE : GL_FALSE
		);
		target.setColorBufferParameters(actual.getColorBufferParameters());
	}
	GET_GL_ERROR();

	// CullFace
	if(forced || target.cullFaceParametersChanged(actual)) {
		if(actual.getCullFaceParameters().isEnabled()) {
			glEnable(GL_CULL_FACE);

		} else {
			glDisable(GL_CULL_FACE);
		}
		switch(actual.getCullFaceParameters().getMode()) {
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
		target.setCullFaceParameters(actual.getCullFaceParameters());
	}
	GET_GL_ERROR();

	// DepthBuffer
	if(forced || target.depthBufferParametersChanged(actual)) {
		if(actual.getDepthBufferParameters().isTestEnabled()) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}
		if(actual.getDepthBufferParameters().isWritingEnabled()) {
			glDepthMask(GL_TRUE);
		} else {
			glDepthMask(GL_FALSE);
		}
		glDepthFunc(Comparison::functionToGL(actual.getDepthBufferParameters().getFunction()));
		target.setDepthBufferParameters(actual.getDepthBufferParameters());
	}
	GET_GL_ERROR();

	// Line
	if(forced || target.lineParametersChanged(actual)) {
		glLineWidth(std::min(actual.getLineParameters().getWidth(), 1.0f)); // deprecated for line width > 1
		target.setLineParameters(actual.getLineParameters());
	}
	GET_GL_ERROR();

	// stencil
	if (forced || target.stencilParametersChanged(actual)) {
		const StencilParameters & targetParams = target.getStencilParameters();
		const StencilParameters & actualParams = actual.getStencilParameters();
		if(forced || targetParams.isEnabled() != actualParams.isEnabled()) {
			if(actualParams.isEnabled()) {
				glEnable(GL_STENCIL_TEST);
			} else {
				glDisable(GL_STENCIL_TEST);
			}
		}
		if(forced || targetParams.differentFunctionParameters(actualParams)) {
			glStencilFunc(Comparison::functionToGL(actualParams.getFunction()), actualParams.getReferenceValue(), actualParams.getBitMask().to_ulong());
		}
		if(forced || targetParams.differentActionParameters(actualParams)) {
			glStencilOp(convertStencilAction(actualParams.getFailAction()),
						convertStencilAction(actualParams.getDepthTestFailAction()),
						convertStencilAction(actualParams.getDepthTestPassAction()));
		}
		target.updateStencilParameters(actual);
	}
	GET_GL_ERROR();

#ifdef LIB_GL
	// polygonMode
	if(forced || target.polygonModeParametersChanged(actual) ) {
		glPolygonMode(GL_FRONT_AND_BACK, PolygonModeParameters::modeToGL(actual.getPolygonModeParameters().getMode()));
		target.setPolygonModeParameters(actual.getPolygonModeParameters());
	}
	GET_GL_ERROR();
#endif /* LIB_GL */

	// PolygonOffset
	if(forced || target.polygonOffsetParametersChanged(actual)) {
		if(actual.getPolygonOffsetParameters().isEnabled()) {
			glEnable(GL_POLYGON_OFFSET_FILL);
#ifdef LIB_GL
			glEnable(GL_POLYGON_OFFSET_LINE);
			glEnable(GL_POLYGON_OFFSET_POINT);
#endif /* LIB_GL */
			glPolygonOffset(actual.getPolygonOffsetParameters().getFactor(), actual.getPolygonOffsetParameters().getUnits());
		} else {
			glDisable(GL_POLYGON_OFFSET_FILL);
#ifdef LIB_GL
			glDisable(GL_POLYGON_OFFSET_LINE);
			glDisable(GL_POLYGON_OFFSET_POINT);
#endif /* LIB_GL */
		}
		target.setPolygonOffsetParameters(actual.getPolygonOffsetParameters());
	}
	GET_GL_ERROR();

	// Textures
	if(forced || target.texturesChanged(actual)) {
		for(uint_fast8_t unit = 0; unit < MAX_TEXTURES; ++unit) {
			const auto & texture = actual.getTexture(unit);
			const auto & oldTexture = target.getTexture(unit);
			if(forced || texture != oldTexture) {
				glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
				if( texture ) {
					glBindTexture(texture->getGLTextureType(), texture->getGLId());
#if defined(LIB_GL)
					BufferObject* buffer = texture->getBufferObject();
					if(buffer)
						glTexBuffer( GL_TEXTURE_BUFFER, texture->getFormat().pixelFormat.glInternalFormat, buffer->getGLId() );
#endif
				} else if( oldTexture ) {
					glBindTexture(oldTexture->getGLTextureType(), 0);
				} else {
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			}
		}
		target.updateTextures(actual);
	}
	GET_GL_ERROR();
	
	// Viewport
	if(forced || target.viewportChanged(actual)) {
		glViewport(actual.getViewport().getX(), actual.getViewport().getY(), actual.getViewport().getWidth(), actual.getViewport().getHeight());
		target.setViewport(actual.getViewport());
	}
	GET_GL_ERROR();	
	
	// Scissor
	if(forced || target.scissorParametersChanged(actual)) {
		const auto& sp = actual.getScissorParameters();
		if(sp.isEnabled()) {
			const Geometry::Rect_i & scissorRect = sp.getRect();
			glScissor(scissorRect.getX(), scissorRect.getY(), scissorRect.getWidth(), scissorRect.getHeight());
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
		target.setScissorParameters(sp);
	}
	GET_GL_ERROR();	
		
	// FBO
	if(forced || target.fboChanged(actual)) {
		const auto& fbo = actual.getFBO();
		if(fbo.isNull()) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			fbo->_enable();
		}
		target.setFBO(fbo);
	}
	GET_GL_ERROR();
		
	// Vertex Format
	if(forced || target.vertexFormatChanged(actual)) {
		for(uint_fast8_t location=0; location<PipelineState::MAX_VERTEXATTRIBS; ++location) {
			const auto& format = actual.getVertexFormat(location);
			const auto& oldFormat = target.getVertexFormat(location);
			if(forced || format != oldFormat) {
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
		}
		target.updateVertexFormat(actual);
	}
	
	if(forced || target.vertexBindingChanged(actual)) {
		for(uint_fast8_t i = 0; i<PipelineState::MAX_VERTEXBINDINGS; ++i) {
			const auto& binding = actual.getVertexBinding(i);
			const auto& oldBinding = target.getVertexBinding(i);
			if(forced || binding != oldBinding) {
				uint32_t buffer, offset, stride, divisor;
				std::tie(buffer, offset, stride, divisor) = binding;
				
			  glVertexBindingDivisor(i, divisor);
				glBindVertexBuffer(i, buffer, offset, stride);
			}
		}
		target.updateVertexBinding(actual);
	}
	GET_GL_ERROR();
}

}
}
