/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "RenderingContext.h"

#include "internal/CoreRenderingStatus.h"
#include "internal/RenderingStatus.h"
#include "internal/StatusHandler_glCompatibility.h"
#include "internal/StatusHandler_glCore.h"
#include "internal/StatusHandler_sgUniforms.h"
#include "RenderingParameters.h"
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
#include <array>
#include <stdexcept>
#include <stack>

#ifdef WIN32
#include <GL/wglew.h>
#endif

namespace Rendering {


class RenderingContext::InternalData {
	public:
		RenderingStatus targetRenderingStatus;
		RenderingStatus openGLRenderingStatus;
		RenderingStatus * activeRenderingStatus;
		std::stack<RenderingStatus *> renderingDataStack;

		CoreRenderingStatus actualCoreRenderingStatus;
		CoreRenderingStatus appliedCoreRenderingStatus;

		void setActiveRenderingStatus(RenderingStatus * rd) {
			activeRenderingStatus = rd;
		}
		RenderingStatus * getActiveRenderingStatus() const {
			return activeRenderingStatus;
		}

		std::stack<AlphaTestParameters> alphaTestParameterStack;
		std::unordered_map<uint32_t,std::stack<Util::Reference<Texture>>> atomicCounterStacks; 

		std::stack<BlendingParameters> blendingParameterStack;
		std::stack<ColorBufferParameters> colorBufferParameterStack;
		std::stack<CullFaceParameters> cullFaceParameterStack;
		std::stack<DepthBufferParameters> depthBufferParameterStack;
		std::array<std::stack<ImageBindParameters>, MAX_BOUND_IMAGES> imageStacks; 
		std::array<ImageBindParameters, MAX_BOUND_IMAGES> boundImages;
		std::stack<LightingParameters> lightingParameterStack;
		std::stack<LineParameters> lineParameterStack;
		std::stack<MaterialParameters> materialStack;
		std::stack<PointParameters> pointParameterStack;
		std::stack<PolygonModeParameters> polygonModeParameterStack;
		std::stack<PolygonOffsetParameters> polygonOffsetParameterStack;
		std::stack<PrimitiveRestartParameters> primitiveRestartParameterStack;
		std::stack<ScissorParameters> scissorParametersStack;
		ScissorParameters currentScissorParameters;
		std::stack<StencilParameters> stencilParameterStack;
		
		std::array<std::stack<ClipPlaneParameters>, MAX_CLIP_PLANES> clipPlaneStacks;
		std::array<ClipPlaneParameters, MAX_CLIP_PLANES> activeClipPlanes;
				
		std::stack<Util::Reference<FBO> > fboStack;
		Util::Reference<FBO> activeFBO;

		UniformRegistry globalUniforms;

		std::stack<Geometry::Matrix4x4> matrixStack;
		std::stack<Geometry::Matrix4x4> projectionMatrixStack;

		std::array<std::stack<std::pair<Util::Reference<Texture>, TexUnitUsageParameter>>, MAX_TEXTURES> textureStacks;

		typedef std::pair<Util::Reference<CountedBufferObject>,uint32_t> feedbackBufferStatus_t; // buffer->mode

		std::stack<feedbackBufferStatus_t> feedbackStack;
		feedbackBufferStatus_t activeFeedbackStatus;

		std::stack<uint32_t> activeClientStates;
		std::stack<uint32_t> activeTextureClientStates;
		std::stack<uint32_t> activeVertexAttributeBindings;
		
		Geometry::Rect_i currentViewport;
		std::stack<Geometry::Rect_i> viewportStack;

		Geometry::Rect_i windowClientArea;
		
		InternalData() : targetRenderingStatus(), openGLRenderingStatus(), activeRenderingStatus(nullptr),
			actualCoreRenderingStatus(), appliedCoreRenderingStatus(), globalUniforms(), textureStacks(),
			currentViewport(0, 0, 0, 0) {
		}
};

RenderingContext::RenderingContext() :
	internalData(new InternalData), immediate(true), displayMeshFn() {

	resetDisplayMeshFn();

	internalData->setActiveRenderingStatus(&(internalData->openGLRenderingStatus));

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

bool RenderingContext::compabilityMode = true;

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
	if(immediate)
		applyChanges();
}

void RenderingContext::clearScreenRect(const Geometry::Rect_i & rect, const Util::Color4f & color, bool _clearDepth) {
	pushAndSetScissor(ScissorParameters(rect));
	applyChanges();
	glClearColor(color.getR(), color.getG(), color.getB(), color.getA());
	glClear(GL_COLOR_BUFFER_BIT |( _clearDepth ? GL_DEPTH_BUFFER_BIT : 0));
	popScissor();
}

// static helper ***************************************************************************

//!	(static) 
void RenderingContext::clearScreen(const Util::Color4f & color) {
	glClearColor(color.getR(), color.getG(), color.getB(), color.getA());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


//! (static)
void RenderingContext::initGLState() {
#ifdef LIB_GLEW
	glewExperimental = GL_TRUE; // Workaround: Needed for OpenGL core profile until GLEW will be fixed.
	const GLenum err = glewInit();
	if(GLEW_OK != err) {
		WARN(std::string("GLEW Error: ") + reinterpret_cast<const char *>(glewGetErrorString(err)));
	}

#ifdef LIB_GL
	glPixelStorei( GL_PACK_ALIGNMENT,1); // allow glReadPixel for all possible resolutions

	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	glBlendEquation(GL_FUNC_ADD);

	glActiveTexture(GL_TEXTURE0);

	// Do not use deprecated functions in a OpenGL core profile.
	if(glewIsSupported("GL_ARB_compatibility")) {
		compabilityMode = true;
		glEnable(GL_COLOR_MATERIAL);

		glShadeModel(GL_SMOOTH);

		// disable global ambient light
		GLfloat lmodel_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
		glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);

		glEnable(GL_NORMALIZE);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		
		glEnable(GL_POINT_SPRITE);
	} else if(glewIsSupported("GL_VERSION_3_0") || glewIsSupported("GL_ARB_vertex_array_object")) {
		compabilityMode = false;
		// Workaround: Create a single vertex array object here.
		// For the core profile of OpenGL 3.2 or higher this is required,
		// because glVertexAttribPointer generates an GL_INVALID_OPERATION without it.
		// In the future, vertex array objects should be integrated into the rendering system.
		GLuint vertexArrayObject;
		glGenVertexArrays(1, &vertexArrayObject);
		glBindVertexArray(vertexArrayObject);
	}

	// Enable the possibility to write gl_PointSize from the vertex shader.
	glEnable(GL_PROGRAM_POINT_SIZE);
	
	if( glewIsSupported("GL_ARB_seamless_cube_map") ) //! \see http://www.opengl.org/wiki/Cubemap_Texture#Seamless_cubemap
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif /* LIB_GL */
#endif /* LIB_GLEW */

#ifdef WIN32
	wglSwapIntervalEXT(false);
#endif /* WIN32 */
	GET_GL_ERROR();
}

static bool detectAMDGPU(){
	const GLubyte * rendererStr = glGetString(GL_RENDERER);
	if(rendererStr!=nullptr){
		const std::string s(reinterpret_cast<const char*>(rendererStr));
		if(s.find("ATI")!=std::string::npos || s.find("AMD")!=std::string::npos){
			return true;
		}
	}
	return false;
}

//! (static)
bool RenderingContext::useAMDAttrBugWorkaround(){
	static bool useWorkaround = detectAMDGPU();
	return useWorkaround;
}

//! (static)
void RenderingContext::flush() {
	glFlush();
}

//! (static)
void RenderingContext::finish() {
	glFinish();
}

void RenderingContext::barrier(uint32_t flags) {
	applyChanges();
	glMemoryBarrier(flags == 0 ? GL_ALL_BARRIER_BITS : static_cast<GLbitfield>(flags));
}
// Applying changes ***************************************************************************

void RenderingContext::applyChanges(bool forced) {
	try {
		StatusHandler_glCore::apply(internalData->appliedCoreRenderingStatus, internalData->actualCoreRenderingStatus, forced);
		Shader * shader = internalData->getActiveRenderingStatus()->getShader();
		if(shader) {
			if(shader->usesClassicOpenGL())
				StatusHandler_glCompatibility::apply(internalData->openGLRenderingStatus, internalData->targetRenderingStatus, forced);

			if(shader->usesSGUniforms()) {
				StatusHandler_sgUniforms::apply(*shader->getRenderingStatus(), internalData->targetRenderingStatus, forced);
				if(immediate && getActiveShader() == shader) {
					shader->applyUniforms(false); // forced is false here, as this forced means to re-apply all uniforms
				}
			}

			// transfer updated global uniforms to the shader
			shader->_getUniformRegistry()->performGlobalSync(internalData->globalUniforms, false);

			// apply uniforms
			shader->applyUniforms(forced);
			GET_GL_ERROR();
		} else if(compabilityMode) {
			StatusHandler_glCompatibility::apply(internalData->openGLRenderingStatus, internalData->targetRenderingStatus, forced);
		}
	} catch(const std::exception & e) {
		WARN(std::string("Problem detected while setting rendering internalData: ") + e.what());
	}
	GET_GL_ERROR();
}

// Atomic counters (extension ARB_shader_atomic_counters)  *****************************************************


//! (static)
bool RenderingContext::isAtomicCountersSupported(){
#if defined(GL_ARB_shader_atomic_counters)
	static const bool support = isExtensionSupported("GL_ARB_shader_atomic_counters");
	return support;
#else
	return false;
#endif
}
//! (static)
uint32_t RenderingContext::getMaxAtomicCounterBuffers(){
	static const uint32_t value = [](){
#if defined(GL_ARB_shader_atomic_counters)
		if(isAtomicCountersSupported()){
			GLint max;
			glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &max); 
			return static_cast<uint32_t>(max);
		}
#endif
		return static_cast<uint32_t>(0);
	}();
	return value;
}
//! (static)
uint32_t RenderingContext::getMaxAtomicCounterBufferSize(){
	static const uint32_t value = [](){
#if defined(GL_ARB_shader_atomic_counters)
		if(isAtomicCountersSupported()){
			GLint max;
			glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, &max); 
			return static_cast<uint32_t>(max);
		}
#endif
		return static_cast<uint32_t>(0);
	}();
	return value;
}

static void assertCorrectAtomicBufferIndex(uint32_t index){
	if(index>=RenderingContext::getMaxAtomicCounterBuffers()){ // GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS
		std::cout << "Error:"<<index << ">=" << RenderingContext::getMaxAtomicCounterBuffers() <<"\n";
		throw std::runtime_error("RenderingContext::assertCorrectAtomicBufferIndex: Invalid buffer index.");
	}
}

Texture* RenderingContext::getAtomicCounterTextureBuffer(uint32_t index)const{
	assertCorrectAtomicBufferIndex(index);
	auto& stack = internalData->atomicCounterStacks[index];
	return stack.empty() ? nullptr : stack.top().get();
}

void RenderingContext::pushAtomicCounterTextureBuffer(uint32_t index){
	assertCorrectAtomicBufferIndex(index);
	auto& stack = internalData->atomicCounterStacks[index];
	if(stack.empty()) stack.push(nullptr); // init stack
	stack.push( getAtomicCounterTextureBuffer(index) );
}

void RenderingContext::pushAndSetAtomicCounterTextureBuffer(uint32_t index, Texture* t){
	pushAtomicCounterTextureBuffer(index);
	setAtomicCounterTextureBuffer(index,t);
}
void RenderingContext::popAtomicCounterTextureBuffer(uint32_t index){
	assertCorrectAtomicBufferIndex(index);
	auto& stack = internalData->atomicCounterStacks[index];
	if(stack.size()<=1){
		WARN("popAtomicCounterTexture: Empty stack");
	}else{
		stack.pop();
		setAtomicCounterTextureBuffer(index,stack.top().get());
	}
}

//! \note the texture in iParam may be null to unbind
void RenderingContext::setAtomicCounterTextureBuffer(uint32_t index, Texture * texture){
	assertCorrectAtomicBufferIndex(index);
#if defined(GL_ARB_shader_image_load_store)
	if(isAtomicCountersSupported()){
		GET_GL_ERROR();
		if(texture){
			if(texture->getTextureType()!=TextureType::TEXTURE_BUFFER )
				throw std::invalid_argument("RenderingContext::setAtomicCounterTextureBuffer: texture is not of type TEXTURE_BUFFER.");

			const auto& pixelFormat = texture->getFormat().pixelFormat;
			if( pixelFormat.glInternalFormat!=GL_R32I && pixelFormat.glInternalFormat!=GL_R32UI )
				throw std::invalid_argument("RenderingContext::setAtomicCounterTextureBuffer: texture is not red 32bit integer.");
		
			if(texture->getWidth()>getMaxAtomicCounterBufferSize()){
				std::cout << texture->getWidth()<<">"<<getMaxAtomicCounterBufferSize()<<"\n";
				throw std::invalid_argument("RenderingContext::setAtomicCounterTextureBuffer: textureBuffer is too large.");
			}
			if(!texture->getLocalData()) // (workaround) buffer seems to contain invalid values if the memory is only allocated and not uploaded.
				texture->allocateLocalData();
			texture->_prepareForBinding(*this);
			BufferObject* bo = texture->getBufferObject();
			if( bo&& bo->isValid() ){
				glBindBufferBase( GL_ATOMIC_COUNTER_BUFFER,index,bo->getGLId());
			}else{
				WARN("RenderingContext::setAtomicCounterTexture: TextureBuffer is invalid.");
			}
			GET_GL_ERROR();
		}else{
			glBindBufferBase( GL_ATOMIC_COUNTER_BUFFER,index,0);
			GET_GL_ERROR();
		}
	}else{
		WARN("RenderingContext::setAtomicCounterTexture: GL_ARB_shader_image_load_store is not supported by your driver.");
	}
#endif 

	auto& stack = internalData->atomicCounterStacks[index];
	if(stack.empty())
		stack.push(texture);
	else
		stack.top() = texture;
}

// Blending ************************************************************************************
const BlendingParameters & RenderingContext::getBlendingParameters() const {
	return internalData->actualCoreRenderingStatus.getBlendingParameters();
}

void RenderingContext::pushAndSetBlending(const BlendingParameters & p) {
	pushBlending();
	setBlending(p);
}
void RenderingContext::popBlending() {
	if(internalData->blendingParameterStack.empty()) {
		WARN("popBlending: Empty Blending-Stack");
		return;
	}
	setBlending(internalData->blendingParameterStack.top());
	internalData->blendingParameterStack.pop();
}

void RenderingContext::pushBlending() {
	internalData->blendingParameterStack.emplace(internalData->actualCoreRenderingStatus.getBlendingParameters());
}

void RenderingContext::setBlending(const BlendingParameters & p) {
	internalData->actualCoreRenderingStatus.setBlendingParameters(p);
	if(immediate)
		applyChanges();
}


// ClipPlane ************************************************************************************

const ClipPlaneParameters & RenderingContext::getClipPlane(uint8_t index) const {
	static ClipPlaneParameters emptyClipPlane;
	if(index >= MAX_CLIP_PLANES) {
		WARN("getClipPlane: invalid plane index");
		return emptyClipPlane;
	}
	return internalData->activeClipPlanes.at(index);
}

void RenderingContext::popClipPlane(uint8_t index) {
	if(internalData->clipPlaneStacks.at(index).empty()) {
		WARN("popClipPlane: Empty ClipPlane-Stack");
		return;
	}
	setClipPlane(index, internalData->clipPlaneStacks.at(index).top());
	internalData->clipPlaneStacks.at(index).pop();
}

void RenderingContext::pushClipPlane(uint8_t index) {
	internalData->clipPlaneStacks.at(index).emplace(getClipPlane(index));
}

void RenderingContext::pushAndSetClipPlane(uint8_t index, const ClipPlaneParameters & planeParameters) {
	pushClipPlane(index);
	setClipPlane(index, planeParameters);
}

void RenderingContext::setClipPlane(uint8_t index, const ClipPlaneParameters & planeParameters) {	
	if(index >= MAX_CLIP_PLANES) {
		WARN("setClipPlane: invalid plane index");
		return;
	}
	if(internalData->activeClipPlanes.at(index) != planeParameters) {
		internalData->activeClipPlanes[index] = planeParameters;
		#if defined(LIB_GL)
			applyChanges();
			if(planeParameters.isEnabled()) {
				glEnable(GL_CLIP_PLANE0 + index);
				auto plane = planeParameters.getPlane();
				std::vector<double> planeEq = {plane.getNormal().x(), plane.getNormal().y(), plane.getNormal().z(), plane.getOffset()};
				glClipPlane(GL_CLIP_PLANE0 + index, planeEq.data());
			} else {
				glDisable(GL_CLIP_PLANE0 + index);
			}
		#endif
	}
}

// ColorBuffer ************************************************************************************
const ColorBufferParameters & RenderingContext::getColorBufferParameters() const {
	return internalData->actualCoreRenderingStatus.getColorBufferParameters();
}
void RenderingContext::popColorBuffer() {
	if(internalData->colorBufferParameterStack.empty()) {
		WARN("popColorBuffer: Empty ColorBuffer stack");
		return;
	}
	setColorBuffer(internalData->colorBufferParameterStack.top());
	internalData->colorBufferParameterStack.pop();
}

void RenderingContext::pushColorBuffer() {
	internalData->colorBufferParameterStack.emplace(internalData->actualCoreRenderingStatus.getColorBufferParameters());
}

void RenderingContext::pushAndSetColorBuffer(const ColorBufferParameters & p) {
	pushColorBuffer();
	setColorBuffer(p);
}

void RenderingContext::setColorBuffer(const ColorBufferParameters & p) {
	internalData->actualCoreRenderingStatus.setColorBufferParameters(p);
	if(immediate)
		applyChanges();
}

void RenderingContext::clearColor(const Util::Color4f & clearValue) {
	glClearColor(clearValue.getR(), clearValue.getG(), clearValue.getB(), clearValue.getA());
	glClear(GL_COLOR_BUFFER_BIT);
}

// Cull Face ************************************************************************************
const CullFaceParameters & RenderingContext::getCullFaceParameters() const {
	return internalData->actualCoreRenderingStatus.getCullFaceParameters();
}
void RenderingContext::popCullFace() {
	if(internalData->cullFaceParameterStack.empty()) {
		WARN("popCullFace: Empty CullFace-Stack");
		return;
	}
	setCullFace(internalData->cullFaceParameterStack.top());
	internalData->cullFaceParameterStack.pop();
}

void RenderingContext::pushCullFace() {
	internalData->cullFaceParameterStack.emplace(internalData->actualCoreRenderingStatus.getCullFaceParameters());
}

void RenderingContext::pushAndSetCullFace(const CullFaceParameters & p) {
	pushCullFace();
	setCullFace(p);
}

void RenderingContext::setCullFace(const CullFaceParameters & p) {
	internalData->actualCoreRenderingStatus.setCullFaceParameters(p);
	if(immediate)
		applyChanges();
}

// DepthBuffer ************************************************************************************
const DepthBufferParameters & RenderingContext::getDepthBufferParameters() const {
	return internalData->actualCoreRenderingStatus.getDepthBufferParameters();
}
void RenderingContext::popDepthBuffer() {
	if(internalData->depthBufferParameterStack.empty()) {
		WARN("popDepthBuffer: Empty DepthBuffer stack");
		return;
	}
	setDepthBuffer(internalData->depthBufferParameterStack.top());
	internalData->depthBufferParameterStack.pop();
}

void RenderingContext::pushDepthBuffer() {
	internalData->depthBufferParameterStack.emplace(internalData->actualCoreRenderingStatus.getDepthBufferParameters());
}

void RenderingContext::pushAndSetDepthBuffer(const DepthBufferParameters & p) {
	pushDepthBuffer();
	setDepthBuffer(p);
}

void RenderingContext::setDepthBuffer(const DepthBufferParameters & p) {
	internalData->actualCoreRenderingStatus.setDepthBufferParameters(p);
	if(immediate)
		applyChanges();
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
	return internalData->actualCoreRenderingStatus.getAlphaTestParameters();
}
void RenderingContext::popAlphaTest() {
	if(internalData->alphaTestParameterStack.empty()) {
		WARN("popAlphaTest: Empty AlphaTest-Stack");
		return;
	}
	setAlphaTest(internalData->alphaTestParameterStack.top());
	internalData->alphaTestParameterStack.pop();
}

void RenderingContext::pushAlphaTest() {
	internalData->alphaTestParameterStack.emplace(internalData->actualCoreRenderingStatus.getAlphaTestParameters());
}

void RenderingContext::pushAndSetAlphaTest(const AlphaTestParameters & p) {
	pushAlphaTest();
	setAlphaTest(p);
}

void RenderingContext::setAlphaTest(const AlphaTestParameters & p) {
	internalData->actualCoreRenderingStatus.setAlphaTestParameters(p);
	if(immediate)
		applyChanges();
}

// ImageBinding ************************************************************************************
//! (static)
bool RenderingContext::isImageBindingSupported(){
#if defined(GL_ARB_shader_image_load_store)
	static const bool support = isExtensionSupported("GL_ARB_shader_image_load_store");
	return support;
#else
	return false;
#endif
}

static void assertCorrectImageUnit(uint8_t unit){
	if(unit>=MAX_BOUND_IMAGES)
		throw std::runtime_error("RenderingContext: Invalid image unit.");
}

ImageBindParameters RenderingContext::getBoundImage(uint8_t unit)const{
	assertCorrectImageUnit(unit);
	return internalData->boundImages[unit];
}

void RenderingContext::pushBoundImage(uint8_t unit){
	assertCorrectImageUnit(unit);
	internalData->imageStacks[unit].push( internalData->boundImages[unit] );
}

void RenderingContext::pushAndSetBoundImage(uint8_t unit, const ImageBindParameters& iParam){
	pushBoundImage(unit);
	setBoundImage(unit,iParam);
}
void RenderingContext::popBoundImage(uint8_t unit){
	assertCorrectImageUnit(unit);
	auto& iStack = internalData->imageStacks[unit];
	if(iStack.empty()){
		WARN("popBoundImage: Empty stack");
	}else{
		setBoundImage(unit,iStack.top());
		iStack.pop();
	}
}

//! \note the texture in iParam may be null to unbind
void RenderingContext::setBoundImage(uint8_t unit, const ImageBindParameters& iParam){
	assertCorrectImageUnit(unit);
	internalData->boundImages[unit] = iParam;
#if defined(GL_ARB_shader_image_load_store)
	if(isImageBindingSupported()){
		GET_GL_ERROR();
		Texture* texture = iParam.getTexture();
		if(texture){
			GLenum access;
			if(!iParam.getReadOperations()){
				access =  GL_WRITE_ONLY;
			}else if(!iParam.getWriteOperations()){
				access =  GL_READ_ONLY;
			}else{
				access =  GL_READ_WRITE;
			}
			const auto& pixelFormat = texture->getFormat().pixelFormat;
			GLenum format = pixelFormat.glInternalFormat;
			// special case:the used internalFormat in TextureUtils is not applicable here
			if(pixelFormat.glLocalDataType==GL_BYTE || pixelFormat.glLocalDataType==GL_UNSIGNED_BYTE){
				if(pixelFormat.glInternalFormat==GL_RED){
					format = GL_R8;
				}else if(pixelFormat.glInternalFormat==GL_RG){
					format = GL_RG8;
				}else if(pixelFormat.glInternalFormat==GL_RGB){
					format = GL_RGB8; // not supported by opengl!
				}else if(pixelFormat.glInternalFormat==GL_RGBA){
					format = GL_RGBA8;
				}
			}
			GET_GL_ERROR();
			glBindImageTexture(unit,texture->_prepareForBinding(*this),
								iParam.getLevel(),iParam.getMultiLayer()? GL_TRUE : GL_FALSE,iParam.getLayer(), access,
								format);
			GET_GL_ERROR();
		}else{
			glBindImageTexture(unit,0,0,GL_FALSE,0, GL_READ_WRITE, GL_RGBA32F);
			GET_GL_ERROR();
		}
	}else{
		WARN("RenderingContext::setBoundImage: GL_ARB_shader_image_load_store is not supported by your driver.");
	}
#else
	WARN("RenderingContext::setBoundImage: GL_ARB_shader_image_load_store is not available for this executable.");
#endif 
}
	
// Lighting ************************************************************************************
const LightingParameters & RenderingContext::getLightingParameters() const {
	return internalData->actualCoreRenderingStatus.getLightingParameters();
}
void RenderingContext::popLighting() {
	if(internalData->lightingParameterStack.empty()) {
		WARN("popLighting: Empty lighting stack");
		return;
	}
	setLighting(internalData->lightingParameterStack.top());
	internalData->lightingParameterStack.pop();
}

void RenderingContext::pushLighting() {
	internalData->lightingParameterStack.emplace(internalData->actualCoreRenderingStatus.getLightingParameters());
}

void RenderingContext::pushAndSetLighting(const LightingParameters & p) {
	pushLighting();
	setLighting(p);
}

void RenderingContext::setLighting(const LightingParameters & p) {
	internalData->actualCoreRenderingStatus.setLightingParameters(p);
	if(immediate)
		applyChanges();
}

// Line ************************************************************************************
const LineParameters& RenderingContext::getLineParameters() const {
	return internalData->actualCoreRenderingStatus.getLineParameters();
}

void RenderingContext::popLine() {
	if(internalData->lineParameterStack.empty()) {
		WARN("popLine: Empty line parameters stack");
		return;
	}
	setLine(internalData->lineParameterStack.top());
	internalData->lineParameterStack.pop();
}

void RenderingContext::pushLine() {
	internalData->lineParameterStack.emplace(internalData->actualCoreRenderingStatus.getLineParameters());
}

void RenderingContext::pushAndSetLine(const LineParameters & p) {
	pushLine();
	setLine(p);
}

void RenderingContext::setLine(const LineParameters & p) {
	internalData->actualCoreRenderingStatus.setLineParameters(p);
	if(immediate)
		applyChanges();
}

// Point ************************************************************************************
const PointParameters& RenderingContext::getPointParameters() const {
	return internalData->targetRenderingStatus.getPointParameters();
}

void RenderingContext::popPointParameters() {
	if(internalData->pointParameterStack.empty()) {
		WARN("popPoint: Empty point parameters stack");
		return;
	}
	setPointParameters(internalData->pointParameterStack.top());
	internalData->pointParameterStack.pop();
}

void RenderingContext::pushPointParameters() {
	internalData->pointParameterStack.emplace(internalData->targetRenderingStatus.getPointParameters());
}

void RenderingContext::pushAndSetPointParameters(const PointParameters & p) {
	pushPointParameters();
	setPointParameters(p);
}

void RenderingContext::setPointParameters(const PointParameters & p) {
	internalData->targetRenderingStatus.setPointParameters(p);
	if(immediate)
		applyChanges();
}
// PolygonMode ************************************************************************************
const PolygonModeParameters & RenderingContext::getPolygonModeParameters() const {
	return internalData->actualCoreRenderingStatus.getPolygonModeParameters();
}
void RenderingContext::popPolygonMode() {
	if(internalData->polygonModeParameterStack.empty()) {
		WARN("popPolygonMode: Empty PolygonMode-Stack");
		return;
	}
	setPolygonMode(internalData->polygonModeParameterStack.top());
	internalData->polygonModeParameterStack.pop();
}

void RenderingContext::pushPolygonMode() {
	internalData->polygonModeParameterStack.emplace(internalData->actualCoreRenderingStatus.getPolygonModeParameters());
}

void RenderingContext::pushAndSetPolygonMode(const PolygonModeParameters & p) {
	pushPolygonMode();
	setPolygonMode(p);
}

void RenderingContext::setPolygonMode(const PolygonModeParameters & p) {
	internalData->actualCoreRenderingStatus.setPolygonModeParameters(p);
	if(immediate)
		applyChanges();
}

// PolygonOffset ************************************************************************************
const PolygonOffsetParameters & RenderingContext::getPolygonOffsetParameters() const {
	return internalData->actualCoreRenderingStatus.getPolygonOffsetParameters();
}
void RenderingContext::popPolygonOffset() {
	if(internalData->polygonOffsetParameterStack.empty()) {
		WARN("popPolygonOffset: Empty PolygonOffset stack");
		return;
	}
	setPolygonOffset(internalData->polygonOffsetParameterStack.top());
	internalData->polygonOffsetParameterStack.pop();
}

void RenderingContext::pushPolygonOffset() {
	internalData->polygonOffsetParameterStack.emplace(internalData->actualCoreRenderingStatus.getPolygonOffsetParameters());
}

void RenderingContext::pushAndSetPolygonOffset(const PolygonOffsetParameters & p) {
	pushPolygonOffset();
	setPolygonOffset(p);
}

void RenderingContext::setPolygonOffset(const PolygonOffsetParameters & p) {
	internalData->actualCoreRenderingStatus.setPolygonOffsetParameters(p);
	if(immediate)
		applyChanges();
}

// PolygonOffset ************************************************************************************
const PrimitiveRestartParameters & RenderingContext::getPrimitiveRestartParameters() const {
	return internalData->actualCoreRenderingStatus.getPrimitiveRestartParameters();
}
void RenderingContext::popPrimitiveRestart() {
	if(internalData->primitiveRestartParameterStack.empty()) {
		WARN("popPrimitiveRestart: Empty PrimitiveRestart stack");
		return;
	}
	setPrimitiveRestart(internalData->primitiveRestartParameterStack.top());
	internalData->primitiveRestartParameterStack.pop();
}

void RenderingContext::pushPrimitiveRestart() {
	internalData->primitiveRestartParameterStack.emplace(internalData->actualCoreRenderingStatus.getPrimitiveRestartParameters());
}

void RenderingContext::pushAndSetPrimitiveRestart(const PrimitiveRestartParameters & p) {
	pushPrimitiveRestart();
	setPrimitiveRestart(p);
}

void RenderingContext::setPrimitiveRestart(const PrimitiveRestartParameters & p) {
	internalData->actualCoreRenderingStatus.setPrimitiveRestartParameters(p);
	if(immediate)
		applyChanges();
}


// Scissor ************************************************************************************

const ScissorParameters & RenderingContext::getScissor() const {
	return internalData->currentScissorParameters;
}
void RenderingContext::popScissor() {
	if(internalData->scissorParametersStack.empty()) {
		WARN("popScissor: Empty scissor parameters stack");
		return;
	}
	setScissor(internalData->scissorParametersStack.top());
	internalData->scissorParametersStack.pop();
}

void RenderingContext::pushScissor() {
	internalData->scissorParametersStack.emplace(getScissor());
}

void RenderingContext::pushAndSetScissor(const ScissorParameters & scissorParameters) {
	pushScissor();
	setScissor(scissorParameters);
}

static const Uniform::UniformName UNIFORM_SG_SCISSOR_RECT("sg_scissorRect");
static const Uniform::UniformName UNIFORM_SG_SCISSOR_ENABLED("sg_scissorEnabled");

void RenderingContext::setScissor(const ScissorParameters & scissorParameters) {
	internalData->currentScissorParameters = scissorParameters;

	if(internalData->currentScissorParameters.isEnabled()) {
		const Geometry::Rect_i & scissorRect = internalData->currentScissorParameters.getRect();
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
	return internalData->actualCoreRenderingStatus.getStencilParameters();
}

void RenderingContext::pushAndSetStencil(const StencilParameters & stencilParameter) {
	pushStencil();
	setStencil(stencilParameter);
}

void RenderingContext::popStencil() {
	if(internalData->stencilParameterStack.empty()) {
		WARN("popStencil: Empty stencil stack");
		return;
	}
	setStencil(internalData->stencilParameterStack.top());
	internalData->stencilParameterStack.pop();
}

void RenderingContext::pushStencil() {
	internalData->stencilParameterStack.emplace(internalData->actualCoreRenderingStatus.getStencilParameters());
}

void RenderingContext::setStencil(const StencilParameters & stencilParameter) {
	internalData->actualCoreRenderingStatus.setStencilParameters(stencilParameter);
	if(immediate)
		applyChanges();
}

void RenderingContext::clearStencil(int32_t clearValue) {
	glClearStencil(clearValue);
	glClear(GL_STENCIL_BUFFER_BIT);
}

// FBO ************************************************************************************

FBO * RenderingContext::getActiveFBO() const {
	return internalData->activeFBO.get();
}

void RenderingContext::popFBO() {
	if(internalData->fboStack.empty()) {
		WARN("popFBO: Empty FBO-Stack");
		return;
	}
	setFBO(internalData->fboStack.top().get());
	internalData->fboStack.pop();
}

void RenderingContext::pushFBO() {
	internalData->fboStack.emplace(getActiveFBO());
}

void RenderingContext::pushAndSetFBO(FBO * fbo) {
	pushFBO();
	setFBO(fbo);
}

void RenderingContext::setFBO(FBO * fbo) {
	FBO * lastActiveFBO = getActiveFBO();
	if(fbo == lastActiveFBO)
		return;
	if(fbo == nullptr) {
		FBO::_disable();
	} else {
		fbo->_enable();
	}
	internalData->activeFBO = fbo;
}

// GLOBAL UNIFORMS ***************************************************************************
void RenderingContext::setGlobalUniform(const Uniform & u) {
	internalData->globalUniforms.setUniform(u, false, false);
	if(immediate)
		applyChanges();	
}
const Uniform & RenderingContext::getGlobalUniform(const Util::StringIdentifier & uniformName) {
	return internalData->globalUniforms.getUniform(uniformName);
}

// SHADER ************************************************************************************
void RenderingContext::setShader(Shader * shader) {
	if(shader) {
		if(shader->_enable()) {
			internalData->setActiveRenderingStatus(shader->getRenderingStatus());
			if (!internalData->getActiveRenderingStatus()->isInitialized()) { // this shader has not yet been initialized.
				applyChanges(true); // make sure that all uniforms are initially set (e.g. even for disabled lights)
				internalData->getActiveRenderingStatus()->markInitialized();
				//				std::cout << " !!!! FORCED !!! \n";
			}
		} else {
			WARN("RenderingContext::pushShader: can't enable shader, using OpenGL instead");
			internalData->setActiveRenderingStatus(&(internalData->openGLRenderingStatus));
			glUseProgram(0);
		}
	} else {
		internalData->setActiveRenderingStatus(&(internalData->openGLRenderingStatus));

		glUseProgram(0);
	}
	if(immediate)
		applyChanges();
}

void RenderingContext::pushShader() {
	internalData->renderingDataStack.emplace(internalData->getActiveRenderingStatus());
}

void RenderingContext::pushAndSetShader(Shader * shader) {
	pushShader();
	setShader(shader);
}

void RenderingContext::popShader() {
	if(internalData->renderingDataStack.empty()) {
		WARN("popShader: Empty Shader-Stack");
		return;
	}
	setShader(internalData->renderingDataStack.top()->getShader());
	internalData->setActiveRenderingStatus(internalData->renderingDataStack.top());
	internalData->renderingDataStack.pop();

	if(immediate)
		applyChanges();
}

bool RenderingContext::isShaderEnabled(Shader * shader) {
	return shader == internalData->getActiveRenderingStatus()->getShader();
}

Shader * RenderingContext::getActiveShader() {
	return internalData->getActiveRenderingStatus()->getShader();
}

const Shader * RenderingContext::getActiveShader() const {
	return internalData->getActiveRenderingStatus()->getShader();
}

void RenderingContext::dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) {	
	#if defined(LIB_GL) and defined(GL_ARB_compute_shader)
		if(!getActiveShader()) {
			WARN("dispatchCompute: There is no active compute shader.");
		} else {
			applyChanges();
			glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
			GET_GL_ERROR();
		}
	#else
		WARN("dispatchCompute: Compute shaders are not supported.");
	#endif
}

void RenderingContext::dispatchComputeIndirect(size_t offset) {	
	#if defined(LIB_GL) and defined(GL_ARB_compute_shader)
		if(!getActiveShader()) {
			WARN("glDispatchComputeIndirect: There is no active compute shader.");
		} else {
			applyChanges();
			glDispatchComputeIndirect(offset);
			GET_GL_ERROR();
		}
	#else
		WARN("glDispatchComputeIndirect: Compute shaders are not supported.");
	#endif
}

void RenderingContext::loadUniformSubroutines(uint32_t shaderStage, const std::vector<uint32_t>& indices) {
	#if defined(LIB_GL) and defined(GL_ARB_shader_subroutine)
		if(!getActiveShader()) {
			WARN("loadUniformSubroutines: There is no active shader.");
		} else {
			applyChanges();
			glUniformSubroutinesuiv(shaderStage, indices.size(), static_cast<const GLuint*>(indices.data()));
			GET_GL_ERROR();
		}
	#else
		WARN("loadUniformSubroutines: Uniform subroutines are not supported.");
	#endif
}

void RenderingContext::loadUniformSubroutines(uint32_t shaderStage, const std::vector<std::string>& names) {	
	auto shader = getActiveShader();
	if(!shader) {
		WARN("loadUniformSubroutines: There is no active shader.");
	} else {
		std::vector<uint32_t> indices;
		for(const auto& name : names)
			indices.emplace_back(shader->getSubroutineIndex(shaderStage, name));
		loadUniformSubroutines(shaderStage, indices);
	}
}

void RenderingContext::_setUniformOnShader(Shader * shader, const Uniform & uniform, bool warnIfUnused, bool forced) {
	shader->_getUniformRegistry()->setUniform(uniform, warnIfUnused, forced);
	if(immediate && getActiveShader() == shader)
		shader->applyUniforms(false); // forced is false here, as this forced means to re-apply all uniforms
}

// TEXTURES **********************************************************************************

Texture * RenderingContext::getTexture(uint8_t unit)const {
	return unit < MAX_TEXTURES ? internalData->actualCoreRenderingStatus.getTexture(unit).get() : nullptr;
}

TexUnitUsageParameter RenderingContext::getTextureUsage(uint8_t unit)const{
	return internalData->targetRenderingStatus.getTextureUnitParams(unit).first;
}

void RenderingContext::pushTexture(uint8_t unit) {
	internalData->textureStacks.at(unit).emplace(getTexture(unit),getTextureUsage(unit));
}

void RenderingContext::pushAndSetTexture(uint8_t unit, Texture * texture) {
	pushAndSetTexture(unit, texture, TexUnitUsageParameter::TEXTURE_MAPPING);
}

void RenderingContext::pushAndSetTexture(uint8_t unit, Texture * texture, TexUnitUsageParameter usage) {
	pushTexture(unit);
	setTexture(unit, texture, usage);
}

void RenderingContext::popTexture(uint8_t unit) {
	if(internalData->textureStacks.at(unit).empty()) {
		WARN("popTexture: Empty Texture-Stack");
		return;
	}
	const auto& textureAndUsage = internalData->textureStacks[unit].top();
	setTexture(unit, textureAndUsage.first.get(), textureAndUsage.second );
	internalData->textureStacks[unit].pop();
}

void RenderingContext::setTexture(uint8_t unit, Texture * texture) {
	setTexture(unit, texture, TexUnitUsageParameter::TEXTURE_MAPPING);
}

void RenderingContext::setTexture(uint8_t unit, Texture * texture, TexUnitUsageParameter usage) {
	Texture * oldTexture = getTexture(unit);
	if(texture != oldTexture) {
		if(texture) 
			texture->_prepareForBinding(*this);
		internalData->actualCoreRenderingStatus.setTexture(unit, texture);
	}
	const auto oldUsage = internalData->targetRenderingStatus.getTextureUnitParams(unit).first;
	if(!texture){
		if( oldUsage!= TexUnitUsageParameter::DISABLED )
			internalData->targetRenderingStatus.setTextureUnitParams(unit, TexUnitUsageParameter::DISABLED , oldTexture ? oldTexture->getTextureType() : TextureType::TEXTURE_2D);
	}else if( oldUsage!= usage ){
		internalData->targetRenderingStatus.setTextureUnitParams(unit, usage , texture->getTextureType());
	}
	if(immediate)
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
	return internalData->activeFeedbackStatus.first.get();
}

void RenderingContext::popTransformFeedbackBufferStatus(){
	if(internalData->feedbackStack.empty()) {
		WARN("popTransformFeedbackBufferStatus: The stack is empty.");
	}else{
		stopTransformFeedback();
		internalData->activeFeedbackStatus = internalData->feedbackStack.top();
		_startTransformFeedback(internalData->activeFeedbackStatus.second);
	}
}
void RenderingContext::pushTransformFeedbackBufferStatus(){
	internalData->feedbackStack.emplace(internalData->activeFeedbackStatus);
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
	internalData->activeFeedbackStatus.first = buffer;
	_startTransformFeedback(internalData->activeFeedbackStatus.second); // restart
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
	internalData->activeFeedbackStatus.second = primitiveMode;
}
void RenderingContext::startTransformFeedback_lines()		{	_startTransformFeedback(GL_LINES);	}
void RenderingContext::startTransformFeedback_points()		{	_startTransformFeedback(GL_POINTS);	}
void RenderingContext::startTransformFeedback_triangles()	{	_startTransformFeedback(GL_TRIANGLES);	}
void RenderingContext::stopTransformFeedback()				{	_startTransformFeedback(0);	}

// LIGHTS ************************************************************************************

uint8_t RenderingContext::enableLight(const LightParameters & light) {
	if(internalData->targetRenderingStatus.getNumEnabledLights() >= RenderingStatus::MAX_LIGHTS) {
		WARN("Cannot enable more lights; ignoring call.");
		return 255;
	}
	const uint8_t lightNumber = internalData->targetRenderingStatus.enableLight(light);
	if(immediate)
		applyChanges();
	return lightNumber;
}

void RenderingContext::disableLight(uint8_t lightNumber) {
	if (!internalData->targetRenderingStatus.isLightEnabled(lightNumber)) {
		WARN("Cannot disable an already disabled light; ignoring call.");
		return;
	}
	internalData->targetRenderingStatus.disableLight(lightNumber);
	if(immediate)
		applyChanges();
}

// PROJECTION MATRIX *************************************************************************

void RenderingContext::popMatrix_cameraToClipping() {
	if(internalData->projectionMatrixStack.empty()) {
		WARN("Cannot pop projection matrix. The stack is empty.");
		return;
	}
	internalData->targetRenderingStatus.setMatrix_cameraToClipping(internalData->projectionMatrixStack.top());
	internalData->projectionMatrixStack.pop();
	if(immediate)
		applyChanges();
}

void RenderingContext::pushMatrix_cameraToClipping() {
	internalData->projectionMatrixStack.emplace(internalData->targetRenderingStatus.getMatrix_cameraToClipping());
}

void RenderingContext::pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4 & matrix) {
	pushMatrix_cameraToClipping();
	setMatrix_cameraToClipping(matrix);
}
	
void RenderingContext::setMatrix_cameraToClipping(const Geometry::Matrix4x4 & matrix) {
	internalData->targetRenderingStatus.setMatrix_cameraToClipping(matrix);
	if(immediate)
		applyChanges();
}

const Geometry::Matrix4x4 & RenderingContext::getMatrix_cameraToClipping() const {
	return internalData->targetRenderingStatus.getMatrix_cameraToClipping();
}

// CAMERA MATRIX *****************************************************************************

void RenderingContext::setMatrix_cameraToWorld(const Geometry::Matrix4x4 & matrix) {
	internalData->targetRenderingStatus.setMatrix_cameraToWorld(matrix);
	if(immediate)
		applyChanges();
}
const Geometry::Matrix4x4 & RenderingContext::getMatrix_worldToCamera() const {
	return internalData->targetRenderingStatus.getMatrix_worldToCamera();
}
const Geometry::Matrix4x4 & RenderingContext::getMatrix_cameraToWorld() const {
	return internalData->targetRenderingStatus.getMatrix_cameraToWorld();
}

// MODEL VIEW MATRIX *************************************************************************

void RenderingContext::resetMatrix() {
	internalData->targetRenderingStatus.setMatrix_modelToCamera(internalData->targetRenderingStatus.getMatrix_worldToCamera());
	if(immediate)
		applyChanges();
}


void RenderingContext::pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix) {
	pushMatrix_modelToCamera();
	setMatrix_modelToCamera(matrix);
}

const Geometry::Matrix4x4 & RenderingContext::getMatrix_modelToCamera() const {
	return internalData->targetRenderingStatus.getMatrix_modelToCamera();
}

void RenderingContext::pushMatrix_modelToCamera() {
	internalData->matrixStack.emplace(internalData->targetRenderingStatus.getMatrix_modelToCamera());
}

void RenderingContext::multMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix) {
	internalData->targetRenderingStatus.multModelViewMatrix(matrix);
	if(immediate)
		applyChanges();
}

void RenderingContext::setMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix) {
	internalData->targetRenderingStatus.setMatrix_modelToCamera(matrix);
	if(immediate)
		applyChanges();
}

void RenderingContext::popMatrix_modelToCamera() {
	if(internalData->matrixStack.empty()) {
		WARN("Cannot pop matrix. The stack is empty.");
		return;
	}
	internalData->targetRenderingStatus.setMatrix_modelToCamera(internalData->matrixStack.top());
	internalData->matrixStack.pop();
	if(immediate)
		applyChanges();
}

// MATERIAL **********************************************************************************


const MaterialParameters & RenderingContext::getMaterial() const {
	return internalData->targetRenderingStatus.getMaterialParameters();
}

void RenderingContext::popMaterial() {
	if(internalData->materialStack.empty()) {
		WARN("RenderingContext.popMaterial: stack empty, ignoring call");
		FAIL();
		return;
	}
	internalData->materialStack.pop();
	if(internalData->materialStack.empty()) {
		internalData->targetRenderingStatus.disableMaterial();
	} else {
		internalData->targetRenderingStatus.setMaterial(internalData->materialStack.top());
	}
	if(immediate)
		applyChanges();
}

void RenderingContext::pushMaterial() {
	internalData->materialStack.emplace(internalData->targetRenderingStatus.getMaterialParameters());
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
	internalData->targetRenderingStatus.setMaterial(material);
	if(immediate)
		applyChanges();
}

// VIEWPORT **********************************************************************************

static const Uniform::UniformName UNIFORM_SG_VIEWPORT("sg_viewport");

const Geometry::Rect_i & RenderingContext::getWindowClientArea() const {
	return internalData->windowClientArea;
}

const Geometry::Rect_i & RenderingContext::getViewport() const {
	return internalData->currentViewport;
}
void RenderingContext::popViewport() {
	if(internalData->viewportStack.empty()) {
		WARN("Cannot pop viewport stack because it is empty. Ignoring call.");
		return;
	}
	setViewport(internalData->viewportStack.top());
	internalData->viewportStack.pop();
}
void RenderingContext::pushViewport() {
	internalData->viewportStack.emplace(internalData->currentViewport);
}
void RenderingContext::setViewport(const Geometry::Rect_i & viewport) {
	internalData->currentViewport = viewport;
	glViewport(internalData->currentViewport.getX(), internalData->currentViewport.getY(), internalData->currentViewport.getWidth(), internalData->currentViewport.getHeight());

	std::vector<int> vp;
	vp.push_back(internalData->currentViewport.getX());
	vp.push_back(internalData->currentViewport.getY());
	vp.push_back(internalData->currentViewport.getWidth());
	vp.push_back(internalData->currentViewport.getHeight());
	setGlobalUniform(Uniform(UNIFORM_SG_VIEWPORT, vp));
}

void RenderingContext::pushAndSetViewport(const Geometry::Rect_i & viewport) {
	pushViewport();
	setViewport(viewport);
}

void RenderingContext::setWindowClientArea(const Geometry::Rect_i & clientArea) {
	internalData->windowClientArea = clientArea;
}

// VBO Client States **********************************************************************************

void RenderingContext::enableClientState(uint32_t clientState) {
	internalData->activeClientStates.emplace(clientState);
#ifdef LIB_GL
	glEnableClientState(clientState);
#endif /* LIB_GL */
}

void RenderingContext::disableAllClientStates() {
	while(!internalData->activeClientStates.empty()) {
#ifdef LIB_GL
		glDisableClientState(internalData->activeClientStates.top());
#endif /* LIB_GL */
		internalData->activeClientStates.pop();
	}
}

void RenderingContext::enableTextureClientState(uint32_t textureUnit) {
	internalData->activeTextureClientStates.emplace(textureUnit);
#ifdef LIB_GL
	glClientActiveTexture(textureUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif /* LIB_GL */
}

void RenderingContext::disableAllTextureClientStates() {
	while(!internalData->activeTextureClientStates.empty()) {
#ifdef LIB_GL
		glClientActiveTexture(internalData->activeTextureClientStates.top());
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif /* LIB_GL */
		internalData->activeTextureClientStates.pop();
	}
}

void RenderingContext::enableVertexAttribArray(const VertexAttribute & attr, const uint8_t * data, int32_t stride) {
	Shader * shader = getActiveShader();
	GLint location = shader->getVertexAttributeLocation(attr.getNameId());
	if(location != -1) {
		GLuint attribLocation = static_cast<GLuint> (location);
		internalData->activeVertexAttributeBindings.emplace(attribLocation);
		if( attr.getConvertToFloat() ){
			glVertexAttribPointer(attribLocation, attr.getNumValues(), attr.getDataType(), attr.getNormalize() ? GL_TRUE : GL_FALSE, stride, data + attr.getOffset());
		} else {
			glVertexAttribIPointer(attribLocation, attr.getNumValues(), attr.getDataType(), stride, data + attr.getOffset());
		}
		glEnableVertexAttribArray(attribLocation);
	}
}

void RenderingContext::disableAllVertexAttribArrays() {
	while(!internalData->activeVertexAttributeBindings.empty()) {
		glDisableVertexAttribArray(internalData->activeVertexAttributeBindings.top());
		internalData->activeVertexAttributeBindings.pop();
	}
}

}
