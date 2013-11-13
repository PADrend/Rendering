/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "RenderingContext.h"
#include "CoreRenderingData.h"
#include "ParameterStructs.h"
#include "RenderingData.h"
#include "../BufferObject.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttribute.h"
#include "../Shader/Shader.h"
#include "../Shader/UniformRegistry.h"
#include "../Texture/Texture.h"
#include "../FBO.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <stdexcept>
#include <stack>

#ifdef WIN32
#include <GL/wglew.h>
#endif

namespace Rendering {

static void applyGL_Core(CoreRenderingData & target, const CoreRenderingData & actual, bool forced);
static void applyGL(RenderingData & target, const RenderingData & actual, bool forced);
static void updateSGUniforms(RenderingData & target, const RenderingData & actual, bool forced);

class RenderingContext::InternalState {
	public:
		RenderingData actualRenderingData;
		RenderingData openGLRenderingData;
		RenderingData * activeRenderingData;
		std::stack<RenderingData *> renderingDataStack;

		CoreRenderingData actualCoreRenderingData;
		CoreRenderingData appliedCoreRenderingData;

		void setActiveRenderingData(RenderingData * rd) {
			activeRenderingData = rd;
		}
		RenderingData * getActiveRenderingData() const {
			return activeRenderingData;
		}

		std::stack<AlphaTestParameters> alphaTestParameterStack;
		std::stack<BlendingParameters> blendingParameterStack;
		std::stack<ColorBufferParameters> colorBufferParameterStack;
		std::stack<CullFaceParameters> cullFaceParameterStack;
		std::stack<DepthBufferParameters> depthBufferParameterStack;
		std::stack<LightingParameters> lightingParameterStack;
		std::stack<LineParameters> lineParameterStack;
		std::stack<MaterialParameters> materialStack;
		std::stack<PointParameters> pointParameterStack;
		std::stack<PolygonModeParameters> polygonModeParameterStack;
		std::stack<PolygonOffsetParameters> polygonOffsetParameterStack;
		std::stack<ScissorParameters> scissorParametersStack;
		ScissorParameters currentScissorParameters;
		std::stack<StencilParameters> stencilParameterStack;

		std::stack<Util::Reference<FBO> > fboStack;
		Util::Reference<FBO> activeFBO;

		UniformRegistry globalUniforms;

		std::stack<Geometry::Matrix4x4> matrixStack;
		std::stack<Geometry::Matrix4x4> projectionMatrixStack;

		std::vector<Util::Reference<Texture>> activeTextures;
		std::vector<std::stack<Util::Reference<Texture>>> textureStack;

		typedef std::pair<Util::Reference<CountedBufferObject>,uint32_t> feedbackBufferStatus_t; // buffer->mode

		std::stack<feedbackBufferStatus_t> feedbackStack;
		feedbackBufferStatus_t activeFeedbackStatus;

		std::stack<uint32_t> activeClientStates;
		std::stack<uint32_t> activeTextureClientStates;
		std::stack<uint32_t> activeVertexAttributeBindings;
		
		Geometry::Rect_i currentViewport;
		std::stack<Geometry::Rect_i> viewportStack;

		Geometry::Rect_i windowClientArea;

		InternalState() : actualRenderingData(), openGLRenderingData(), activeRenderingData(nullptr),
			actualCoreRenderingData(), appliedCoreRenderingData(), globalUniforms(),
			currentViewport(0, 0, 0, 0) {
		}
};

RenderingContext::RenderingContext() :
	state(new InternalState), immediate(true), displayMeshFn() {

	resetDisplayMeshFn();

	state->setActiveRenderingData(&(state->openGLRenderingData));

	setBlending(BlendingParameters());
	setColorBuffer(ColorBufferParameters());
	// Initially enable the back-face culling
	setCullFace(CullFaceParameters::CULL_BACK);
	// Initially enable the depth test.
	setDepthBuffer(DepthBufferParameters(true, true, Comparison::LESS));
	// Initially enable the lighting.
	setLighting(LightingParameters(true));
	setLine(LineParameters());
	setPointParameters(PointParameters());
	setPolygonOffset(PolygonOffsetParameters());
	setStencil(StencilParameters());
}

RenderingContext::~RenderingContext() = default;

void RenderingContext::resetDisplayMeshFn() {
	using namespace std::placeholders;
	displayMeshFn = std::bind(&Rendering::Mesh::_display, _2, _1, _3, _4);
}

void RenderingContext::displayMesh(Mesh * mesh){
	displayMeshFn(*this, mesh,0,mesh->isUsingIndexData()? mesh->getIndexCount() : mesh->getVertexCount());
}

void RenderingContext::setImmediateMode(const bool enabled) {
	immediate = enabled;
	if (immediate)
		applyChanges();
}

/*!	(static) */
void RenderingContext::clearScreen(const Util::Color4f & color) {
	glClearColor(color.getR(), color.getG(), color.getB(), color.getA());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderingContext::clearScreenRect(const Geometry::Rect_i & rect, const Util::Color4f & color, bool _clearDepth) {
	pushAndSetScissor(ScissorParameters(rect));
	applyChanges();
	glClearColor(color.getR(), color.getG(), color.getB(), color.getA());
	glClear(GL_COLOR_BUFFER_BIT |( _clearDepth ? GL_DEPTH_BUFFER_BIT : 0));
	popScissor();
}

/*!	(static) */
void RenderingContext::initGLState() {
#ifdef LIB_GLEW
	glewExperimental = GL_TRUE; // Workaround: Needed for OpenGL core profile until GLEW will be fixed.
	const GLenum err = glewInit();
	if (GLEW_OK != err) {
		WARN(std::string("GLEW Error: ") + reinterpret_cast<const char *>(glewGetErrorString(err)));
	}

#ifdef LIB_GL
	glPixelStorei( GL_PACK_ALIGNMENT,1); // allow glReadPixel for all possible resolutions

	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	glBlendEquation(GL_FUNC_ADD);

	glActiveTexture(GL_TEXTURE0);

	// Do not use deprecated functions in a OpenGL core profile.
//	if(glewIsSupported("GL_ARB_compatibility")) {
		glEnable(GL_COLOR_MATERIAL);

		glShadeModel(GL_SMOOTH);

		// disable global ambient light
		GLfloat lmodel_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
		glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);

		glEnable(GL_NORMALIZE);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//	} else if(glewIsSupported("GL_VERSION_3_0") || glewIsSupported("GL_ARB_vertex_array_object")) {
//		// Workaround: Create a single vertex array object here.
//		// For the core profile of OpenGL 3.2 or higher this is required,
//		// because glVertexAttribPointer generates an GL_INVALID_OPERATION without it.
//		// In the future, vertex array objects should be integrated into the rendering system.
//		GLuint vertexArrayObject;
//		glGenVertexArrays(1, &vertexArrayObject);
//		glBindVertexArray(vertexArrayObject);
//	}

	// Enable the possibility to write gl_PointSize from the vertex shader.
	glEnable(GL_PROGRAM_POINT_SIZE);
#endif /* LIB_GL */
#endif /* LIB_GLEW */

#ifdef WIN32
	wglSwapIntervalEXT(false);
#endif /* WIN32 */
}

//! static
bool RenderingContext::useAMDAttrBugWorkaround(){
	static bool useWorkaround = false;
	static bool initialized = false;
	if(!initialized){
		initialized = true;
		const GLubyte * rendererStr = glGetString(GL_RENDERER);
		if(rendererStr!=nullptr){
			const std::string s(reinterpret_cast<const char*>(rendererStr));
			if(s.find("ATI")!=std::string::npos || s.find("AMD")!=std::string::npos){
				useWorkaround = true;
			}
		}
	}
	return useWorkaround;
}

void RenderingContext::flush() {
	glFlush();
}

void RenderingContext::finish() {
	glFinish();
}

// Applying changes ***************************************************************************

void RenderingContext::applyChanges(bool forced) {
	try {
		applyGL_Core(state->appliedCoreRenderingData, state->actualCoreRenderingData, forced);
		Shader * shader = state->getActiveRenderingData()->getShader();
		if (shader) {
			if (shader->usesClassicOpenGL())
				applyGL(state->openGLRenderingData, state->actualRenderingData, forced);

			if (shader->usesSGUniforms()) {
				updateSGUniforms(*shader->getRenderingData(), state->actualRenderingData, forced);
				if (immediate && getActiveShader() == shader) {
					shader->applyUniforms(false); // forced is false here, as this forced means to re-apply all uniforms
				}
			}

			// transfer updated global uniforms to the shader
			shader->_getUniformRegistry()->performGlobalSync(state->globalUniforms, false);

			// apply uniforms
			shader->applyUniforms(forced);
			GET_GL_ERROR();
		} else {
			applyGL(state->openGLRenderingData, state->actualRenderingData, forced);
		}
	} catch(const std::exception & e) {
		WARN(std::string("Problem detected while setting rendering state: ") + e.what());
	}
	GET_GL_ERROR();
}

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

static void applyGL_Core(CoreRenderingData & target, const CoreRenderingData & actual, bool forced) {
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
		glLineWidth(actual.getLineParameters().getWidth());
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
// 	if(glewIsSupported("GL_ARB_compatibility")) {
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
// 	}
#endif /* LIB_GL */

	// Lighting
	if(forced || target.lightingParametersChanged(actual)) {
#ifdef LIB_GL
// 		if(glewIsSupported("GL_ARB_compatibility")) {
			if(actual.getLightingParameters().isEnabled()) {
				glEnable(GL_LIGHTING);
			} else {
				glDisable(GL_LIGHTING);
			}
// 		}
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
}

static void applyGL(RenderingData & target, const RenderingData & actual, bool forced) {
#ifdef LIB_GL

	bool cc = target.cameraInverseMatrixChanged(actual);
	if (forced || cc) {
		target.updateCameraMatrix(actual);
	}

	if (forced || target.projectionMatrixChanged(actual)) {
		glMatrixMode(GL_PROJECTION);
		glLoadTransposeMatrixf(actual.getProjectionMatrix().getData());
		glMatrixMode(GL_MODELVIEW);
		target.updateProjectionMatrix(actual);
	}

	if (forced || target.modelViewMatrixChanged(actual)) {
		glLoadTransposeMatrixf(actual.getModelViewMatrix().getData());
		target.updateModelViewMatrix(actual);
	}

	if (forced || target.materialChanged(actual)) {
		static const float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
		static const float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		static const float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};

		const MaterialParameters & materialParams = actual.getMaterialParameters();

		if (actual.isMaterialEnabled()) {
			if (materialParams.getColorMaterial()) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
				glEnable(GL_COLOR_MATERIAL);
				glColor4fv(materialParams.getDiffuse().data());
			} else {
				glDisable(GL_COLOR_MATERIAL);
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialParams.getAmbient().data());
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialParams.getDiffuse().data());
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialParams.getSpecular().data());
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, materialParams.getShininess());
			}
		} else {
			glEnable(GL_COLOR_MATERIAL);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
		}
		target.updateMaterial(actual);
	}

	//lights
	bool lc = target.lightsChanged(actual);
	if (lc || cc || forced) {

		const uint_fast8_t numEnabledLights = actual.getNumEnabledLights();

		target.updateLights(actual);

		for (uint_fast8_t i = 0; i < numEnabledLights; ++i)
		glEnable(GL_LIGHT0 + static_cast<GLenum>(i));

		for (uint_fast8_t i = numEnabledLights; i < RenderingData::MAX_LIGHTS; ++i)
		glDisable(GL_LIGHT0 + static_cast<GLenum>(i));

		glPushMatrix();
		glLoadTransposeMatrixf(actual.getCameraMatrix().getData());

		for (uint_fast8_t i = 0; i < numEnabledLights; ++i) {

			const GLenum lightNumber = GL_LIGHT0 + static_cast<GLenum>(i);
			const LightParameters & parameters = actual.getEnabledLight(i);

			glLightfv(lightNumber, GL_AMBIENT, parameters.ambient.data());
			glLightfv(lightNumber, GL_DIFFUSE, parameters.diffuse.data());
			glLightfv(lightNumber, GL_SPECULAR, parameters.specular.data());

			if (parameters.type == LightParameters::DIRECTIONAL) {
				glLightfv(lightNumber, GL_POSITION, (-parameters.direction).getVec());
				glLightf(lightNumber, GL_CONSTANT_ATTENUATION, 1.0f);
				glLightf(lightNumber, GL_LINEAR_ATTENUATION, 0.0f);
				glLightf(lightNumber, GL_QUADRATIC_ATTENUATION, 0.0f);
			} else {
				glLightfv(lightNumber, GL_POSITION, parameters.position.getVec());
				glLightf(lightNumber, GL_CONSTANT_ATTENUATION, parameters.constant);
				glLightf(lightNumber, GL_LINEAR_ATTENUATION, parameters.linear);
				glLightf(lightNumber, GL_QUADRATIC_ATTENUATION, parameters.quadratic);
			}

			if (parameters.type == LightParameters::SPOT) {
				glLightf(lightNumber, GL_SPOT_CUTOFF, parameters.cutoff);
				glLightfv(lightNumber, GL_SPOT_DIRECTION, parameters.direction.getVec());
				glLightf(lightNumber, GL_SPOT_EXPONENT, parameters.exponent);
			} else {
				glLightf(lightNumber, GL_SPOT_CUTOFF, 180.0f);
				glLightfv(lightNumber, GL_SPOT_DIRECTION, Geometry::Vec4f(0.0f, 0.0f, -1.0f, 0.0f).getVec());
				glLightf(lightNumber, GL_SPOT_EXPONENT, 0.0f);
			}
			target.updateLightParameter(i,parameters);
		}
		glPopMatrix();
	}

	// Point
	if(forced || target.pointParametersChanged(actual)) {
		glPointSize(actual.getPointParameters().getSize());
		if(actual.getPointParameters().isPointSmoothingEnabled()){
			glEnable(GL_POINT_SMOOTH);
		}else{
			glDisable(GL_POINT_SMOOTH);
		}
		target.setPointParameters(actual.getPointParameters());
	}
	GET_GL_ERROR();
	//! \note TextureUnits are always enabled directly for OpenGL, so they don't need to be handled here.
#endif /* LIB_GL */
}

typedef std::vector<Uniform::UniformName> UniformNameArray_t;
//! (internal)
static UniformNameArray_t createNames(const std::string & prefix, uint8_t number, const std::string &postfix) {
	UniformNameArray_t arr;
	for (uint8_t i = 0; i < number; ++i)
		arr.push_back(prefix + static_cast<char> ('0' + i) + postfix);
	return arr;
}

static const Uniform::UniformName UNIFORM_SG_MODEL_VIEW_MATRIX("sg_modelViewMatrix");
static const Uniform::UniformName UNIFORM_SG_PROJECTION_MATRIX("sg_projectionMatrix");
static const Uniform::UniformName UNIFORM_SG_MODEL_VIEW_PROJECTION_MATRIX("sg_modelViewProjectionMatrix");
static const Uniform::UniformName UNIFORM_SG_CAMERA_MATRIX("sg_cameraMatrix");
static const Uniform::UniformName UNIFORM_SG_CAMERA_INVERSE_MATRIX("sg_cameraInverseMatrix");
static const Uniform::UniformName UNIFORM_SG_LIGHT_COUNT("sg_lightCount");
static const Uniform::UniformName UNIFORM_SG_POINT_SIZE("sg_pointSize");

static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_POSITION(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].position"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_DIRECTION(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].direction"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_TYPE(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].type"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_CONSTANT(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].constant"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_LINEAR(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].linear"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_QUADRATIC(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].quadratic"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_AMBIENT(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].ambient"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_DIFFUSE(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].diffuse"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_SPECULAR(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].specular"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_EXPONENT(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].exponent"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_COSCUTOFF(createNames("sg_LightSource[", RenderingData::MAX_LIGHTS, "].cosCutoff"));

static const Uniform::UniformName UNIFORM_SG_VIEWPORT("sg_viewport");
static const Uniform::UniformName UNIFORM_SG_SCISSOR_RECT("sg_scissorRect");
static const Uniform::UniformName UNIFORM_SG_SCISSOR_ENABLED("sg_scissorEnabled");
static const Uniform::UniformName UNIFORM_SG_TEXTURE_ENABLED("sg_textureEnabled");
static const UniformNameArray_t UNIFORM_SG_TEXTURES(createNames("sg_texture", RenderingData::MAX_TEXTURES, ""));
static const Uniform::UniformName UNIFORM_SG_USE_MATERIALS("sg_useMaterials");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_AMBIENT("sg_Material.ambient");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_DIFFUSE("sg_Material.diffuse");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_SPECULAR("sg_Material.specular");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_SHININESS("sg_Material.shininess");

//! (static)
static void updateSGUniforms(RenderingData & target, const RenderingData & actual, bool forced) {

	Shader * shader = target.getShader();
	std::deque<Uniform> uniforms;

	// camera  & inverse
	bool cc = false;
	if (forced || target.cameraInverseMatrixChanged(actual)) {
		cc = true;
		target.updateCameraMatrix(actual);

		uniforms.emplace_back(UNIFORM_SG_CAMERA_MATRIX, actual.getCameraMatrix());
		uniforms.emplace_back(UNIFORM_SG_CAMERA_INVERSE_MATRIX, actual.getCameraInverseMatrix());
	}

	// lights
	if (forced || cc || target.lightsChanged(actual)) {

		target.updateLights(actual);

		uniforms.emplace_back(UNIFORM_SG_LIGHT_COUNT, static_cast<int> (actual.getNumEnabledLights()));

		const uint_fast8_t numEnabledLights = actual.getNumEnabledLights();
		for (uint_fast8_t i = 0; i < numEnabledLights; ++i) {
			const LightParameters & params = actual.getEnabledLight(i);

			target.updateLightParameter(i, params);

			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_POSITION[i], (actual.getCameraMatrix() * params.position).xyz());
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIRECTION[i], (actual.getCameraMatrix() * params.direction).xyz());
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_TYPE[i], static_cast<int> (params.type));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_CONSTANT[i], params.constant);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_LINEAR[i], params.linear);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_QUADRATIC[i], params.quadratic);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_AMBIENT[i], params.ambient);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIFFUSE[i], params.diffuse);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_SPECULAR[i], params.specular);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_EXPONENT[i], params.exponent);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_COSCUTOFF[i], params.cosCutoff);
		}

		if (forced) {
			LightParameters params;
			for (uint_fast8_t i = numEnabledLights; i < RenderingData::MAX_LIGHTS; ++i) {
				target.updateLightParameter(i, params);

				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_POSITION[i], (actual.getCameraMatrix() * params.position).xyz());
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIRECTION[i], (actual.getCameraMatrix() * params.direction).xyz());
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_TYPE[i], static_cast<int> (params.type));
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_CONSTANT[i], params.constant);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_LINEAR[i], params.linear);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_QUADRATIC[i], params.quadratic);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_AMBIENT[i], params.ambient);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIFFUSE[i], params.diffuse);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_SPECULAR[i], params.specular);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_EXPONENT[i], params.exponent);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_COSCUTOFF[i], params.cosCutoff);
			}
		}
	}

	// materials
	if (forced || target.materialChanged(actual)) {
		target.updateMaterial(actual);

		uniforms.emplace_back(UNIFORM_SG_USE_MATERIALS, actual.isMaterialEnabled());
		if (forced || actual.isMaterialEnabled()) {
			const MaterialParameters & material = actual.getMaterialParameters();
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_AMBIENT, material.getAmbient());
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_DIFFUSE, material.getDiffuse());
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_SPECULAR, material.getSpecular());
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_SHININESS, material.getShininess());
		}
	}

	// modelview & projection
	{
		bool pc = false;
		bool mc = false;

		if (forced || target.modelViewMatrixChanged(actual)) {
			mc = true;
			target.updateModelViewMatrix(actual);
			uniforms.emplace_back(UNIFORM_SG_MODEL_VIEW_MATRIX, actual.getModelViewMatrix());
		}

		if (forced || target.projectionMatrixChanged(actual)) {
			pc = true;
			target.updateProjectionMatrix(actual);
			uniforms.emplace_back(UNIFORM_SG_PROJECTION_MATRIX, actual.getProjectionMatrix());
		}
		if (forced || pc || mc) {
			uniforms.emplace_back(UNIFORM_SG_MODEL_VIEW_PROJECTION_MATRIX, actual.getProjectionMatrix() * actual.getModelViewMatrix());
		}
	}

	// Point
	if(forced || target.pointParametersChanged(actual)) {
		target.setPointParameters(actual.getPointParameters());
		uniforms.emplace_back(UNIFORM_SG_POINT_SIZE, actual.getPointParameters().getSize());
	}

	// TEXTURE UNITS
	if (forced || target.texturesChanged(actual)) {
		uniforms.emplace_back(UNIFORM_SG_TEXTURE_ENABLED, actual.getEnabledTextureUnits());
		for (uint_fast8_t i = 0; i < RenderingData::MAX_TEXTURES; ++i) {
			uniforms.emplace_back(UNIFORM_SG_TEXTURES[i], i);
		}
		target.updateTextureUnits(actual);
	}

	for(const auto & uniform : uniforms) {
		shader->_getUniformRegistry()->setUniform(uniform, false, forced);
	}
}

// BLENDING ************************************************************************************
const BlendingParameters & RenderingContext::getBlendingParameters() const {
	return state->actualCoreRenderingData.getBlendingParameters();
}

void RenderingContext::pushAndSetBlending(const BlendingParameters & p) {
	pushBlending();
	setBlending(p);
}
void RenderingContext::popBlending() {
	if (state->blendingParameterStack.empty()) {
		WARN("popBlending: Empty Blending-Stack");
		return;
	}
	setBlending(state->blendingParameterStack.top());
	state->blendingParameterStack.pop();
}

void RenderingContext::pushBlending() {
	state->blendingParameterStack.emplace(state->actualCoreRenderingData.getBlendingParameters());
}

void RenderingContext::setBlending(const BlendingParameters & p) {
	state->actualCoreRenderingData.setBlendingParameters(p);
	if (immediate)
		applyChanges();
}

// ColorBuffer ************************************************************************************
const ColorBufferParameters & RenderingContext::getColorBufferParameters() const {
	return state->actualCoreRenderingData.getColorBufferParameters();
}
void RenderingContext::popColorBuffer() {
	if(state->colorBufferParameterStack.empty()) {
		WARN("popColorBuffer: Empty ColorBuffer stack");
		return;
	}
	setColorBuffer(state->colorBufferParameterStack.top());
	state->colorBufferParameterStack.pop();
}

void RenderingContext::pushColorBuffer() {
	state->colorBufferParameterStack.emplace(state->actualCoreRenderingData.getColorBufferParameters());
}

void RenderingContext::pushAndSetColorBuffer(const ColorBufferParameters & p) {
	pushColorBuffer();
	setColorBuffer(p);
}

void RenderingContext::setColorBuffer(const ColorBufferParameters & p) {
	state->actualCoreRenderingData.setColorBufferParameters(p);
	if(immediate) {
		applyChanges();
	}
}

void RenderingContext::clearColor(const Util::Color4f & clearValue) {
	glClearColor(clearValue.getR(), clearValue.getG(), clearValue.getB(), clearValue.getA());
	glClear(GL_COLOR_BUFFER_BIT);
}

// CULL FACE ************************************************************************************
const CullFaceParameters & RenderingContext::getCullFaceParameters() const {
	return state->actualCoreRenderingData.getCullFaceParameters();
}
void RenderingContext::popCullFace() {
	if (state->cullFaceParameterStack.empty()) {
		WARN("popCullFace: Empty CullFace-Stack");
		return;
	}
	setCullFace(state->cullFaceParameterStack.top());
	state->cullFaceParameterStack.pop();
}

void RenderingContext::pushCullFace() {
	state->cullFaceParameterStack.emplace(state->actualCoreRenderingData.getCullFaceParameters());
}

void RenderingContext::pushAndSetCullFace(const CullFaceParameters & p) {
	pushCullFace();
	setCullFace(p);
}

void RenderingContext::setCullFace(const CullFaceParameters & p) {
	state->actualCoreRenderingData.setCullFaceParameters(p);
	if (immediate)
		applyChanges();
}

// DepthBuffer ************************************************************************************
const DepthBufferParameters & RenderingContext::getDepthBufferParameters() const {
	return state->actualCoreRenderingData.getDepthBufferParameters();
}
void RenderingContext::popDepthBuffer() {
	if(state->depthBufferParameterStack.empty()) {
		WARN("popDepthBuffer: Empty DepthBuffer stack");
		return;
	}
	setDepthBuffer(state->depthBufferParameterStack.top());
	state->depthBufferParameterStack.pop();
}

void RenderingContext::pushDepthBuffer() {
	state->depthBufferParameterStack.emplace(state->actualCoreRenderingData.getDepthBufferParameters());
}

void RenderingContext::pushAndSetDepthBuffer(const DepthBufferParameters & p) {
	pushDepthBuffer();
	setDepthBuffer(p);
}

void RenderingContext::setDepthBuffer(const DepthBufferParameters & p) {
	state->actualCoreRenderingData.setDepthBufferParameters(p);
	if(immediate) {
		applyChanges();
	}
}

void RenderingContext::clearDepth(float clearValue) {
#ifdef LIB_GLESv2
	glClearDepthf(clearValue);
#else
	glClearDepth(clearValue);
#endif
	glClear(GL_DEPTH_BUFFER_BIT);
}

// AlphaTest ************************************************************************************
const AlphaTestParameters & RenderingContext::getAlphaTestParameters() const {
	return state->actualCoreRenderingData.getAlphaTestParameters();
}
void RenderingContext::popAlphaTest() {
	if (state->alphaTestParameterStack.empty()) {
		WARN("popAlphaTest: Empty AlphaTest-Stack");
		return;
	}
	setAlphaTest(state->alphaTestParameterStack.top());
	state->alphaTestParameterStack.pop();
}

void RenderingContext::pushAlphaTest() {
	state->alphaTestParameterStack.emplace(state->actualCoreRenderingData.getAlphaTestParameters());
}

void RenderingContext::pushAndSetAlphaTest(const AlphaTestParameters & p) {
	pushAlphaTest();
	setAlphaTest(p);
}

void RenderingContext::setAlphaTest(const AlphaTestParameters & p) {
	state->actualCoreRenderingData.setAlphaTestParameters(p);
	if (immediate)
		applyChanges();
}

// Lighting ************************************************************************************
const LightingParameters & RenderingContext::getLightingParameters() const {
	return state->actualCoreRenderingData.getLightingParameters();
}
void RenderingContext::popLighting() {
	if(state->lightingParameterStack.empty()) {
		WARN("popLighting: Empty lighting stack");
		return;
	}
	setLighting(state->lightingParameterStack.top());
	state->lightingParameterStack.pop();
}

void RenderingContext::pushLighting() {
	state->lightingParameterStack.emplace(state->actualCoreRenderingData.getLightingParameters());
}

void RenderingContext::pushAndSetLighting(const LightingParameters & p) {
	pushLighting();
	setLighting(p);
}

void RenderingContext::setLighting(const LightingParameters & p) {
	state->actualCoreRenderingData.setLightingParameters(p);
	if(immediate) {
		applyChanges();
	}
}

// Line ************************************************************************************
const LineParameters& RenderingContext::getLineParameters() const {
	return state->actualCoreRenderingData.getLineParameters();
}

void RenderingContext::popLine() {
	if(state->lineParameterStack.empty()) {
		WARN("popLine: Empty line parameters stack");
		return;
	}
	setLine(state->lineParameterStack.top());
	state->lineParameterStack.pop();
}

void RenderingContext::pushLine() {
	state->lineParameterStack.emplace(state->actualCoreRenderingData.getLineParameters());
}

void RenderingContext::pushAndSetLine(const LineParameters & p) {
	pushLine();
	setLine(p);
}

void RenderingContext::setLine(const LineParameters & p) {
	state->actualCoreRenderingData.setLineParameters(p);
	if(immediate) {
		applyChanges();
	}
}

// Point ************************************************************************************
const PointParameters& RenderingContext::getPointParameters() const {
	return state->actualRenderingData.getPointParameters();
}

void RenderingContext::popPointParameters() {
	if(state->pointParameterStack.empty()) {
		WARN("popPoint: Empty point parameters stack");
		return;
	}
	setPointParameters(state->pointParameterStack.top());
	state->pointParameterStack.pop();
}

void RenderingContext::pushPointParameters() {
	state->pointParameterStack.emplace(state->actualRenderingData.getPointParameters());
}

void RenderingContext::pushAndSetPointParameters(const PointParameters & p) {
	pushPointParameters();
	setPointParameters(p);
}

void RenderingContext::setPointParameters(const PointParameters & p) {
	state->actualRenderingData.setPointParameters(p);
	if(immediate) {
		applyChanges();
	}
}
// PolygonMode ************************************************************************************
const PolygonModeParameters & RenderingContext::getPolygonModeParameters() const {
	return state->actualCoreRenderingData.getPolygonModeParameters();
}
void RenderingContext::popPolygonMode() {
	if (state->polygonModeParameterStack.empty()) {
		WARN("popPolygonMode: Empty PolygonMode-Stack");
		return;
	}
	setPolygonMode(state->polygonModeParameterStack.top());
	state->polygonModeParameterStack.pop();
}

void RenderingContext::pushPolygonMode() {
	state->polygonModeParameterStack.emplace(state->actualCoreRenderingData.getPolygonModeParameters());
}

void RenderingContext::pushAndSetPolygonMode(const PolygonModeParameters & p) {
	pushPolygonMode();
	setPolygonMode(p);
}

void RenderingContext::setPolygonMode(const PolygonModeParameters & p) {
	state->actualCoreRenderingData.setPolygonModeParameters(p);
	if (immediate)
		applyChanges();
}

// PolygonOffset ************************************************************************************
const PolygonOffsetParameters & RenderingContext::getPolygonOffsetParameters() const {
	return state->actualCoreRenderingData.getPolygonOffsetParameters();
}
void RenderingContext::popPolygonOffset() {
	if(state->polygonOffsetParameterStack.empty()) {
		WARN("popPolygonOffset: Empty PolygonOffset stack");
		return;
	}
	setPolygonOffset(state->polygonOffsetParameterStack.top());
	state->polygonOffsetParameterStack.pop();
}

void RenderingContext::pushPolygonOffset() {
	state->polygonOffsetParameterStack.emplace(state->actualCoreRenderingData.getPolygonOffsetParameters());
}

void RenderingContext::pushAndSetPolygonOffset(const PolygonOffsetParameters & p) {
	pushPolygonOffset();
	setPolygonOffset(p);
}

void RenderingContext::setPolygonOffset(const PolygonOffsetParameters & p) {
	state->actualCoreRenderingData.setPolygonOffsetParameters(p);
	if(immediate) {
		applyChanges();
	}
}

// Scissor ************************************************************************************

const ScissorParameters & RenderingContext::getScissor() const {
	return state->currentScissorParameters;
}
void RenderingContext::popScissor() {
	if (state->scissorParametersStack.empty()) {
		WARN("popScissor: Empty scissor parameters stack");
		return;
	}
	setScissor(state->scissorParametersStack.top());
	state->scissorParametersStack.pop();
}

void RenderingContext::pushScissor() {
	state->scissorParametersStack.emplace(getScissor());
}

void RenderingContext::pushAndSetScissor(const ScissorParameters & scissorParameters) {
	pushScissor();
	setScissor(scissorParameters);
}

void RenderingContext::setScissor(const ScissorParameters & scissorParameters) {
	state->currentScissorParameters = scissorParameters;

	if(state->currentScissorParameters.isEnabled()) {
		const Geometry::Rect_i & scissorRect = state->currentScissorParameters.getRect();
		glScissor(scissorRect.getX(), scissorRect.getY(), scissorRect.getWidth(), scissorRect.getHeight());
		glEnable(GL_SCISSOR_TEST);
		std::vector<int> sr;
		sr.push_back(scissorRect.getX());
		sr.push_back(scissorRect.getY());
		sr.push_back(scissorRect.getWidth());
		sr.push_back(scissorRect.getHeight());
		setGlobalUniform(Uniform(UNIFORM_SG_SCISSOR_RECT, sr));
		setGlobalUniform(Uniform(UNIFORM_SG_SCISSOR_ENABLED, true));
	} else {
		glDisable(GL_SCISSOR_TEST);
		setGlobalUniform(Uniform(UNIFORM_SG_SCISSOR_ENABLED, false));
	}
}


// Stencil ************************************************************************************
const StencilParameters & RenderingContext::getStencilParamters() const {
	return state->actualCoreRenderingData.getStencilParameters();
}

void RenderingContext::pushAndSetStencil(const StencilParameters & stencilParameter) {
	pushStencil();
	setStencil(stencilParameter);
}

void RenderingContext::popStencil() {
	if (state->stencilParameterStack.empty()) {
		WARN("popStencil: Empty stencil stack");
		return;
	}
	setStencil(state->stencilParameterStack.top());
	state->stencilParameterStack.pop();
}

void RenderingContext::pushStencil() {
	state->stencilParameterStack.emplace(state->actualCoreRenderingData.getStencilParameters());
}

void RenderingContext::setStencil(const StencilParameters & stencilParameter) {
	state->actualCoreRenderingData.setStencilParameters(stencilParameter);
	if (immediate) {
		applyChanges();
	}
}

void RenderingContext::clearStencil(int32_t clearValue) {
	glClearStencil(clearValue);
	glClear(GL_STENCIL_BUFFER_BIT);
}

// FBO ************************************************************************************

FBO * RenderingContext::getActiveFBO() const {
	return state->activeFBO.get();
}

void RenderingContext::popFBO() {
	if (state->fboStack.empty()) {
		WARN("popFBO: Empty FBO-Stack");
		return;
	}
	setFBO(state->fboStack.top().get());
	state->fboStack.pop();
}

void RenderingContext::pushFBO() {
	state->fboStack.emplace(getActiveFBO());
}

void RenderingContext::pushAndSetFBO(FBO * fbo) {
	pushFBO();
	setFBO(fbo);
}

void RenderingContext::setFBO(FBO * fbo) {
	FBO * lastActiveFBO = getActiveFBO();
	if (fbo == lastActiveFBO)
		return;
	if (fbo == nullptr) {
		FBO::_disable();
	} else {
		fbo->_enable();
	}
	state->activeFBO = fbo;
}

// GLOBAL UNIFORMS ***************************************************************************
void RenderingContext::setGlobalUniform(const Uniform & u) {
	state->globalUniforms.setUniform(u, false, false);
}
const Uniform & RenderingContext::getGlobalUniform(const Util::StringIdentifier & uniformName) {
	return state->globalUniforms.getUniform(uniformName);
}

// SHADER ************************************************************************************
void RenderingContext::setShader(Shader * shader) {
	if (shader) {
		if (shader->_enable()) {
			state->setActiveRenderingData(shader->getRenderingData());
			if (!state->getActiveRenderingData()->isInitialized()) { // this shader has not yet been initialized.
				applyChanges(true); // make sure that all uniforms are initially set (e.g. even for disabled lights)
				state->getActiveRenderingData()->markInitialized();
				//				std::cout << " !!!! FORCED !!! \n";
			}
		} else {
			WARN("RenderingContext::pushShader: can't enable shader, using OpenGL instead");
			state->setActiveRenderingData(&(state->openGLRenderingData));
			glUseProgram(0);
		}
	} else {
		state->setActiveRenderingData(&(state->openGLRenderingData));

		glUseProgram(0);
	}
	if (immediate)
		applyChanges();
}

void RenderingContext::pushShader() {
	state->renderingDataStack.emplace(state->getActiveRenderingData());
}

void RenderingContext::pushAndSetShader(Shader * shader) {
	pushShader();
	setShader(shader);
}

void RenderingContext::popShader() {
	if (state->renderingDataStack.empty()) {
		WARN("popShader: Empty Shader-Stack");
		return;
	}
	setShader(state->renderingDataStack.top()->getShader());
	state->setActiveRenderingData(state->renderingDataStack.top());
	state->renderingDataStack.pop();

	if (immediate)
		applyChanges();
}

bool RenderingContext::isShaderEnabled(Shader * shader) {
	return shader == state->getActiveRenderingData()->getShader();
}

Shader * RenderingContext::getActiveShader() {
	return state->getActiveRenderingData()->getShader();
}

const Shader * RenderingContext::getActiveShader() const {
	return state->getActiveRenderingData()->getShader();
}

void RenderingContext::_setUniformOnShader(Shader * shader, const Uniform & uniform, bool warnIfUnused, bool forced) {
	shader->_getUniformRegistry()->setUniform(uniform, warnIfUnused, forced);
	if (immediate && getActiveShader() == shader)
		shader->applyUniforms(false); // forced is false here, as this forced means to re-apply all uniforms
}

// TEXTURES **********************************************************************************

Texture * RenderingContext::getTexture(uint32_t unit) {
	return unit < state->activeTextures.size() ? state->activeTextures[unit].get() : nullptr;
}

void RenderingContext::pushTexture(uint32_t unit) {
	if (state->textureStack.size() < unit + 1)
		state->textureStack.resize(unit + 1);
	state->textureStack[unit].emplace(getTexture(unit));
}

void RenderingContext::pushAndSetTexture(uint32_t unit, Texture * texture) {
	pushTexture(unit);
	setTexture(unit, texture);
}

void RenderingContext::popTexture(uint32_t unit) {
	if (unit >= state->textureStack.size() || state->textureStack[unit].empty()) {
		WARN("popTexture: Empty Texture-Stack");
		return;
	}
	setTexture(unit, state->textureStack[unit].top().get());
	state->textureStack[unit].pop();
}

void RenderingContext::setTexture(uint32_t unit, Texture * texture) {
	if (state->activeTextures.size() < unit + 1)
		state->activeTextures.resize(unit + 1);

	Texture * oldTexture = getTexture(unit);
	if (texture == oldTexture)
		return;

	// todo: move enable / disable to RenderingData (?), but be warned: At some positions might be expected, that the texture is enabled immediately.
	glActiveTexture(GL_TEXTURE0 + unit);
	if(texture!=nullptr) {
		texture->_enable(*this);
		state->actualRenderingData.enableTextureUnit(unit);
	}
	else if (oldTexture != nullptr) {
		oldTexture->_disable();
		state->actualRenderingData.disableTextureUnit(unit);
	}
	
	state->activeTextures[unit] = texture;
	if (immediate)
		applyChanges();

}

// TRANSFORM FEEDBACK ************************************************************************

//! (static)
bool RenderingContext::isTransformFeedbackSupported(){
#if defined(GL_EXT_transform_feedback)
	static const bool support = isExtensionSupported("GL_EXT_transform_feedback");
	return support;
#else
	return false;
#endif
}

//! (static)
bool RenderingContext::requestTransformFeedbackSupport(){
	struct _{ static bool once() {
		if(RenderingContext::isTransformFeedbackSupported())
			return true;
		WARN("RenderingContext: TransformFeedback is not supported! (This warning is only shown once!)");
		return false;
	}};
	static const bool b = _::once();
	return b;
}

CountedBufferObject * RenderingContext::getActiveTransformFeedbackBuffer() const{
	return state->activeFeedbackStatus.first.get();
}

void RenderingContext::popTransformFeedbackBufferStatus(){
	if(state->feedbackStack.empty()) {
		WARN("popTransformFeedbackBufferStatus: The stack is empty.");
	}else{
		stopTransformFeedback();
		state->activeFeedbackStatus = state->feedbackStack.top();
		_startTransformFeedback(state->activeFeedbackStatus.second);
	}
}
void RenderingContext::pushTransformFeedbackBufferStatus(){
	state->feedbackStack.emplace(state->activeFeedbackStatus);
}
void RenderingContext::setTransformFeedbackBuffer(CountedBufferObject * buffer){
	if(requestTransformFeedbackSupport()){
		#if defined(LIB_GL) and defined(GL_EXT_transform_feedback)
		if(buffer!=nullptr){
			(*buffer)->bind(GL_TRANSFORM_FEEDBACK_BUFFER_EXT);
		}else{
			glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0);
		}
		#endif
	}
	state->activeFeedbackStatus.first = buffer;
	_startTransformFeedback(state->activeFeedbackStatus.second); // restart
}
void RenderingContext::_startTransformFeedback(uint32_t primitiveMode){
	if(requestTransformFeedbackSupport()){
		#if defined(LIB_GL) and defined(GL_EXT_transform_feedback)
		if(primitiveMode==0){
			glEndTransformFeedbackEXT();
		}else{
			glBeginTransformFeedbackEXT(static_cast<GLenum>(primitiveMode));
		}
		#endif // defined
	}
	state->activeFeedbackStatus.second = primitiveMode;
}
void RenderingContext::startTransformFeedback_lines()		{	_startTransformFeedback(GL_POINTS);	}
void RenderingContext::startTransformFeedback_points()		{	_startTransformFeedback(GL_LINES);	}
void RenderingContext::startTransformFeedback_triangles()	{	_startTransformFeedback(GL_TRIANGLES);	}
void RenderingContext::stopTransformFeedback()				{	_startTransformFeedback(0);	}

// LIGHTS ************************************************************************************

uint8_t RenderingContext::enableLight(const LightParameters & light) {
	if (state->actualRenderingData.getNumEnabledLights() >= RenderingData::MAX_LIGHTS) {
		WARN("Cannot enable more lights; ignoring call.");
		return 255;
	}
	const uint8_t lightNumber = state->actualRenderingData.enableLight(light);
	if (immediate) {
		applyChanges();
	}
	return lightNumber;
}

void RenderingContext::disableLight(uint8_t lightNumber) {
	if (!state->actualRenderingData.isLightEnabled(lightNumber)) {
		WARN("Cannot disable an already disabled light; ignoring call.");
		return;
	}
	state->actualRenderingData.disableLight(lightNumber);
	if (immediate) {
		applyChanges();
	}
}

// PROJECTION MATRIX *************************************************************************

void RenderingContext::popProjectionMatrix() {
	if (state->projectionMatrixStack.empty()) {
		WARN("Cannot pop projection matrix. The stack is empty.");
		return;
	}
	state->actualRenderingData.setProjectionMatrix(state->projectionMatrixStack.top());
	state->projectionMatrixStack.pop();
	if (immediate)
		applyChanges();
}

void RenderingContext::pushProjectionMatrix() {
	state->projectionMatrixStack.emplace(state->actualRenderingData.getProjectionMatrix());
}

void RenderingContext::setProjectionMatrix(const Geometry::Matrix4x4 & matrix) {
	state->actualRenderingData.setProjectionMatrix(matrix);
	if (immediate)
		applyChanges();
}

const Geometry::Matrix4x4 & RenderingContext::getProjectionMatrix() const {
	return state->actualRenderingData.getProjectionMatrix();
}

// CAMERA MATRIX *****************************************************************************

void RenderingContext::setInverseCameraMatrix(const Geometry::Matrix4x4 & matrix) {
	state->actualRenderingData.setCameraInverseMatrix(matrix);
	if (immediate)
		applyChanges();
}
const Geometry::Matrix4x4 & RenderingContext::getCameraMatrix() const {
	return state->actualRenderingData.getCameraMatrix();
}
const Geometry::Matrix4x4 & RenderingContext::getInverseCameraMatrix() const {
	return state->actualRenderingData.getCameraInverseMatrix();
}

// MODEL VIEW MATRIX *************************************************************************

void RenderingContext::resetMatrix() {
	state->actualRenderingData.setModelViewMatrix(state->actualRenderingData.getCameraMatrix());
	if (immediate)
		applyChanges();
}

const Geometry::Matrix4x4 & RenderingContext::getMatrix() const {
	return state->actualRenderingData.getModelViewMatrix();
}

void RenderingContext::pushMatrix() {
	state->matrixStack.emplace(state->actualRenderingData.getModelViewMatrix());
}

void RenderingContext::multMatrix(const Geometry::Matrix4x4 & matrix) {
	state->actualRenderingData.multModelViewMatrix(matrix);
	if (immediate)
		applyChanges();
}

void RenderingContext::setMatrix(const Geometry::Matrix4x4 & matrix) {
	state->actualRenderingData.setModelViewMatrix(matrix);
	if (immediate)
		applyChanges();
}

void RenderingContext::popMatrix() {
	if (state->matrixStack.empty()) {
		WARN("Cannot pop matrix. The stack is empty.");
		return;
	}
	state->actualRenderingData.setModelViewMatrix(state->matrixStack.top());
	state->matrixStack.pop();
	if (immediate)
		applyChanges();
}

// MATERIAL **********************************************************************************


const MaterialParameters & RenderingContext::getMaterial() const {
	return state->actualRenderingData.getMaterialParameters();
}

void RenderingContext::popMaterial() {
	if (state->materialStack.empty()) {
		WARN("RenderingContext.popMaterial: stack empty, ignoring call");
		FAIL();
		return;
	}
	state->materialStack.pop();
	if (state->materialStack.empty()) {
		state->actualRenderingData.disableMaterial();
	} else {
		state->actualRenderingData.setMaterial(state->materialStack.top());
	}
	if (immediate)
		applyChanges();
}

void RenderingContext::pushMaterial() {
	state->materialStack.emplace(state->actualRenderingData.getMaterialParameters());
}
void RenderingContext::pushAndSetMaterial(const MaterialParameters & material) {
	pushMaterial();
	setMaterial(material);
}
void RenderingContext::pushAndSetColorMaterial(const Util::Color4f & color) {
	MaterialParameters material;
	material.setAmbient(color);
	material.setDiffuse(color);
	material.setSpecular(Util::ColorLibrary::BLACK);
	material.enableColorMaterial();
	pushAndSetMaterial(material);
}
void RenderingContext::setMaterial(const MaterialParameters & material) {
	state->actualRenderingData.setMaterial(material);
	if (immediate)
		applyChanges();
}

// VIEWPORT **********************************************************************************

const Geometry::Rect_i & RenderingContext::getWindowClientArea() const {
	return state->windowClientArea;
}

const Geometry::Rect_i & RenderingContext::getViewport() const {
	return state->currentViewport;
}
void RenderingContext::popViewport() {
	if (state->viewportStack.empty()) {
		WARN("Cannot pop viewport stack because it is empty. Ignoring call.");
		return;
	}
	setViewport(state->viewportStack.top());
	state->viewportStack.pop();
}
void RenderingContext::pushViewport() {
	state->viewportStack.emplace(state->currentViewport);
}
void RenderingContext::setViewport(const Geometry::Rect_i & viewport) {
	state->currentViewport = viewport;
	glViewport(state->currentViewport.getX(), state->currentViewport.getY(), state->currentViewport.getWidth(), state->currentViewport.getHeight());

	std::vector<int> vp;
	vp.push_back(state->currentViewport.getX());
	vp.push_back(state->currentViewport.getY());
	vp.push_back(state->currentViewport.getWidth());
	vp.push_back(state->currentViewport.getHeight());
	setGlobalUniform(Uniform(UNIFORM_SG_VIEWPORT, vp));
}
void RenderingContext::setWindowClientArea(const Geometry::Rect_i & clientArea) {
	state->windowClientArea = clientArea;
}

// VBO Client States **********************************************************************************

void RenderingContext::enableClientState(uint32_t clientState) {
	state->activeClientStates.emplace(clientState);
#ifdef LIB_GL
	glEnableClientState(clientState);
#endif /* LIB_GL */
}

void RenderingContext::disableAllClientStates() {
	while (!state->activeClientStates.empty()) {
#ifdef LIB_GL
		glDisableClientState(state->activeClientStates.top());
#endif /* LIB_GL */
		state->activeClientStates.pop();
	}
}

void RenderingContext::enableTextureClientState(uint32_t textureUnit) {
	state->activeTextureClientStates.emplace(textureUnit);
#ifdef LIB_GL
	glClientActiveTexture(textureUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif /* LIB_GL */
}

void RenderingContext::disableAllTextureClientStates() {
	while (!state->activeTextureClientStates.empty()) {
#ifdef LIB_GL
		glClientActiveTexture(state->activeTextureClientStates.top());
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif /* LIB_GL */
		state->activeTextureClientStates.pop();
	}
}

void RenderingContext::enableVertexAttribArray(const VertexAttribute & attr, const uint8_t * data, int32_t stride) {
	Shader * shader = getActiveShader();
	GLint location = shader->getVertexAttributeLocation(attr.getNameId());
	if (location != -1) {
		GLuint attribLocation = static_cast<GLuint> (location);
		state->activeVertexAttributeBindings.emplace(attribLocation);
		glVertexAttribPointer(attribLocation, attr.getNumValues(), attr.getDataType(), attr.getDataType() != GL_FLOAT, stride, data + attr.getOffset());
		glEnableVertexAttribArray(attribLocation);
	}
}

void RenderingContext::disableAllVertexAttribArrays() {
	while (!state->activeVertexAttributeBindings.empty()) {
		glDisableVertexAttribArray(state->activeVertexAttributeBindings.top());
		state->activeVertexAttributeBindings.pop();
	}
}

}
