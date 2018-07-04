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

#include "internal/PipelineState.h"
#include "internal/ProgramState.h"
#include "RenderingParameters.h"
#include "../BufferObject.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttribute.h"
#include "../Mesh/VertexDescription.h"
#include "../Shader/Shader.h"
#include "../Shader/UniformRegistry.h"
#include "../Texture/Texture.h"
#include "../FBO.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include "../VAO.h"
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

static const Uniform::UniformName UNIFORM_SG_VIEWPORT("sg_viewport");
static const Uniform::UniformName UNIFORM_SG_SCISSOR_RECT("sg_scissorRect");
static const Uniform::UniformName UNIFORM_SG_SCISSOR_ENABLED("sg_scissorEnabled");
	
class RenderingContext::InternalData {
	public:
		ProgramState targetProgramState;
		ProgramState activeProgramState;

		PipelineState targetPipelineState;
		PipelineState activePipelineState;

		std::unordered_map<uint32_t,std::stack<Util::Reference<Texture>>> atomicCounterStacks; 

		std::stack<BlendingParameters> blendingParameterStack;
		std::stack<ColorBufferParameters> colorBufferParameterStack;
		std::stack<CullFaceParameters> cullFaceParameterStack;
		std::stack<DepthBufferParameters> depthBufferParameterStack;
		std::array<std::stack<ImageBindParameters>, MAX_BOUND_IMAGES> imageStacks; 
		std::array<ImageBindParameters, MAX_BOUND_IMAGES> boundImages;
		std::stack<LineParameters> lineParameterStack;
		std::stack<MaterialParameters> materialStack;
		std::stack<PointParameters> pointParameterStack;
		std::stack<PolygonModeParameters> polygonModeParameterStack;
		std::stack<PolygonOffsetParameters> polygonOffsetParameterStack;
		std::stack<ScissorParameters> scissorParametersStack;
		std::stack<StencilParameters> stencilParameterStack;
						
		std::stack<Util::Reference<FBO>> fboStack;
		std::stack<Util::Reference<Shader>> shaderStack;

		UniformRegistry globalUniforms;

		std::stack<Geometry::Matrix4x4> matrixStack;
		std::stack<Geometry::Matrix4x4> projectionMatrixStack;

		std::array<std::stack<std::pair<Util::Reference<Texture>, TexUnitUsageParameter>>, MAX_TEXTURES> textureStacks;

		typedef std::pair<Util::Reference<CountedBufferObject>,uint32_t> feedbackBufferStatus_t; // buffer->mode

		std::stack<feedbackBufferStatus_t> feedbackStack;
		feedbackBufferStatus_t activeFeedbackStatus;
		
		std::stack<Geometry::Rect_i> viewportStack;

		Geometry::Rect_i windowClientArea;
		
		Util::Reference<VAO> defaultVAO;
};

RenderingContext::RenderingContext() :
	internalData(new InternalData), immediate(false), displayMeshFn() {

	resetDisplayMeshFn();
	internalData->activeProgramState.initBuffers();

	setBlending(BlendingParameters());
	setColorBuffer(ColorBufferParameters());
	// Initially enable the back-face culling
	setCullFace(CullFaceParameters::CULL_BACK);
	// Initially enable the depth test.
	setDepthBuffer(DepthBufferParameters(true, true, Comparison::LESS));
	
	setLine(LineParameters());
	setPointParameters(PointParameters());
	setPolygonOffset(PolygonOffsetParameters());
	setStencil(StencilParameters());
	
	internalData->defaultVAO = new VAO;
	internalData->defaultVAO->bind();
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
	applyChanges();
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
	
	if(!glewIsSupported("GL_VERSION_4_5")) {
		throw std::runtime_error("RenderingContext::initGLState: Required OpenGL version 4.5 is not supported.");
	}

#ifdef LIB_GL
	glPixelStorei( GL_PACK_ALIGNMENT,1); // allow glReadPixel for all possible resolutions

	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	glBlendEquation(GL_FUNC_ADD);
	
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	
	// Enable the possibility to write gl_PointSize from the vertex shader.
	glEnable(GL_PROGRAM_POINT_SIZE);
	//glEnable(GL_POINT_SPRITE);
	
	// Workaround: Create a single vertex array object here.
	// For the core profile of OpenGL 3.2 or higher this is required,
	// because glVertexAttribPointer generates an GL_INVALID_OPERATION without it.
	// In the future, vertex array objects should be integrated into the rendering system.
	//GLuint vertexArrayObject;
	//glGenVertexArrays(1, &vertexArrayObject);
	//glBindVertexArray(vertexArrayObject);
	
#endif /* LIB_GL */
#endif /* LIB_GLEW */

#ifdef WIN32
	wglSwapIntervalEXT(false);
#endif /* WIN32 */
}

//! (static)
void RenderingContext::flush() {
	applyChanges();
	glFlush();
}

//! (static)
void RenderingContext::finish() {
	applyChanges();
	glFinish();
}

void RenderingContext::barrier(uint32_t flags) {
	applyChanges();
	glMemoryBarrier(flags == 0 ? GL_ALL_BARRIER_BITS : static_cast<GLbitfield>(flags));
}
// Applying changes ***************************************************************************

void RenderingContext::applyChanges(bool forced) {
	try {
		if(forced)
			internalData->activePipelineState.invalidate();
		internalData->activePipelineState.setDebug(debugMode);
		internalData->activePipelineState.apply(internalData->targetPipelineState);
		
		if(internalData->activePipelineState.isShaderValid()) {
			auto shader = internalData->activePipelineState.getShader();
			internalData->activeProgramState.apply(shader.get(), internalData->targetProgramState, forced);

			// transfer updated global uniforms to the shader
			shader->_getUniformRegistry()->performGlobalSync(internalData->globalUniforms, false);

			// apply uniforms
			shader->applyUniforms(forced);
			GET_GL_ERROR();
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
	return internalData->targetPipelineState.getBlendingParameters();
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
	internalData->blendingParameterStack.emplace(internalData->targetPipelineState.getBlendingParameters());
}

void RenderingContext::setBlending(const BlendingParameters & p) {
	internalData->targetPipelineState.setBlendingParameters(p);
	if(immediate)
		applyChanges();
}


// ClipPlane ************************************************************************************

const ClipPlaneParameters & RenderingContext::getClipPlane(uint8_t index) const {
	static ClipPlaneParameters p;
	return p;
}

// ColorBuffer ************************************************************************************
const ColorBufferParameters & RenderingContext::getColorBufferParameters() const {
	return internalData->targetPipelineState.getColorBufferParameters();
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
	internalData->colorBufferParameterStack.emplace(internalData->targetPipelineState.getColorBufferParameters());
}

void RenderingContext::pushAndSetColorBuffer(const ColorBufferParameters & p) {
	pushColorBuffer();
	setColorBuffer(p);
}

void RenderingContext::setColorBuffer(const ColorBufferParameters & p) {
	internalData->targetPipelineState.setColorBufferParameters(p);
	if(immediate)
		applyChanges();
}

void RenderingContext::clearColor(const Util::Color4f & clearValue) {
	applyChanges();
	glClearColor(clearValue.getR(), clearValue.getG(), clearValue.getB(), clearValue.getA());
	glClear(GL_COLOR_BUFFER_BIT);
}

// Cull Face ************************************************************************************
const CullFaceParameters & RenderingContext::getCullFaceParameters() const {
	return internalData->targetPipelineState.getCullFaceParameters();
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
	internalData->cullFaceParameterStack.emplace(internalData->targetPipelineState.getCullFaceParameters());
}

void RenderingContext::pushAndSetCullFace(const CullFaceParameters & p) {
	pushCullFace();
	setCullFace(p);
}

void RenderingContext::setCullFace(const CullFaceParameters & p) {
	internalData->targetPipelineState.setCullFaceParameters(p);
	if(immediate)
		applyChanges();
}

// DepthBuffer ************************************************************************************
const DepthBufferParameters & RenderingContext::getDepthBufferParameters() const {
	return internalData->targetPipelineState.getDepthBufferParameters();
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
	internalData->depthBufferParameterStack.emplace(internalData->targetPipelineState.getDepthBufferParameters());
}

void RenderingContext::pushAndSetDepthBuffer(const DepthBufferParameters & p) {
	pushDepthBuffer();
	setDepthBuffer(p);
}

void RenderingContext::setDepthBuffer(const DepthBufferParameters & p) {
	internalData->targetPipelineState.setDepthBufferParameters(p);
	if(immediate)
		applyChanges();
}

void RenderingContext::clearDepth(float clearValue) {
	applyChanges();
#ifdef LIB_GLESv2
	glClearDepthf(clearValue);
#else
	glClearDepth(clearValue);
#endif
	glClear(GL_DEPTH_BUFFER_BIT);
}

// AlphaTest ************************************************************************************
const AlphaTestParameters & RenderingContext::getAlphaTestParameters() const {
	static AlphaTestParameters p;
	return p;
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
	static LightingParameters p;
	return p;
}

// Line ************************************************************************************
const LineParameters& RenderingContext::getLineParameters() const {
	return internalData->targetPipelineState.getLineParameters();
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
	internalData->lineParameterStack.emplace(internalData->targetPipelineState.getLineParameters());
}

void RenderingContext::pushAndSetLine(const LineParameters & p) {
	pushLine();
	setLine(p);
}

void RenderingContext::setLine(const LineParameters & p) {
	internalData->targetPipelineState.setLineParameters(p);
	if(immediate)
		applyChanges();
}

// Point ************************************************************************************
const PointParameters& RenderingContext::getPointParameters() const {
	return internalData->targetProgramState.getPointParameters();
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
	internalData->pointParameterStack.emplace(internalData->targetProgramState.getPointParameters());
}

void RenderingContext::pushAndSetPointParameters(const PointParameters & p) {
	pushPointParameters();
	setPointParameters(p);
}

void RenderingContext::setPointParameters(const PointParameters & p) {
	internalData->targetProgramState.setPointParameters(p);
	if(immediate)
		applyChanges();
}
// PolygonMode ************************************************************************************
const PolygonModeParameters & RenderingContext::getPolygonModeParameters() const {
	return internalData->targetPipelineState.getPolygonModeParameters();
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
	internalData->polygonModeParameterStack.emplace(internalData->targetPipelineState.getPolygonModeParameters());
}

void RenderingContext::pushAndSetPolygonMode(const PolygonModeParameters & p) {
	pushPolygonMode();
	setPolygonMode(p);
}

void RenderingContext::setPolygonMode(const PolygonModeParameters & p) {
	internalData->targetPipelineState.setPolygonModeParameters(p);
	if(immediate)
		applyChanges();
}

// PolygonOffset ************************************************************************************
const PolygonOffsetParameters & RenderingContext::getPolygonOffsetParameters() const {
	return internalData->targetPipelineState.getPolygonOffsetParameters();
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
	internalData->polygonOffsetParameterStack.emplace(internalData->targetPipelineState.getPolygonOffsetParameters());
}

void RenderingContext::pushAndSetPolygonOffset(const PolygonOffsetParameters & p) {
	pushPolygonOffset();
	setPolygonOffset(p);
}

void RenderingContext::setPolygonOffset(const PolygonOffsetParameters & p) {
	internalData->targetPipelineState.setPolygonOffsetParameters(p);
	if(immediate)
		applyChanges();
}

// Scissor ************************************************************************************

const ScissorParameters & RenderingContext::getScissor() const {
	return internalData->targetPipelineState.getScissorParameters();
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

void RenderingContext::setScissor(const ScissorParameters & scissorParameters) {
	internalData->targetPipelineState.setScissorParameters(scissorParameters);
	const auto& sr = scissorParameters.getRect();
	internalData->globalUniforms.setUniform({UNIFORM_SG_SCISSOR_RECT, Geometry::Vec4(sr.getX(), sr.getY(), sr.getWidth(), sr.getHeight())}, false, false);
	internalData->globalUniforms.setUniform({UNIFORM_SG_SCISSOR_ENABLED, scissorParameters.isEnabled()}, false, false);
	if(immediate)
		applyChanges();
}


// Stencil ************************************************************************************
const StencilParameters & RenderingContext::getStencilParamters() const {
	return internalData->targetPipelineState.getStencilParameters();
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
	internalData->stencilParameterStack.emplace(internalData->targetPipelineState.getStencilParameters());
}

void RenderingContext::setStencil(const StencilParameters & stencilParameter) {
	internalData->targetPipelineState.setStencilParameters(stencilParameter);
	if(immediate)
		applyChanges();
}

void RenderingContext::clearStencil(int32_t clearValue) {
	applyChanges();
	glClearStencil(clearValue);
	glClear(GL_STENCIL_BUFFER_BIT);
}

// FBO ************************************************************************************

FBO * RenderingContext::getActiveFBO() const {
	return internalData->targetPipelineState.getFBO().get();
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
	internalData->targetPipelineState.setFBO(fbo);
	if(immediate)
		applyChanges();
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
	internalData->targetPipelineState.setShader(shader);
	if(immediate)
		applyChanges();
}

void RenderingContext::pushShader() {
	internalData->shaderStack.emplace(getActiveShader());
}

void RenderingContext::pushAndSetShader(Shader * shader) {
	pushShader();
	setShader(shader);
}

void RenderingContext::popShader() {
	if(internalData->shaderStack.empty()) {
		WARN("popShader: Empty Shader-Stack");
		return;
	}
	setShader(internalData->shaderStack.top().get());
}

bool RenderingContext::isShaderEnabled(Shader * shader) {
	return shader == getActiveShader();
}

Shader * RenderingContext::getActiveShader() {
	return internalData->targetPipelineState.getShader().get();
}

const Shader * RenderingContext::getActiveShader() const {
	return internalData->targetPipelineState.getShader().get();
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
	return unit < MAX_TEXTURES ? internalData->targetPipelineState.getTexture(unit).get() : nullptr;
}

TexUnitUsageParameter RenderingContext::getTextureUsage(uint8_t unit)const{
	return internalData->targetProgramState.getTextureUnitParams(unit).first;
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
		internalData->targetPipelineState.setTexture(unit, texture);
	}
	const auto oldUsage = internalData->targetProgramState.getTextureUnitParams(unit).first;
	if(!texture){
		if( oldUsage!= TexUnitUsageParameter::DISABLED )
			internalData->targetProgramState.setTextureUnitParams(unit, TexUnitUsageParameter::DISABLED , oldTexture ? oldTexture->getTextureType() : TextureType::TEXTURE_2D);
	}else if( oldUsage!= usage ){
		internalData->targetProgramState.setTextureUnitParams(unit, usage , texture->getTextureType());
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
	applyChanges();
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
	applyChanges();
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
	if(internalData->targetProgramState.getNumEnabledLights() >= ProgramState::MAX_LIGHTS) {
		WARN("Cannot enable more lights; ignoring call.");
		return 255;
	}
	const uint8_t lightNumber = internalData->targetProgramState.enableLight(light);
	if(immediate)
		applyChanges();
	return lightNumber;
}

void RenderingContext::disableLight(uint8_t lightNumber) {
	if (!internalData->targetProgramState.isLightEnabled(lightNumber)) {
		WARN("Cannot disable an already disabled light; ignoring call.");
		return;
	}
	internalData->targetProgramState.disableLight(lightNumber);
	if(immediate)
		applyChanges();
}

// PROJECTION MATRIX *************************************************************************

void RenderingContext::popMatrix_cameraToClipping() {
	if(internalData->projectionMatrixStack.empty()) {
		WARN("Cannot pop projection matrix. The stack is empty.");
		return;
	}
	internalData->targetProgramState.setMatrix_cameraToClipping(internalData->projectionMatrixStack.top());
	internalData->projectionMatrixStack.pop();
	if(immediate)
		applyChanges();
}

void RenderingContext::pushMatrix_cameraToClipping() {
	internalData->projectionMatrixStack.emplace(internalData->targetProgramState.getMatrix_cameraToClipping());
}

void RenderingContext::pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4 & matrix) {
	pushMatrix_cameraToClipping();
	setMatrix_cameraToClipping(matrix);
}
	
void RenderingContext::setMatrix_cameraToClipping(const Geometry::Matrix4x4 & matrix) {
	internalData->targetProgramState.setMatrix_cameraToClipping(matrix);
	if(immediate)
		applyChanges();
}

const Geometry::Matrix4x4 & RenderingContext::getMatrix_cameraToClipping() const {
	return internalData->targetProgramState.getMatrix_cameraToClipping();
}

// CAMERA MATRIX *****************************************************************************

void RenderingContext::setMatrix_cameraToWorld(const Geometry::Matrix4x4 & matrix) {
	internalData->targetProgramState.setMatrix_cameraToWorld(matrix);
	if(immediate)
		applyChanges();
}
const Geometry::Matrix4x4 & RenderingContext::getMatrix_worldToCamera() const {
	return internalData->targetProgramState.getMatrix_worldToCamera();
}
const Geometry::Matrix4x4 & RenderingContext::getMatrix_cameraToWorld() const {
	return internalData->targetProgramState.getMatrix_cameraToWorld();
}

// MODEL VIEW MATRIX *************************************************************************

void RenderingContext::resetMatrix() {
	internalData->targetProgramState.setMatrix_modelToCamera(internalData->targetProgramState.getMatrix_worldToCamera());
	if(immediate)
		applyChanges();
}


void RenderingContext::pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix) {
	pushMatrix_modelToCamera();
	setMatrix_modelToCamera(matrix);
}

const Geometry::Matrix4x4 & RenderingContext::getMatrix_modelToCamera() const {
	return internalData->targetProgramState.getMatrix_modelToCamera();
}

void RenderingContext::pushMatrix_modelToCamera() {
	internalData->matrixStack.emplace(internalData->targetProgramState.getMatrix_modelToCamera());
}

void RenderingContext::multMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix) {
	internalData->targetProgramState.multModelViewMatrix(matrix);
	if(immediate)
		applyChanges();
}

void RenderingContext::setMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix) {
	internalData->targetProgramState.setMatrix_modelToCamera(matrix);
	if(immediate)
		applyChanges();
}

void RenderingContext::popMatrix_modelToCamera() {
	if(internalData->matrixStack.empty()) {
		WARN("Cannot pop matrix. The stack is empty.");
		return;
	}
	internalData->targetProgramState.setMatrix_modelToCamera(internalData->matrixStack.top());
	internalData->matrixStack.pop();
	if(immediate)
		applyChanges();
}

// MATERIAL **********************************************************************************


const MaterialParameters & RenderingContext::getMaterial() const {
	return internalData->targetProgramState.getMaterialParameters();
}

void RenderingContext::popMaterial() {
	if(internalData->materialStack.empty()) {
		WARN("RenderingContext.popMaterial: stack empty, ignoring call");
		FAIL();
		return;
	}
	internalData->materialStack.pop();
	if(internalData->materialStack.empty()) {
		internalData->targetProgramState.disableMaterial();
	} else {
		internalData->targetProgramState.setMaterial(internalData->materialStack.top());
	}
	if(immediate)
		applyChanges();
}

void RenderingContext::pushMaterial() {
	internalData->materialStack.emplace(internalData->targetProgramState.getMaterialParameters());
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
	pushAndSetMaterial(material);
}
void RenderingContext::setMaterial(const MaterialParameters & material) {
	internalData->targetProgramState.setMaterial(material);
	if(immediate)
		applyChanges();
}

//  **********************************************************************************


const Geometry::Rect_i & RenderingContext::getWindowClientArea() const {
	return internalData->windowClientArea;
}

const Geometry::Rect_i & RenderingContext::getViewport() const {
	return internalData->targetPipelineState.getViewport();
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
	internalData->viewportStack.emplace(getViewport());
}
void RenderingContext::setViewport(const Geometry::Rect_i & vp) {
	internalData->targetPipelineState.setViewport(vp);
	internalData->globalUniforms.setUniform({UNIFORM_SG_VIEWPORT, Geometry::Vec4(vp.getX(), vp.getY(), vp.getWidth(), vp.getHeight())}, false, false);
	
	if(immediate)
		applyChanges();
}

void RenderingContext::pushAndSetViewport(const Geometry::Rect_i & viewport) {
	pushViewport();
	setViewport(viewport);
}

void RenderingContext::setWindowClientArea(const Geometry::Rect_i & clientArea) {
	internalData->windowClientArea = clientArea;
}

// Vertex Format **********************************************************************************

void RenderingContext::setVertexFormat(uint32_t binding, const VertexDescription& vd) {
	const auto& shader = getActiveShader();
	internalData->targetPipelineState.resetVertexFormats(binding);
	if(shader) {
		for(const auto& attr : vd.getAttributes()) {
			int32_t location = shader->getVertexAttributeLocation(attr.getNameId());
			if(location >= 0 && location < PipelineState::MAX_VERTEXATTRIBS)
				internalData->targetPipelineState.setVertexFormat(location, attr, binding);
		}
	} else {
		uint32_t location = 0;
		for(const auto& attr : vd.getAttributes())
			internalData->targetPipelineState.setVertexFormat(location++, attr, binding);
	}
	if(immediate)
		applyChanges();
}

void RenderingContext::bindVertexBuffer(uint32_t binding, uint32_t bufferId, uint32_t offset, uint32_t stride, uint32_t divisor) {
	internalData->targetPipelineState.setVertexBinding(binding, bufferId, offset, stride, divisor);
	if(immediate)
		applyChanges();
}

void RenderingContext::bindIndexBuffer(uint32_t bufferId) {
	applyChanges();
	internalData->defaultVAO->bindElementBuffer(bufferId);
}

// Draw Commands **********************************************************************************

void RenderingContext::submitDraw(uint32_t mode, const DrawArraysCommand& cmd) {
	applyChanges();
	//glDrawArraysIndirect(mode, &cmd);
	if(debugMode) std::cout << "draw arrays " << cmd.first << " - " << cmd.count << std::endl;
  glDrawArraysInstancedBaseInstance(mode, cmd.first, cmd.count, cmd.primCount, cmd.baseInstance);
}

void RenderingContext::submitDraw(uint32_t mode, uint32_t type, const DrawElementsCommand& cmd) {
	applyChanges();
	//glDrawElementsIndirect(mode, type, &cmd);
	if(debugMode) std::cout << "draw elements " << cmd.first << " - " << cmd.count << std::endl;
	uint8_t* first = reinterpret_cast<uint8_t*>(cmd.first * getGLTypeSize(type));
	glDrawElementsInstancedBaseVertexBaseInstance(mode, cmd.count, type, first, cmd.primCount, cmd.baseVertex, cmd.baseInstance);
}

}
