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
#include "Core/ImageView.h"
#include "Core/ImageStorage.h"
#include "Core/BufferStorage.h"
#include "Core/Sampler.h"

#include "RenderingContext/RenderingParameters.h"
#include "BufferObject.h"
#include "Mesh/Mesh.h"
#include "Mesh/VertexDescription.h"
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
#include <Util/UI/Window.h>
#include <unordered_map>
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

		// binding stacks
		std::unordered_map<uint32_t, std::stack<Texture::Ref>> textureStacks;
		std::unordered_map<uint32_t, std::stack<ImageView::Ref>> imageStacks;
		
		// transformation stack
		std::stack<Geometry::Matrix4x4> matrixStack;
		std::stack<Geometry::Matrix4x4> projectionMatrixStack;

		// lights & materials
		std::stack<LightingParameters> lightingParameterStack;
		std::stack<MaterialParameters> materialStack;
		std::stack<PointParameters> pointParameterStack;

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

	Geometry::Rect_i windowRect{0, 0, static_cast<int32_t>(device->getWindow()->getWidth()), static_cast<int32_t>(device->getWindow()->getHeight())};
	setViewport({windowRect}, {windowRect});

	applyChanges();
}

RenderingContext::RenderingContext() : RenderingContext(Device::getDefault()) {}

RenderingContext::~RenderingContext() = default;

void RenderingContext::resetDisplayMeshFn() {
	using namespace std::placeholders;
	displayMeshFn = std::bind(&Rendering::Mesh::_display, _2, _1, _3, _4);
}

void RenderingContext::displayMesh(Mesh * mesh) {
	displayMeshFn(*this, mesh,0,mesh->isUsingIndexData()? mesh->getIndexCount() : mesh->getVertexCount());
}


CommandBufferRef RenderingContext::getCommandBuffer() const {
	return internal->cmd;
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
	if(internal->cmd->isInRenderPass() && internal->cmd->getFBO() != internal->pipelineState.getFBO()) {
		// FBO has changed: end the active render pass
		internal->cmd->endRenderPass();
	}

	// Update state
	internal->cmd->setPipelineState(internal->pipelineState);
	internal->cmd->setBindings(internal->bindingState);

	// Set the shader
	if(!internal->activeShader || !internal->activeShader->init()) {
		// if there is no active shader, use the fallback
		internal->cmd->setShader(internal->fallbackShader);
	} else {
		internal->cmd->setShader(internal->activeShader);
	}

	// if there is no active FBO, use the swapchain FBO
	if(!internal->pipelineState.getFBO())
		internal->cmd->setFBO(internal->device->getSwapchain()->getCurrentFBO());

	auto shader = internal->cmd->getShader();

	// transfer updated global uniforms to the shader
	shader->_getUniformRegistry()->performGlobalSync(internal->globalUniforms, false);

	// apply uniforms
	shader->applyUniforms(forced);

	// TODO: set dynamic state
}

// Clear ***************************************************************************

void RenderingContext::clearColor(const Util::Color4f& color) {
	applyChanges();
	if(!internal->cmd->isInRenderPass())
		internal->cmd->beginRenderPass();
	internal->cmd->clearColor({color});
}

void RenderingContext::clearScreen(const Util::Color4f& color) {
	applyChanges();
	if(!internal->cmd->isInRenderPass())
		internal->cmd->beginRenderPass();
	internal->cmd->clearColor({color});
	internal->cmd->clearDepthStencil(1, 0);
}

void RenderingContext::clearScreenRect(const Geometry::Rect_i& rect, const Util::Color4f& color, bool _clearDepth) {
	applyChanges();
	if(!internal->cmd->isInRenderPass())
		internal->cmd->beginRenderPass();
	internal->cmd->clearColor({color}, rect);
	if(_clearDepth)
		internal->cmd->clearDepthStencil(1, 0, rect);
}

void RenderingContext::clearDepth(float clearValue) {
	applyChanges();
	if(!internal->cmd->isInRenderPass())
		internal->cmd->beginRenderPass();
	internal->cmd->clearDepthStencil(clearValue, 0, {}, true, false);
}

void RenderingContext::clearStencil(int32_t clearValue) {
	applyChanges();
	if(!internal->cmd->isInRenderPass())
		internal->cmd->beginRenderPass();
	internal->cmd->clearDepthStencil(1, clearValue, {}, false, true);
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
	WARN_AND_RETURN_IF(internal->colorBlendStack.empty(), "popBlending: Empty Blending-Stack",);
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

// Compute ************************************************************************************

void RenderingContext::dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) {
	applyChanges();
	WARN("dispatchCompute: Compute shaders are not supported.");
}

void RenderingContext::dispatchComputeIndirect(size_t offset) {
	applyChanges();
	WARN("dispatchComputeIndirect: Compute shaders are not supported.");
}

void RenderingContext::loadUniformSubroutines(uint32_t shaderStage, const std::vector<uint32_t>& indices) {
	WARN("loadUniformSubroutines: Uniform subroutines are not supported.");
}

void RenderingContext::loadUniformSubroutines(uint32_t shaderStage, const std::vector<std::string>& names) {
	WARN("loadUniformSubroutines: Uniform subroutines are not supported.");
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
	WARN_AND_RETURN_IF(internal->depthStencilStack.empty(), "popDepthStencil: Empty DepthStencil stack",);
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

// Drawing ************************************************************************************

void RenderingContext::bindVertexBuffer(const BufferObjectRef& buffer, const VertexDescription& vd) {
	auto shader = internal->activeShader ? internal->activeShader : internal->fallbackShader;
	WARN_AND_RETURN_IF(!shader, "There is no bound shader.",);

	VertexInputState state;	
	state.setBinding({0, static_cast<uint32_t>(vd.getVertexSize()), 0});
	for(auto& attr : vd.getAttributes()) {
		auto location = shader->getVertexAttributeLocation(attr.getNameId());
		if(location != -1) {
			state.setAttribute({static_cast<uint32_t>(location), 0, toInternalFormat(attr), attr.getOffset()});
		}
	}
	internal->pipelineState.setVertexInputState(state);

	internal->cmd->bindVertexBuffers(0, {buffer});
}

void RenderingContext::bindIndexBuffer(const BufferObjectRef& buffer) {
	internal->cmd->bindIndexBuffer(buffer);
}

void RenderingContext::draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount, uint32_t firstInstance) {
	applyChanges();
	if(!internal->cmd->isInRenderPass())
		internal->cmd->beginRenderPass();
	internal->cmd->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderingContext::drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t instanceCount, uint32_t firstInstance) {
	applyChanges();
	if(!internal->cmd->isInRenderPass())
		internal->cmd->beginRenderPass();
	internal->cmd->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void RenderingContext::setPrimitiveTopology(PrimitiveTopology topology) {
	auto state = internal->pipelineState.getInputAssemblyState();
	state.setTopology(topology);
	internal->pipelineState.setInputAssemblyState(state);
}

// FBO ************************************************************************************

FBO * RenderingContext::getActiveFBO() const {
	return internal->pipelineState.getFBO().get();
}

FBORef RenderingContext::getFBO() const {
	return internal->pipelineState.getFBO();
}

void RenderingContext::popFBO() {
	WARN_AND_RETURN_IF(internal->fboStack.empty(), "popFBO: Empty FBO-Stack",);
	setFBO(internal->fboStack.top().get());
	internal->fboStack.pop();
}

void RenderingContext::pushFBO() {
	internal->fboStack.emplace(getFBO());
}

void RenderingContext::pushAndSetFBO(const FBORef& fbo) {
	pushFBO();
	setFBO(fbo);
}

void RenderingContext::setFBO(const FBORef& fbo) {
	internal->pipelineState.setFBO(fbo);
}

// ImageBinding ************************************************************************************

ImageBindParameters RenderingContext::getBoundImage(uint8_t unit, uint8_t set) const {
	auto image = internal->bindingState.getBoundInputImage(set, unit, 0);
	ImageBindParameters p;
	if(!image)
		return p;
	p.setTexture(Texture::create(internal->device, image));
	p.setLayer(image->getLayer());
	p.setLevel(image->getMipLevel());
	p.setMultiLayer(image->getLayerCount() > 1);
	auto usage = image->getImage()->getConfig().usage;
	p.setReadOperations(usage == ResourceUsage::ShaderResource || usage == ResourceUsage::ShaderWrite || usage == ResourceUsage::General);
	p.setWriteOperations(usage == ResourceUsage::ShaderWrite || usage == ResourceUsage::General);
	return p;
}

void RenderingContext::pushBoundImage(uint8_t unit, uint8_t set) {
	auto image = internal->bindingState.getBoundInputImage(set, unit, 0);
	internal->imageStacks[unit].emplace(image);
}

void RenderingContext::pushAndSetBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set) {
	pushBoundImage(unit);
	setBoundImage(unit,iParam);
}

void RenderingContext::popBoundImage(uint8_t unit, uint8_t set) {
	WARN_AND_RETURN_IF(internal->imageStacks[unit].empty(), "popBoundImage: Empty stack",);
	internal->bindingState.bindInputImage(internal->imageStacks[unit].top(), set, unit, 0);
	internal->imageStacks[unit].pop();
}

//! \note the texture in iParam may be null to unbind
void RenderingContext::setBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set) {
	ImageView::Ref view = iParam.getTexture() ? iParam.getTexture()->getImageView() : nullptr; 
	internal->bindingState.bindInputImage(view, set, unit, 0);
	// TODO: transfer image to correct usage type
}
	
// Lighting ************************************************************************************
const LightingParameters& RenderingContext::getLightingParameters() const {
	return LightingParameters();
}
void RenderingContext::popLighting() {
	WARN_AND_RETURN_IF(internal->lightingParameterStack.empty(), "popLighting: Empty lighting stack",);
	setLighting(internal->lightingParameterStack.top());
	internal->lightingParameterStack.pop();
}

void RenderingContext::pushLighting() {
	internal->lightingParameterStack.emplace(LightingParameters());
}

void RenderingContext::pushAndSetLighting(const LightingParameters& p) {
	pushLighting();
	setLighting(p);
}

void RenderingContext::setLighting(const LightingParameters& p) {
	// TODO: use uniform buffers
}

// Line ************************************************************************************
const LineParameters& RenderingContext::getLineParameters() const {
	return {internal->pipelineState.getRasterizationState().getLineWidth()};
}

void RenderingContext::popLine() {
	popRasterization();
}

void RenderingContext::pushLine() {
	pushRasterization();
}

void RenderingContext::pushAndSetLine(const LineParameters& p) {
	pushRasterization();
	setLine(p);
}

void RenderingContext::setLine(const LineParameters& p) {
	auto state = internal->pipelineState.getRasterizationState();
	state.setLineWidth(p.getWidth());
	internal->pipelineState.setRasterizationState(state);
}

// Point ************************************************************************************
const PointParameters& RenderingContext::getPointParameters() const {
	return {};
}

void RenderingContext::popPointParameters() {
	WARN_AND_RETURN_IF(internal->pointParameterStack.empty(), "popPoint: Empty point parameters stack",);
	setPointParameters(internal->pointParameterStack.top());
	internal->pointParameterStack.pop();
}

void RenderingContext::pushPointParameters() {
	internal->pointParameterStack.emplace(PointParameters());
}

void RenderingContext::pushAndSetPointParameters(const PointParameters& p) {
	pushPointParameters();
	setPointParameters(p);
}

void RenderingContext::setPointParameters(const PointParameters& p) {
	// TODO: use push constants?
}

// PolygonMode ************************************************************************************
const PolygonModeParameters& RenderingContext::getPolygonModeParameters() const {
	return PolygonModeParameters(internal->pipelineState.getRasterizationState().getPolygonMode());
}

void RenderingContext::popPolygonMode() {
	popRasterization();
}

void RenderingContext::pushPolygonMode() {
	pushRasterization();
}

void RenderingContext::pushAndSetPolygonMode(const PolygonModeParameters& p) {
	pushPolygonMode();
	setPolygonMode(p);
}

void RenderingContext::setPolygonMode(const PolygonModeParameters& p) {
	auto state = internal->pipelineState.getRasterizationState();
	state.setPolygonMode(PolygonModeParameters::modeToPolygonMode(p.getMode()));
	internal->pipelineState.setRasterizationState(state);
}

// PolygonOffset ************************************************************************************
const PolygonOffsetParameters& RenderingContext::getPolygonOffsetParameters() const {
	const auto& state = internal->pipelineState.getRasterizationState();
	PolygonOffsetParameters p(state.getDepthBiasSlopeFactor(), state.getDepthBiasConstantFactor());
	if(!state.isDepthBiasEnabled()) p.disable();
	return p;
}

void RenderingContext::popPolygonOffset() {	
	popRasterization();
}

void RenderingContext::pushPolygonOffset() {
	pushRasterization();
}

void RenderingContext::pushAndSetPolygonOffset(const PolygonOffsetParameters& p) {
	pushPolygonOffset();
	setPolygonOffset(p);
}

void RenderingContext::setPolygonOffset(const PolygonOffsetParameters& p) {
	auto state = internal->pipelineState.getRasterizationState();
	state.setDepthBiasEnabled(p.isEnabled()).setDepthBiasConstantFactor(p.getUnits()).setDepthBiasSlopeFactor(p.getFactor());
	internal->pipelineState.setRasterizationState(state);
}

// PrimitiveRestart ************************************************************************************
const PrimitiveRestartParameters& RenderingContext::getPrimitiveRestartParameters() const {
	const auto& state = internal->pipelineState.getInputAssemblyState();
	return state.isPrimitiveRestartEnabled() ? PrimitiveRestartParameters(0xffffffffu) : PrimitiveRestartParameters();
}

void RenderingContext::popPrimitiveRestart() {
	WARN_AND_RETURN_IF(internal->inputAssemblyStack.empty(), "popPoint: Empty point parameters stack",);
	const auto& state = internal->inputAssemblyStack.top();
	setPrimitiveRestart(state.isPrimitiveRestartEnabled() ? PrimitiveRestartParameters(0xffffffffu) : PrimitiveRestartParameters());
	internal->inputAssemblyStack.pop();
}

void RenderingContext::pushPrimitiveRestart() {
	internal->inputAssemblyStack.emplace(internal->inputAssemblyStack.top());
}

void RenderingContext::pushAndSetPrimitiveRestart(const PrimitiveRestartParameters& p) {
	pushPrimitiveRestart();
	setPrimitiveRestart(p);
}

void RenderingContext::setPrimitiveRestart(const PrimitiveRestartParameters& p) {
	auto state = internal->pipelineState.getInputAssemblyState();
	state.setPrimitiveRestartEnabled(p.isEnabled());
	internal->pipelineState.setInputAssemblyState(state);
}

// Rasterization ************************************************************************************

const RasterizationState& RenderingContext::getRasterization() const {
	internal->pipelineState.getRasterizationState();
}

void RenderingContext::popRasterization() {
	WARN_AND_RETURN_IF(internal->rasterizationStack.empty(), "popRasterization: Empty Rasterization stack",);
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
	const auto& state = internal->pipelineState.getViewportState();
	return (state.getScissor() == state.getViewport().rect) ? ScissorParameters(state.getScissor()) : ScissorParameters();
}

void RenderingContext::popScissor() {
	popViewport();
}

void RenderingContext::pushScissor() {
	pushViewport();
}

void RenderingContext::pushAndSetScissor(const ScissorParameters& scissorParameters) {
	pushViewport();
	setScissor(scissorParameters);
}

void RenderingContext::setScissor(const ScissorParameters& scissorParameters) {
	auto state = internal->pipelineState.getViewportState();
	state.setScissor(scissorParameters.isEnabled() ? scissorParameters.getRect() : state.getViewport().rect);
	internal->pipelineState.setViewportState(state);
}

// Stencil ************************************************************************************
const StencilParameters& RenderingContext::getStencilParamters() const {
	auto state = internal->pipelineState.getDepthStencilState();
	return state.isDepthTestEnabled() ? StencilParameters(state.getFront()) : StencilParameters();
}

void RenderingContext::pushAndSetStencil(const StencilParameters& stencilParameter) {
	pushStencil();
	setStencil(stencilParameter);
}

void RenderingContext::popStencil() {
	popDepthStencil();
}

void RenderingContext::pushStencil() {
	pushDepthStencil();
}

void RenderingContext::setStencil(const StencilParameters& p) {
	auto state = internal->pipelineState.getDepthStencilState();
	state.setStencilTestEnabled(p.isEnabled());
	state.setFront(p.getStencilOpState());
	state.setBack(p.getStencilOpState());
	internal->pipelineState.setDepthStencilState(state);
}

// GLOBAL UNIFORMS ***************************************************************************
void RenderingContext::setGlobalUniform(const Uniform& u) {
	internal->globalUniforms.setUniform(u, false, false);	
}

const Uniform& RenderingContext::getGlobalUniform(const Util::StringIdentifier& uniformName) {
	return internal->globalUniforms.getUniform(uniformName);
}

// SHADER ************************************************************************************
void RenderingContext::setShader(const ShaderRef& shader) {
	if(shader && !shader->init()) {
		WARN("RenderingContext::pushShader: can't enable shader, using fallback instead");
		internal->activeShader = nullptr;
	}
	internal->activeShader = shader;
}

void RenderingContext::pushShader() {
	internal->shaderStack.emplace(internal->activeShader);
}

void RenderingContext::pushAndSetShader(const ShaderRef& shader) {
	pushShader();
	setShader(shader);
}

void RenderingContext::popShader() {
	WARN_AND_RETURN_IF(internal->shaderStack.empty(), "popShader: Empty Shader-Stack",);
	setShader(internal->shaderStack.top());
	internal->shaderStack.pop();
}

bool RenderingContext::isShaderEnabled(const ShaderRef& shader) {
	return shader == internal->activeShader;
}

const ShaderRef& RenderingContext::getActiveShader() const {
	return internal->activeShader;
}

void RenderingContext::_setUniformOnShader(const ShaderRef& shader, const Uniform& uniform, bool warnIfUnused, bool forced) {
	shader->_getUniformRegistry()->setUniform(uniform, warnIfUnused, forced);
}

// TEXTURES **********************************************************************************

const TextureRef& RenderingContext::getTexture(uint8_t unit, uint8_t set) const {
	return internal->bindingState.getBoundTexture(set, unit);
}

TexUnitUsageParameter RenderingContext::getTextureUsage(uint8_t unit) const {
	return TexUnitUsageParameter::TEXTURE_MAPPING;
}

void RenderingContext::pushTexture(uint8_t unit, uint8_t set) {
	internal->textureStacks[unit].emplace(getTexture(unit, set));
}

void RenderingContext::pushAndSetTexture(uint8_t unit, const TextureRef& texture, uint8_t set) {
	pushTexture(unit, set);
	setTexture(unit, texture, set);
}

void RenderingContext::popTexture(uint8_t unit, uint8_t set) {
	WARN_AND_RETURN_IF(internal->textureStacks[unit].empty(), "popTexture: Empty Texture-Stack",);
	setTexture(unit, internal->textureStacks[unit].top() );
	internal->textureStacks[unit].pop();
}

void RenderingContext::setTexture(uint8_t unit, const TextureRef& texture, uint8_t set) {
	if(texture)
		texture->upload();
	internal->bindingState.bindTexture(texture, set, unit, 0);
}

// LIGHTS ************************************************************************************

uint8_t RenderingContext::enableLight(const LightParameters& light) {
	return 0;
}

void RenderingContext::disableLight(uint8_t lightNumber) {

}

// PROJECTION MATRIX *************************************************************************

void RenderingContext::popMatrix_cameraToClipping() {
	WARN_AND_RETURN_IF(internal->projectionMatrixStack.empty(), "Cannot pop projection matrix. The stack is empty.",);
	setMatrix_cameraToClipping(internal->projectionMatrixStack.top());
	internal->projectionMatrixStack.pop();
}

void RenderingContext::pushMatrix_cameraToClipping() {
	internal->projectionMatrixStack.emplace(getMatrix_cameraToClipping());
}

void RenderingContext::pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix) {
	pushMatrix_cameraToClipping();
	setMatrix_cameraToClipping(matrix);
}
	
void RenderingContext::setMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix) {
	
}

const Geometry::Matrix4x4& RenderingContext::getMatrix_cameraToClipping() const {
	return {};
}

// CAMERA MATRIX *****************************************************************************

void RenderingContext::setMatrix_cameraToWorld(const Geometry::Matrix4x4& matrix) {
	
}
const Geometry::Matrix4x4& RenderingContext::getMatrix_worldToCamera() const {
	return {};
}
const Geometry::Matrix4x4& RenderingContext::getMatrix_cameraToWorld() const {
	return {};
}

// MODEL VIEW MATRIX *************************************************************************

void RenderingContext::resetMatrix() {
	
}


void RenderingContext::pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	pushMatrix_modelToCamera();
	setMatrix_modelToCamera(matrix);
}

const Geometry::Matrix4x4& RenderingContext::getMatrix_modelToCamera() const {
	return {};
}

void RenderingContext::pushMatrix_modelToCamera() {
	internal->matrixStack.emplace(getMatrix_modelToCamera());
}

void RenderingContext::multMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	
}

void RenderingContext::setMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	
}

void RenderingContext::popMatrix_modelToCamera() {
	WARN_AND_RETURN_IF(internal->matrixStack.empty(), "Cannot pop matrix. The stack is empty.",);
	setMatrix_modelToCamera(internal->matrixStack.top());
	internal->matrixStack.pop();
}

// MATERIAL **********************************************************************************


const MaterialParameters& RenderingContext::getMaterial() const {
	return MaterialParameters();
}

void RenderingContext::popMaterial() {
	WARN_AND_RETURN_IF(internal->matrixStack.empty(), "Cannot pop material. The stack is empty.",);
	internal->materialStack.pop();
	if(internal->materialStack.empty()) {
		//internal->targetRenderingStatus.disableMaterial();
	} else {
		//internal->targetRenderingStatus.setMaterial(internal->materialStack.top());
	}
}

void RenderingContext::pushMaterial() {
	internal->materialStack.emplace(getMaterial());
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
}

// VIEWPORT **********************************************************************************

const Geometry::Rect_i& RenderingContext::getWindowClientArea() const {
	return internal->windowClientArea;
}

const Geometry::Rect_i& RenderingContext::getViewport() const {
	return internal->pipelineState.getViewportState().getViewport().rect;
}

const ViewportState& RenderingContext::getViewportState() const {
	return internal->pipelineState.getViewportState();
}

void RenderingContext::popViewport() {
	WARN_AND_RETURN_IF(internal->viewportStack.empty(), "Cannot pop viewport stack because it is empty. Ignoring call.",);
	setViewport(internal->viewportStack.top());
	internal->viewportStack.pop();
}

void RenderingContext::pushViewport() {
	internal->viewportStack.emplace(internal->pipelineState.getViewportState());
}

void RenderingContext::setViewport(const Geometry::Rect_i& viewport) {
	auto state = internal->pipelineState.getViewportState();
	auto vp = state.getViewport();
	vp.rect = viewport;
	state.setViewport(vp);
	internal->pipelineState.setViewportState(state);
}

void RenderingContext::setViewport(const Geometry::Rect_i& viewport, const Geometry::Rect_i& scissor) {
	auto state = internal->pipelineState.getViewportState();
	auto vp = state.getViewport();
	vp.rect = viewport;
	state.setViewport(vp);
	state.setScissor(scissor);
	internal->pipelineState.setViewportState(state);
}

void RenderingContext::setViewport(const ViewportState& viewport) {
	internal->pipelineState.setViewportState(viewport);
}

void RenderingContext::pushAndSetViewport(const Geometry::Rect_i& viewport) {
	pushViewport();
	setViewport(viewport);
}

void RenderingContext::pushAndSetViewport(const Geometry::Rect_i& viewport, const Geometry::Rect_i& scissor) {
	pushViewport();
	setViewport(viewport, scissor);
}

void RenderingContext::pushAndSetViewport(const ViewportState& viewport) {
	pushViewport();
	setViewport(viewport);
}

void RenderingContext::setWindowClientArea(const Geometry::Rect_i& clientArea) {
	internal->windowClientArea = clientArea;
}

}
