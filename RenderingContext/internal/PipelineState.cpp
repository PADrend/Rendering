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
#include "RenderingStatus.h"
#include "../../BufferObject.h"
#include "../../GLHeader.h"
#include "../../Helper.h"

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
  
void PipelineState::apply(const PipelineState& target) {
	GET_GL_ERROR();
	bool forced = !isValid();
	valid = true;
	
	// Shader
	if(forced || shaderChanged(target)) {
		if(debug) std::cout << "update shader " << target.program << std::endl;
		glUseProgram(target.program);
		updateShader(target.shader, target.program);
	}
	GET_GL_ERROR();
	
	// Blending
	if(forced || blendingParametersChanged(target)) {
		const BlendingParameters & targetParams = getBlendingParameters();
		const BlendingParameters & actualParams = target.getBlendingParameters();
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
		updateBlendingParameters(target);
	}
	GET_GL_ERROR();

	// ColorBuffer
	if(forced || colorBufferParametersChanged(target)) {
		if(debug) std::cout << "update colorbuffer" << std::endl;
		glColorMask(
			target.getColorBufferParameters().isRedWritingEnabled() ? GL_TRUE : GL_FALSE,
			target.getColorBufferParameters().isGreenWritingEnabled() ? GL_TRUE : GL_FALSE,
			target.getColorBufferParameters().isBlueWritingEnabled() ? GL_TRUE : GL_FALSE,
			target.getColorBufferParameters().isAlphaWritingEnabled() ? GL_TRUE : GL_FALSE
		);
		setColorBufferParameters(target.getColorBufferParameters());
	}
	GET_GL_ERROR();

	// CullFace
	if(forced || cullFaceParametersChanged(target)) {
		if(target.getCullFaceParameters().isEnabled()) {
			glEnable(GL_CULL_FACE);

		} else {
			glDisable(GL_CULL_FACE);
		}
		switch(target.getCullFaceParameters().getMode()) {
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
		setCullFaceParameters(target.getCullFaceParameters());
	}
	GET_GL_ERROR();

	// DepthBuffer
	if(forced || depthBufferParametersChanged(target)) {
		if(debug) std::cout << "update depth" << std::endl;
		if(target.getDepthBufferParameters().isTestEnabled()) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}
		if(target.getDepthBufferParameters().isWritingEnabled()) {
			glDepthMask(GL_TRUE);
		} else {
			glDepthMask(GL_FALSE);
		}
		glDepthFunc(Comparison::functionToGL(target.getDepthBufferParameters().getFunction()));
		setDepthBufferParameters(target.getDepthBufferParameters());
	}
	GET_GL_ERROR();

	// Line
	if(forced || lineParametersChanged(target)) {
		glLineWidth(std::min(target.getLineParameters().getWidth(), 1.0f)); // deprecated for line width > 1
		setLineParameters(target.getLineParameters());
	}
	GET_GL_ERROR();

	// stencil
	if (forced || stencilParametersChanged(target)) {
		const StencilParameters & targetParams = getStencilParameters();
		const StencilParameters & actualParams = target.getStencilParameters();
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
		updateStencilParameters(target);
	}
	GET_GL_ERROR();

#ifdef LIB_GL
	// polygonMode
	if(forced || polygonModeParametersChanged(target) ) {
		glPolygonMode(GL_FRONT_AND_BACK, PolygonModeParameters::modeToGL(target.getPolygonModeParameters().getMode()));
		setPolygonModeParameters(target.getPolygonModeParameters());
	}
	GET_GL_ERROR();
#endif /* LIB_GL */

	// PolygonOffset
	if(forced || polygonOffsetParametersChanged(target)) {
		if(target.getPolygonOffsetParameters().isEnabled()) {
			glEnable(GL_POLYGON_OFFSET_FILL);
#ifdef LIB_GL
			glEnable(GL_POLYGON_OFFSET_LINE);
			glEnable(GL_POLYGON_OFFSET_POINT);
#endif /* LIB_GL */
			glPolygonOffset(target.getPolygonOffsetParameters().getFactor(), target.getPolygonOffsetParameters().getUnits());
		} else {
			glDisable(GL_POLYGON_OFFSET_FILL);
#ifdef LIB_GL
			glDisable(GL_POLYGON_OFFSET_LINE);
			glDisable(GL_POLYGON_OFFSET_POINT);
#endif /* LIB_GL */
		}
		setPolygonOffsetParameters(target.getPolygonOffsetParameters());
	}
	GET_GL_ERROR();

	// Textures
	if(forced || texturesChanged(target)) {
		if(debug) std::cout << "update textures" << std::endl;
		for(uint_fast8_t unit = 0; unit < MAX_TEXTURES; ++unit) {
			const auto & texture = target.getTexture(unit);
			const auto & oldTexture = getTexture(unit);
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
		updateTextures(target);
	}
	GET_GL_ERROR();
	
	// Viewport
	if(forced || viewportChanged(target)) {
		if(debug) std::cout << "update viewport" << std::endl;
		glViewport(target.getViewport().getX(), target.getViewport().getY(), target.getViewport().getWidth(), target.getViewport().getHeight());
		setViewport(target.getViewport());
	}
	GET_GL_ERROR();	
	
	// Scissor
	if(forced || scissorParametersChanged(target)) {
		if(debug) std::cout << "update scissor" << std::endl;
		const auto& sp = target.getScissorParameters();
		if(sp.isEnabled()) {
			const Geometry::Rect_i & scissorRect = sp.getRect();
			glScissor(scissorRect.getX(), scissorRect.getY(), scissorRect.getWidth(), scissorRect.getHeight());
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
		setScissorParameters(sp);
	}
	GET_GL_ERROR();	
		
	// FBO
	if(forced || fboChanged(target)) {
		const auto& fbo = target.getFBO();
		if(fbo.isNull()) {
			if(debug) std::cout << "disable fbo" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			fbo->bind();
			if(debug) std::cout << "enable fbo " << fbo->getHandle() << std::endl;
		}
		setFBO(fbo);
	}
	GET_GL_ERROR();
		
	// Vertex Format
	if(forced || vertexFormatChanged(target)) {
		if(debug) std::cout << "update format" << std::endl;
		for(uint_fast8_t location=0; location<PipelineState::MAX_VERTEXATTRIBS; ++location) {
			const auto& format = target.getVertexFormat(location);
			const auto& oldFormat = getVertexFormat(location);
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
		updateVertexFormat(target);
	}
	GET_GL_ERROR();
	
	if(forced || vertexBindingChanged(target)) {
		if(debug) std::cout << "update binding" << std::endl;
		for(uint_fast8_t i = 0; i<PipelineState::MAX_VERTEXBINDINGS; ++i) {
			const auto& binding = target.getVertexBinding(i);
			const auto& oldBinding = getVertexBinding(i);
			if(forced || binding != oldBinding) {
				uint32_t buffer, offset, stride, divisor;
				std::tie(buffer, offset, stride, divisor) = binding;
				
			  glVertexBindingDivisor(i, divisor);
				glBindVertexBuffer(i, buffer, offset, stride);
			}
		}
		updateVertexBinding(target);
	}
	GET_GL_ERROR();
}

}
