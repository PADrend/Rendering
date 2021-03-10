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
#include "CoreRenderingStatus.h"
#include "../RenderingContext.h"
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

void apply(CoreRenderingStatus & target, const CoreRenderingStatus & actual, bool forced) {
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
		auto width = actual.getLineParameters().getWidth();
		glLineWidth(RenderingContext::getCompabilityMode() ? width : std::min(width, 1.0f));
		target.setLineParameters(actual.getLineParameters());
	}

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
 	if(RenderingContext::getCompabilityMode()) {
		// AlphaTest
		if(forced || target.alphaTestParametersChanged(actual)) {
			if(actual.getAlphaTestParameters().isEnabled()) {
				glDisable(GL_ALPHA_TEST);
			} else {
				glEnable(GL_ALPHA_TEST);
			}
			glAlphaFunc(Comparison::functionToGL(actual.getAlphaTestParameters().getMode()), actual.getAlphaTestParameters().getReferenceValue());
			target.setAlphaTestParameters(actual.getAlphaTestParameters());
		}
		GET_GL_ERROR();
 	}
#endif /* LIB_GL */

	// Lighting
	if(forced || target.lightingParametersChanged(actual)) {
#ifdef LIB_GL
 		if(RenderingContext::getCompabilityMode()) {
			if(actual.getLightingParameters().isEnabled()) {
				glEnable(GL_LIGHTING);
			} else {
				glDisable(GL_LIGHTING);
			}
 		}
#endif /* LIB_GL */
		target.setLightingParameters(actual.getLightingParameters());
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

		// PrimitiveRestart
	#ifdef LIB_GL
		if(forced || target.primitiveRestartParametersChanged(actual)) {
			if(actual.getPrimitiveRestartParameters().isEnabled()) {
				glEnable(GL_PRIMITIVE_RESTART);
				glPrimitiveRestartIndex(actual.getPrimitiveRestartParameters().getIndex());
			} else {
				glDisable(GL_PRIMITIVE_RESTART);
			}
			target.setPrimitiveRestartParameters(actual.getPrimitiveRestartParameters());
		}
		GET_GL_ERROR();
	#endif /* LIB_GL */

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
}

}
}
