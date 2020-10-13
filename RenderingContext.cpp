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

#include "State/BindingState.h"
#include "State/PipelineState.h"
#include "Core/Device.h"
#include "Core/CommandBuffer.h"
#include "Core/Swapchain.h"
#include "Core/Queue.h"

#include "RenderingParameters.h"
#include "BufferObject.h"
#include "Mesh/Mesh.h"
#include "Mesh/VertexAttribute.h"
#include "Shader/Shader.h"
#include "Shader/UniformRegistry.h"
#include "Texture/Texture.h"
#include "FBO.h"
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <array>
#include <stdexcept>
#include <stack>

namespace Rendering {


class RenderingContext::InternalData {
	public:
		DeviceRef device;
		PipelineState pipelineState;
		BindingState bindingState;
		CommandBuffer::Ref cmd;

		// pipeline state stacks
		std::stack<VertexInputState> vertexInputStack;
		std::stack<InputAssemblyState> inputAssemblyStack;
		std::stack<ViewportState> viewportStack;
		std::stack<RasterizationState> rasterizationStack;
		std::stack<MultisampleState> multisampleStack;
		std::stack<DepthStencilState> depthStencilStack;
		std::stack<ColorBlendState> colorBlendStack;
		std::stack<Geometry::Rect_i> viewportStack;

		// binding stacks
		std::array<std::stack<Texture::Ref>, MAX_TEXTURES> textureStacks;
		std::array<std::stack<ImageBindParameters>, MAX_BOUND_IMAGES> imageStacks;
		
		// transformation stack
		std::stack<Geometry::Matrix4x4> matrixStack;
		std::stack<Geometry::Matrix4x4> projectionMatrixStack;

		// lights& materials
		std::stack<LightingParameters> lightingParameterStack;
		std::stack<MaterialParameters> materialStack;

		// fbo
		std::stack<FBO::Ref> fboStack;
		Geometry::Rect_i windowClientArea;

		// shader
		std::stack<Shader::Ref> shaderStack;
		Shader::Ref activeShader;
		Shader::Ref fallbackShader;

		UniformRegistry globalUniforms;

};

RenderingContext::RenderingContext(const DeviceRef& device) :
	internal(new InternalData), displayMeshFn() {

	resetDisplayMeshFn();

	internal->device = device;
	
	internal->fallbackShader = ShaderUtils::createDefaultShader(device);

	internal->cmd = CommandBuffer::create(device->getQueue(QueueFamily::Graphics));
	internal->cmd->begin();

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

	applyChanges();
}

RenderingContext::RenderingContext() : RenderingContext(Device::getDefault()) {}

RenderingContext::~RenderingContext() = default;

void RenderingContext::resetDisplayMeshFn() {
	using namespace std::placeholders;
	displayMeshFn = std::bind(&Rendering::Mesh::_display, _2, _1, _3, _4);
}

void RenderingContext::displayMesh(Mesh * mesh){
	applyChanges();
	displayMeshFn(*this, mesh,0,mesh->isUsingIndexData()? mesh->getIndexCount() : mesh->getVertexCount());
}

// helper ***************************************************************************

void RenderingContext::flush(bool wait) {
	applyChanges();
	if(internal->cmd->isInRenderPass())
		internal->cmd->endRenderPass();
	internal->cmd->end();
	internal->cmd->submit(wait);

	internal->cmd = CommandBuffer::create(internal->device->getQueue(QueueFamily::Graphics));
	internal->cmd->begin();
}

void RenderingContext::present() {
	flush();
	internal->device->getQueue(QueueFamily::Present)->present();
}


void RenderingContext::barrier(uint32_t flags) {
	applyChanges();
	WARN("RenderingContext: barrier() is currrently not supported");
}


// Applying changes ***************************************************************************

void RenderingContext::applyChanges(bool forced) {
	internal->cmd->setPipelineState(internal->pipelineState);
	if(!internal->activeShader || !internal->activeShader->init()) {
		internal->cmd->setShader(internal->fallbackShader);
	} else {
		internal->cmd->setShader(internal->activeShader);
	}

	//auto shader = internal->cmd->getShader();

	// transfer updated global uniforms to the shader
	//shader->_getUniformRegistry()->performGlobalSync(internal->globalUniforms, false);

	// apply uniforms
	//shader->applyUniforms(forced);
}

// Clear ***************************************************************************

void RenderingContext::clearColor(const Util::Color4f& color) {
	applyChanges();
	internal->cmd->clearColor({color});
}

void RenderingContext::clearScreen(const Util::Color4f& color) {
	applyChanges();
	internal->cmd->clearColor({color});
	internal->cmd->clearDepthStencil(1, 0);
}

void RenderingContext::clearScreenRect(const Geometry::Rect_i& rect, const Util::Color4f& color, bool _clearDepth) {
	applyChanges();
	internal->cmd->clearColor({color}, rect);
	if(_clearDepth)
		internal->cmd->clearDepthStencil(1, 0, rect);
}


void RenderingContext::clearDepth(float clearValue) {
	internal->cmd->clearDepthStencil(clearValue, 0);
}

// AlphaTest ************************************************************************************

const AlphaTestParameters& RenderingContext::getAlphaTestParameters() const {
	return {};
}

// Blending ************************************************************************************

const BlendingParameters& RenderingContext::getBlendingParameters() const {
	return BlendingParameters(internal->pipelineState.getColorBlendState());
}

const ColorBlendState& RenderingContext::getBlending() const {
	return internal->pipelineState.getColorBlendState();
}

void RenderingContext::pushAndSetBlending(const BlendingParameters& p) {
	pushBlending();
	setBlending(p);
}

void RenderingContext::pushAndSetBlending(const ColorBlendState& s) {
	pushBlending();
	setBlending(s);
}

void RenderingContext::popBlending() {
	if(internal->colorBlendStack.empty()) {
		WARN("popBlending: Empty Blending-Stack");
		return;
	}
	setBlending(internal->colorBlendStack.top());
	internal->colorBlendStack.pop();
}

void RenderingContext::pushBlending() {
	internal->colorBlendStack.emplace(internal->pipelineState.getColorBlendState());
}

void RenderingContext::setBlending(const BlendingParameters& p) {
	auto state = internal->pipelineState.getColorBlendState();
	auto attachment = p.toBlendState().getAttachment();
	attachment.colorWriteMask = state.getAttachment().colorWriteMask;
	state.setAttachment(attachment);
	internal->pipelineState.setColorBlendState(state);
}

void RenderingContext::setBlending(const ColorBlendState& s) {
	internal->pipelineState.setColorBlendState(s);
}


// ClipPlane ************************************************************************************

const ClipPlaneParameters& RenderingContext::getClipPlane(uint8_t index) const {	
	return {};
}


// ColorBuffer ************************************************************************************
const ColorBufferParameters& RenderingContext::getColorBufferParameters() const {
	return ColorBufferParameters(internal->pipelineState.getColorBlendState().getAttachment().colorWriteMask);
}

void RenderingContext::popColorBuffer() {
	popBlending();
}

void RenderingContext::pushColorBuffer() {
	pushBlending();
}

void RenderingContext::pushAndSetColorBuffer(const ColorBufferParameters& p) {
	pushBlending();
	setColorBuffer(p);
}

void RenderingContext::setColorBuffer(const ColorBufferParameters& p) {
	auto state = internal->pipelineState.getColorBlendState();
	auto attachment = state.getAttachment();
	attachment.colorWriteMask = p.getWriteMask();
	state.setAttachment(attachment);
	internal->pipelineState.setColorBlendState(state);
}

// Cull Face ************************************************************************************
const CullFaceParameters& RenderingContext::getCullFaceParameters() const {
	return internal->pipelineState.getRasterizationState().getCullMode();
}
void RenderingContext::popCullFace() {
	popRasterization();
}

void RenderingContext::pushCullFace() {
	pushRasterization();
}

void RenderingContext::pushAndSetCullFace(const CullFaceParameters& p) {
	pushRasterization();
	setCullFace(p);
}

void RenderingContext::setCullFace(const CullFaceParameters& p) {
	auto state = internal->pipelineState.getRasterizationState();
	state.setCullMode(p.getCullMode());
	internal->pipelineState.setRasterizationState(state);
}


// DepthStencil ************************************************************************************

const DepthStencilState& RenderingContext::getDepthStencil() const {
	internal->pipelineState.getDepthStencilState();
}

void RenderingContext::popDepthStencil() {
	if(internal->depthStencilStack.empty()) {
		WARN("popDepthStencil: Empty DepthStencil stack");
		return;
	}
	setDepthStencil(internal->depthStencilStack.top());
	internal->depthStencilStack.pop();
}

void RenderingContext::pushDepthStencil() {
	internal->depthStencilStack.emplace(internal->pipelineState.getDepthStencilState());
}

void RenderingContext::pushAndSetDepthStencil(const DepthStencilState& state) {
	pushDepthStencil();
	setDepthStencil(state);
}

void RenderingContext::setDepthStencil(const DepthStencilState& state) {
	internal->pipelineState.setDepthStencilState(state);
}

// DepthBuffer ************************************************************************************
const DepthBufferParameters& RenderingContext::getDepthBufferParameters() const {
	auto state = internal->pipelineState.getDepthStencilState();
	return DepthBufferParameters(state.isDepthTestEnabled(), state.isDepthWriteEnabled(), Comparison::comparisonFuncToFunction(state.getDepthCompareOp()));
}
void RenderingContext::popDepthBuffer() {
	popDepthStencil();
}

void RenderingContext::pushDepthBuffer() {
	pushDepthStencil();
}

void RenderingContext::pushAndSetDepthBuffer(const DepthBufferParameters& p) {
	pushDepthStencil();
	setDepthBuffer(p);
}

void RenderingContext::setDepthBuffer(const DepthBufferParameters& p) {
	auto state = internal->pipelineState.getDepthStencilState();
	state.setDepthTestEnabled(p.isTestEnabled());
	state.setDepthWriteEnabled(p.isWritingEnabled());
	state.setDepthCompareOp(p.isTestEnabled() ? Comparison::functionToComparisonFunc(p.getFunction()) : ComparisonFunc::Disabled);
	internal->pipelineState.setDepthStencilState(state);
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
	return internal->boundImages[unit];
}

void RenderingContext::pushBoundImage(uint8_t unit){
	assertCorrectImageUnit(unit);
	internal->imageStacks[unit].push( internal->boundImages[unit] );
}

void RenderingContext::pushAndSetBoundImage(uint8_t unit, const ImageBindParameters& iParam){
	pushBoundImage(unit);
	setBoundImage(unit,iParam);
}
void RenderingContext::popBoundImage(uint8_t unit){
	assertCorrectImageUnit(unit);
	auto& iStack = internal->imageStacks[unit];
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
	internal->boundImages[unit] = iParam;
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
const LightingParameters& RenderingContext::getLightingParameters() const {
	return internal->actualCoreRenderingStatus.getLightingParameters();
}
void RenderingContext::popLighting() {
	if(internal->lightingParameterStack.empty()) {
		WARN("popLighting: Empty lighting stack");
		return;
	}
	setLighting(internal->lightingParameterStack.top());
	internal->lightingParameterStack.pop();
}

void RenderingContext::pushLighting() {
	internal->lightingParameterStack.emplace(internal->actualCoreRenderingStatus.getLightingParameters());
}

void RenderingContext::pushAndSetLighting(const LightingParameters& p) {
	pushLighting();
	setLighting(p);
}

void RenderingContext::setLighting(const LightingParameters& p) {
	internal->actualCoreRenderingStatus.setLightingParameters(p);
	if(immediate)
		applyChanges();
}

// Line ************************************************************************************
const LineParameters& RenderingContext::getLineParameters() const {
	return internal->actualCoreRenderingStatus.getLineParameters();
}

void RenderingContext::popLine() {
	if(internal->lineParameterStack.empty()) {
		WARN("popLine: Empty line parameters stack");
		return;
	}
	setLine(internal->lineParameterStack.top());
	internal->lineParameterStack.pop();
}

void RenderingContext::pushLine() {
	internal->lineParameterStack.emplace(internal->actualCoreRenderingStatus.getLineParameters());
}

void RenderingContext::pushAndSetLine(const LineParameters& p) {
	pushLine();
	setLine(p);
}

void RenderingContext::setLine(const LineParameters& p) {
	internal->actualCoreRenderingStatus.setLineParameters(p);
	if(immediate)
		applyChanges();
}

// Point ************************************************************************************
const PointParameters& RenderingContext::getPointParameters() const {
	return internal->targetRenderingStatus.getPointParameters();
}

void RenderingContext::popPointParameters() {
	if(internal->pointParameterStack.empty()) {
		WARN("popPoint: Empty point parameters stack");
		return;
	}
	setPointParameters(internal->pointParameterStack.top());
	internal->pointParameterStack.pop();
}

void RenderingContext::pushPointParameters() {
	internal->pointParameterStack.emplace(internal->targetRenderingStatus.getPointParameters());
}

void RenderingContext::pushAndSetPointParameters(const PointParameters& p) {
	pushPointParameters();
	setPointParameters(p);
}

void RenderingContext::setPointParameters(const PointParameters& p) {
	internal->targetRenderingStatus.setPointParameters(p);
	if(immediate)
		applyChanges();
}
// PolygonMode ************************************************************************************
const PolygonModeParameters& RenderingContext::getPolygonModeParameters() const {
	return internal->actualCoreRenderingStatus.getPolygonModeParameters();
}
void RenderingContext::popPolygonMode() {
	if(internal->polygonModeParameterStack.empty()) {
		WARN("popPolygonMode: Empty PolygonMode-Stack");
		return;
	}
	setPolygonMode(internal->polygonModeParameterStack.top());
	internal->polygonModeParameterStack.pop();
}

void RenderingContext::pushPolygonMode() {
	internal->polygonModeParameterStack.emplace(internal->actualCoreRenderingStatus.getPolygonModeParameters());
}

void RenderingContext::pushAndSetPolygonMode(const PolygonModeParameters& p) {
	pushPolygonMode();
	setPolygonMode(p);
}

void RenderingContext::setPolygonMode(const PolygonModeParameters& p) {
	internal->actualCoreRenderingStatus.setPolygonModeParameters(p);
	if(immediate)
		applyChanges();
}

// PolygonOffset ************************************************************************************
const PolygonOffsetParameters& RenderingContext::getPolygonOffsetParameters() const {
	return internal->actualCoreRenderingStatus.getPolygonOffsetParameters();
}
void RenderingContext::popPolygonOffset() {
	if(internal->polygonOffsetParameterStack.empty()) {
		WARN("popPolygonOffset: Empty PolygonOffset stack");
		return;
	}
	setPolygonOffset(internal->polygonOffsetParameterStack.top());
	internal->polygonOffsetParameterStack.pop();
}

void RenderingContext::pushPolygonOffset() {
	internal->polygonOffsetParameterStack.emplace(internal->actualCoreRenderingStatus.getPolygonOffsetParameters());
}

void RenderingContext::pushAndSetPolygonOffset(const PolygonOffsetParameters& p) {
	pushPolygonOffset();
	setPolygonOffset(p);
}

void RenderingContext::setPolygonOffset(const PolygonOffsetParameters& p) {
	internal->actualCoreRenderingStatus.setPolygonOffsetParameters(p);
	if(immediate)
		applyChanges();
}

// PolygonOffset ************************************************************************************
const PrimitiveRestartParameters& RenderingContext::getPrimitiveRestartParameters() const {
	return internal->actualCoreRenderingStatus.getPrimitiveRestartParameters();
}
void RenderingContext::popPrimitiveRestart() {
	if(internal->primitiveRestartParameterStack.empty()) {
		WARN("popPrimitiveRestart: Empty PrimitiveRestart stack");
		return;
	}
	setPrimitiveRestart(internal->primitiveRestartParameterStack.top());
	internal->primitiveRestartParameterStack.pop();
}

void RenderingContext::pushPrimitiveRestart() {
	internal->primitiveRestartParameterStack.emplace(internal->actualCoreRenderingStatus.getPrimitiveRestartParameters());
}

void RenderingContext::pushAndSetPrimitiveRestart(const PrimitiveRestartParameters& p) {
	pushPrimitiveRestart();
	setPrimitiveRestart(p);
}

void RenderingContext::setPrimitiveRestart(const PrimitiveRestartParameters& p) {
	internal->actualCoreRenderingStatus.setPrimitiveRestartParameters(p);
	if(immediate)
		applyChanges();
}

// Rasterization ************************************************************************************

const RasterizationState& RenderingContext::getRasterization() const {
	internal->pipelineState.getRasterizationState();
}

void RenderingContext::popRasterization() {
	if(internal->rasterizationStack.empty()) {
		WARN("popRasterization: Empty Rasterization stack");
		return;
	}
	setRasterization(internal->rasterizationStack.top());
	internal->rasterizationStack.pop();
}

void RenderingContext::pushRasterization() {
	internal->rasterizationStack.emplace(internal->pipelineState.getRasterizationState());
}

void RenderingContext::pushAndSetRasterization(const RasterizationState& state) {
	pushRasterization();
	setRasterization(state);
}

void RenderingContext::setRasterization(const RasterizationState& state) {
	internal->pipelineState.setRasterizationState(state);
}

// Scissor ************************************************************************************

const ScissorParameters& RenderingContext::getScissor() const {
	return internal->currentScissorParameters;
}
void RenderingContext::popScissor() {
	if(internal->scissorParametersStack.empty()) {
		WARN("popScissor: Empty scissor parameters stack");
		return;
	}
	setScissor(internal->scissorParametersStack.top());
	internal->scissorParametersStack.pop();
}

void RenderingContext::pushScissor() {
	internal->scissorParametersStack.emplace(getScissor());
}

void RenderingContext::pushAndSetScissor(const ScissorParameters& scissorParameters) {
	pushScissor();
	setScissor(scissorParameters);
}

static const Uniform::UniformName UNIFORM_SG_SCISSOR_RECT("sg_scissorRect");
static const Uniform::UniformName UNIFORM_SG_SCISSOR_ENABLED("sg_scissorEnabled");

void RenderingContext::setScissor(const ScissorParameters& scissorParameters) {
	internal->currentScissorParameters = scissorParameters;

	if(internal->currentScissorParameters.isEnabled()) {
		const Geometry::Rect_i& scissorRect = internal->currentScissorParameters.getRect();
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
const StencilParameters& RenderingContext::getStencilParamters() const {
	return internal->actualCoreRenderingStatus.getStencilParameters();
}

void RenderingContext::pushAndSetStencil(const StencilParameters& stencilParameter) {
	pushStencil();
	setStencil(stencilParameter);
}

void RenderingContext::popStencil() {
	if(internal->stencilParameterStack.empty()) {
		WARN("popStencil: Empty stencil stack");
		return;
	}
	setStencil(internal->stencilParameterStack.top());
	internal->stencilParameterStack.pop();
}

void RenderingContext::pushStencil() {
	internal->stencilParameterStack.emplace(internal->actualCoreRenderingStatus.getStencilParameters());
}

void RenderingContext::setStencil(const StencilParameters& stencilParameter) {
	internal->actualCoreRenderingStatus.setStencilParameters(stencilParameter);
	if(immediate)
		applyChanges();
}

void RenderingContext::clearStencil(int32_t clearValue) {
	glClearStencil(clearValue);
	glClear(GL_STENCIL_BUFFER_BIT);
}

// FBO ************************************************************************************

FBO * RenderingContext::getActiveFBO() const {
	return internal->activeFBO.get();
}

void RenderingContext::popFBO() {
	if(internal->fboStack.empty()) {
		WARN("popFBO: Empty FBO-Stack");
		return;
	}
	setFBO(internal->fboStack.top().get());
	internal->fboStack.pop();
}

void RenderingContext::pushFBO() {
	internal->fboStack.emplace(getActiveFBO());
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
	internal->activeFBO = fbo;
}

// GLOBAL UNIFORMS ***************************************************************************
void RenderingContext::setGlobalUniform(const Uniform& u) {
	internal->globalUniforms.setUniform(u, false, false);
	if(immediate)
		applyChanges();	
}
const Uniform& RenderingContext::getGlobalUniform(const Util::StringIdentifier& uniformName) {
	return internal->globalUniforms.getUniform(uniformName);
}

// SHADER ************************************************************************************
void RenderingContext::setShader(Shader * shader) {
	if(shader) {
		if(shader->_enable()) {
			internal->setActiveRenderingStatus(shader->getRenderingStatus());
			if (!internal->getActiveRenderingStatus()->isInitialized()) { // this shader has not yet been initialized.
				applyChanges(true); // make sure that all uniforms are initially set (e.g. even for disabled lights)
				internal->getActiveRenderingStatus()->markInitialized();
				//				std::cout << " !!!! FORCED !!! \n";
			}
		} else {
			WARN("RenderingContext::pushShader: can't enable shader, using OpenGL instead");
			internal->setActiveRenderingStatus(&(internal->openGLRenderingStatus));
			glUseProgram(0);
		}
	} else {
		internal->setActiveRenderingStatus(&(internal->openGLRenderingStatus));

		glUseProgram(0);
	}
	if(immediate)
		applyChanges();
}

void RenderingContext::pushShader() {
	internal->renderingDataStack.emplace(internal->getActiveRenderingStatus());
}

void RenderingContext::pushAndSetShader(Shader * shader) {
	pushShader();
	setShader(shader);
}

void RenderingContext::popShader() {
	if(internal->renderingDataStack.empty()) {
		WARN("popShader: Empty Shader-Stack");
		return;
	}
	setShader(internal->renderingDataStack.top()->getShader());
	internal->setActiveRenderingStatus(internal->renderingDataStack.top());
	internal->renderingDataStack.pop();

	if(immediate)
		applyChanges();
}

bool RenderingContext::isShaderEnabled(Shader * shader) {
	return shader == internal->getActiveRenderingStatus()->getShader();
}

Shader * RenderingContext::getActiveShader() {
	return internal->getActiveRenderingStatus()->getShader();
}

const Shader * RenderingContext::getActiveShader() const {
	return internal->getActiveRenderingStatus()->getShader();
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

void RenderingContext::_setUniformOnShader(Shader * shader, const Uniform& uniform, bool warnIfUnused, bool forced) {
	shader->_getUniformRegistry()->setUniform(uniform, warnIfUnused, forced);
	if(immediate && getActiveShader() == shader)
		shader->applyUniforms(false); // forced is false here, as this forced means to re-apply all uniforms
}

// TEXTURES **********************************************************************************

Texture * RenderingContext::getTexture(uint8_t unit)const {
	return unit < MAX_TEXTURES ? internal->actualCoreRenderingStatus.getTexture(unit).get() : nullptr;
}

TexUnitUsageParameter RenderingContext::getTextureUsage(uint8_t unit)const{
	return internal->targetRenderingStatus.getTextureUnitParams(unit).first;
}

void RenderingContext::pushTexture(uint8_t unit) {
	internal->textureStacks.at(unit).emplace(getTexture(unit),getTextureUsage(unit));
}

void RenderingContext::pushAndSetTexture(uint8_t unit, Texture * texture) {
	pushAndSetTexture(unit, texture, TexUnitUsageParameter::TEXTURE_MAPPING);
}

void RenderingContext::pushAndSetTexture(uint8_t unit, Texture * texture, TexUnitUsageParameter usage) {
	pushTexture(unit);
	setTexture(unit, texture, usage);
}

void RenderingContext::popTexture(uint8_t unit) {
	if(internal->textureStacks.at(unit).empty()) {
		WARN("popTexture: Empty Texture-Stack");
		return;
	}
	const auto& textureAndUsage = internal->textureStacks[unit].top();
	setTexture(unit, textureAndUsage.first.get(), textureAndUsage.second );
	internal->textureStacks[unit].pop();
}

void RenderingContext::setTexture(uint8_t unit, Texture * texture) {
	setTexture(unit, texture, TexUnitUsageParameter::TEXTURE_MAPPING);
}

void RenderingContext::setTexture(uint8_t unit, Texture * texture, TexUnitUsageParameter usage) {
	Texture * oldTexture = getTexture(unit);
	if(texture != oldTexture) {
		if(texture) 
			texture->_prepareForBinding(*this);
		internal->actualCoreRenderingStatus.setTexture(unit, texture);
	}
	const auto oldUsage = internal->targetRenderingStatus.getTextureUnitParams(unit).first;
	if(!texture){
		if( oldUsage!= TexUnitUsageParameter::DISABLED )
			internal->targetRenderingStatus.setTextureUnitParams(unit, TexUnitUsageParameter::DISABLED , oldTexture ? oldTexture->getTextureType() : TextureType::TEXTURE_2D);
	}else if( oldUsage!= usage ){
		internal->targetRenderingStatus.setTextureUnitParams(unit, usage , texture->getTextureType());
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
	return internal->activeFeedbackStatus.first.get();
}

void RenderingContext::popTransformFeedbackBufferStatus(){
	if(internal->feedbackStack.empty()) {
		WARN("popTransformFeedbackBufferStatus: The stack is empty.");
	}else{
		stopTransformFeedback();
		internal->activeFeedbackStatus = internal->feedbackStack.top();
		_startTransformFeedback(internal->activeFeedbackStatus.second);
	}
}
void RenderingContext::pushTransformFeedbackBufferStatus(){
	internal->feedbackStack.emplace(internal->activeFeedbackStatus);
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
	internal->activeFeedbackStatus.first = buffer;
	_startTransformFeedback(internal->activeFeedbackStatus.second); // restart
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
	internal->activeFeedbackStatus.second = primitiveMode;
}
void RenderingContext::startTransformFeedback_lines()		{	_startTransformFeedback(GL_LINES);	}
void RenderingContext::startTransformFeedback_points()		{	_startTransformFeedback(GL_POINTS);	}
void RenderingContext::startTransformFeedback_triangles()	{	_startTransformFeedback(GL_TRIANGLES);	}
void RenderingContext::stopTransformFeedback()				{	_startTransformFeedback(0);	}

// LIGHTS ************************************************************************************

uint8_t RenderingContext::enableLight(const LightParameters& light) {
	if(internal->targetRenderingStatus.getNumEnabledLights() >= RenderingStatus::MAX_LIGHTS) {
		WARN("Cannot enable more lights; ignoring call.");
		return 255;
	}
	const uint8_t lightNumber = internal->targetRenderingStatus.enableLight(light);
	if(immediate)
		applyChanges();
	return lightNumber;
}

void RenderingContext::disableLight(uint8_t lightNumber) {
	if (!internal->targetRenderingStatus.isLightEnabled(lightNumber)) {
		WARN("Cannot disable an already disabled light; ignoring call.");
		return;
	}
	internal->targetRenderingStatus.disableLight(lightNumber);
	if(immediate)
		applyChanges();
}

// PROJECTION MATRIX *************************************************************************

void RenderingContext::popMatrix_cameraToClipping() {
	if(internal->projectionMatrixStack.empty()) {
		WARN("Cannot pop projection matrix. The stack is empty.");
		return;
	}
	internal->targetRenderingStatus.setMatrix_cameraToClipping(internal->projectionMatrixStack.top());
	internal->projectionMatrixStack.pop();
	if(immediate)
		applyChanges();
}

void RenderingContext::pushMatrix_cameraToClipping() {
	internal->projectionMatrixStack.emplace(internal->targetRenderingStatus.getMatrix_cameraToClipping());
}

void RenderingContext::pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix) {
	pushMatrix_cameraToClipping();
	setMatrix_cameraToClipping(matrix);
}
	
void RenderingContext::setMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix) {
	internal->targetRenderingStatus.setMatrix_cameraToClipping(matrix);
	if(immediate)
		applyChanges();
}

const Geometry::Matrix4x4& RenderingContext::getMatrix_cameraToClipping() const {
	return internal->targetRenderingStatus.getMatrix_cameraToClipping();
}

// CAMERA MATRIX *****************************************************************************

void RenderingContext::setMatrix_cameraToWorld(const Geometry::Matrix4x4& matrix) {
	internal->targetRenderingStatus.setMatrix_cameraToWorld(matrix);
	if(immediate)
		applyChanges();
}
const Geometry::Matrix4x4& RenderingContext::getMatrix_worldToCamera() const {
	return internal->targetRenderingStatus.getMatrix_worldToCamera();
}
const Geometry::Matrix4x4& RenderingContext::getMatrix_cameraToWorld() const {
	return internal->targetRenderingStatus.getMatrix_cameraToWorld();
}

// MODEL VIEW MATRIX *************************************************************************

void RenderingContext::resetMatrix() {
	internal->targetRenderingStatus.setMatrix_modelToCamera(internal->targetRenderingStatus.getMatrix_worldToCamera());
	if(immediate)
		applyChanges();
}


void RenderingContext::pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	pushMatrix_modelToCamera();
	setMatrix_modelToCamera(matrix);
}

const Geometry::Matrix4x4& RenderingContext::getMatrix_modelToCamera() const {
	return internal->targetRenderingStatus.getMatrix_modelToCamera();
}

void RenderingContext::pushMatrix_modelToCamera() {
	internal->matrixStack.emplace(internal->targetRenderingStatus.getMatrix_modelToCamera());
}

void RenderingContext::multMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	internal->targetRenderingStatus.multModelViewMatrix(matrix);
	if(immediate)
		applyChanges();
}

void RenderingContext::setMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	internal->targetRenderingStatus.setMatrix_modelToCamera(matrix);
	if(immediate)
		applyChanges();
}

void RenderingContext::popMatrix_modelToCamera() {
	if(internal->matrixStack.empty()) {
		WARN("Cannot pop matrix. The stack is empty.");
		return;
	}
	internal->targetRenderingStatus.setMatrix_modelToCamera(internal->matrixStack.top());
	internal->matrixStack.pop();
	if(immediate)
		applyChanges();
}

// MATERIAL **********************************************************************************


const MaterialParameters& RenderingContext::getMaterial() const {
	return internal->targetRenderingStatus.getMaterialParameters();
}

void RenderingContext::popMaterial() {
	if(internal->materialStack.empty()) {
		WARN("RenderingContext.popMaterial: stack empty, ignoring call");
		FAIL();
		return;
	}
	internal->materialStack.pop();
	if(internal->materialStack.empty()) {
		internal->targetRenderingStatus.disableMaterial();
	} else {
		internal->targetRenderingStatus.setMaterial(internal->materialStack.top());
	}
	if(immediate)
		applyChanges();
}

void RenderingContext::pushMaterial() {
	internal->materialStack.emplace(internal->targetRenderingStatus.getMaterialParameters());
}
void RenderingContext::pushAndSetMaterial(const MaterialParameters& material) {
	pushMaterial();
	setMaterial(material);
}
void RenderingContext::pushAndSetColorMaterial(const Util::Color4f& color) {
	MaterialParameters material;
	material.setAmbient(color);
	material.setDiffuse(color);
	material.setSpecular(Util::ColorLibrary::BLACK);
	material.enableColorMaterial();
	pushAndSetMaterial(material);
}
void RenderingContext::setMaterial(const MaterialParameters& material) {
	internal->targetRenderingStatus.setMaterial(material);
	if(immediate)
		applyChanges();
}

// VIEWPORT **********************************************************************************

static const Uniform::UniformName UNIFORM_SG_VIEWPORT("sg_viewport");

const Geometry::Rect_i& RenderingContext::getWindowClientArea() const {
	return internal->windowClientArea;
}

const Geometry::Rect_i& RenderingContext::getViewport() const {
	return internal->currentViewport;
}
void RenderingContext::popViewport() {
	if(internal->viewportStack.empty()) {
		WARN("Cannot pop viewport stack because it is empty. Ignoring call.");
		return;
	}
	setViewport(internal->viewportStack.top());
	internal->viewportStack.pop();
}
void RenderingContext::pushViewport() {
	internal->viewportStack.emplace(internal->currentViewport);
}
void RenderingContext::setViewport(const Geometry::Rect_i& viewport) {
	internal->currentViewport = viewport;
	glViewport(internal->currentViewport.getX(), internal->currentViewport.getY(), internal->currentViewport.getWidth(), internal->currentViewport.getHeight());

	std::vector<int> vp;
	vp.push_back(internal->currentViewport.getX());
	vp.push_back(internal->currentViewport.getY());
	vp.push_back(internal->currentViewport.getWidth());
	vp.push_back(internal->currentViewport.getHeight());
	setGlobalUniform(Uniform(UNIFORM_SG_VIEWPORT, vp));
}

void RenderingContext::pushAndSetViewport(const Geometry::Rect_i& viewport) {
	pushViewport();
	setViewport(viewport);
}

void RenderingContext::setWindowClientArea(const Geometry::Rect_i& clientArea) {
	internal->windowClientArea = clientArea;
}

// VBO Client States **********************************************************************************

void RenderingContext::enableClientState(uint32_t clientState) {
	internal->activeClientStates.emplace(clientState);
#ifdef LIB_GL
	glEnableClientState(clientState);
#endif /* LIB_GL */
}

void RenderingContext::disableAllClientStates() {
	while(!internal->activeClientStates.empty()) {
#ifdef LIB_GL
		glDisableClientState(internal->activeClientStates.top());
#endif /* LIB_GL */
		internal->activeClientStates.pop();
	}
}

void RenderingContext::enableTextureClientState(uint32_t textureUnit) {
	internal->activeTextureClientStates.emplace(textureUnit);
#ifdef LIB_GL
	glClientActiveTexture(textureUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif /* LIB_GL */
}

void RenderingContext::disableAllTextureClientStates() {
	while(!internal->activeTextureClientStates.empty()) {
#ifdef LIB_GL
		glClientActiveTexture(internal->activeTextureClientStates.top());
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif /* LIB_GL */
		internal->activeTextureClientStates.pop();
	}
}

void RenderingContext::enableVertexAttribArray(const VertexAttribute& attr, const uint8_t * data, int32_t stride) {
	Shader * shader = getActiveShader();
	GLint location = shader->getVertexAttributeLocation(attr.getNameId());
	if(location != -1) {
		GLuint attribLocation = static_cast<GLuint> (location);
		internal->activeVertexAttributeBindings.emplace(attribLocation);
		if( attr.getConvertToFloat() ){
			glVertexAttribPointer(attribLocation, attr.getNumValues(), attr.getDataType(), attr.getNormalize() ? GL_TRUE : GL_FALSE, stride, data + attr.getOffset());
		} else {
			glVertexAttribIPointer(attribLocation, attr.getNumValues(), attr.getDataType(), stride, data + attr.getOffset());
		}
		glEnableVertexAttribArray(attribLocation);
	}
}

void RenderingContext::disableAllVertexAttribArrays() {
	while(!internal->activeVertexAttributeBindings.empty()) {
		glDisableVertexAttribArray(internal->activeVertexAttributeBindings.top());
		internal->activeVertexAttributeBindings.pop();
	}
}

}
