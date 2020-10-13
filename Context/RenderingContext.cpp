/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "RenderingContext.h"
#include "RenderThread.h"

#include "../State/BindingState.h"
#include "../State/PipelineState.h"
#include "../State/RenderingState.h"
#include "../Core/Device.h"
#include "../Core/CommandBuffer.h"
#include "../Core/Swapchain.h"
#include "../Core/Queue.h"
#include "../Core/ImageView.h"
#include "../Core/ImageStorage.h"
#include "../Core/BufferStorage.h"
#include "../Core/Sampler.h"
#include "../RenderingContext/RenderingParameters.h"
#include "../Buffer/BufferObject.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexDescription.h"
#include "../Mesh/VertexAccessor.h"
#include "../Shader/Shader.h"
#include "../Shader/UniformRegistry.h"
#include "../Shader/UniformBuffer.h"
#include "../Texture/Texture.h"
#include "../FBO.h"
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
#include <thread>

//#define PROFILING_ENABLED 1
#include <Util/Profiling/Profiler.h>
#include <Util/Profiling/Logger.h>
#ifdef PROFILING_ENABLED
#include <fstream>

static std::ofstream _out("RCLog.txt", std::fstream::out);
INIT_PROFILING_TIME(_out);
static uint64_t _profilingCount = 0;
#define PROFILING_CONDITION _profilingCount==1000
#endif


namespace Rendering {

static const Util::StringIdentifier DUMMY_VERTEX_ATTR("dummy");

class RenderingContext::InternalData {
public:
	DeviceRef device;
	RenderingState renderingState;
	CommandBuffer::Ref cmd;
	uint64_t submissionIndex = 0;
	const uint64_t maxPendingSubmissions = 100;

	// pipeline state stacks
	std::stack<VertexInputState> vertexInputStack;
	std::stack<InputAssemblyState> inputAssemblyStack;
	std::stack<ViewportState> viewportStack;
	std::stack<RasterizationState> rasterizationStack;
	std::stack<MultisampleState> multisampleStack;
	std::stack<DepthStencilState> depthStencilStack;
	std::stack<ColorBlendState> colorBlendStack;

	// binding stacks
	std::map<std::pair<uint32_t,uint32_t>, std::stack<Texture::Ref>> textureStacks;
	std::unordered_map<uint32_t, std::stack<ImageView::Ref>> imageStacks;
	
	// transformation stack
	std::stack<Geometry::Matrix4x4> modelToCameraStack;
	std::stack<Geometry::Matrix4x4> cameraToClippingStack;

	// materials
	std::stack<MaterialData> materialStack;

	// fbo
	std::stack<FBO::Ref> fboStack;
	Geometry::Rect_i windowClientArea;

	// shader
	std::stack<Shader::Ref> shaderStack;
	Shader::Ref activeShader;
	Shader::Ref fallbackShader;
	UniformRegistry globalUniforms;

	// dummy vertex buffer
	MeshVertexData fallbackVertexBuffer;
	Texture::Ref dummyTexture;

	std::vector<BufferObjectRef> activeVBOs;
	BufferObject::Ref activeIBO;

	// deprecated
	std::stack<AlphaTestParameters> alphaTestParameterStack;
	std::stack<PointParameters> pointParameterStack;
};

RenderingContext::RenderingContext(const DeviceRef& device) :
	internal(new InternalData), displayMeshFn() {

	resetDisplayMeshFn();

	internal->device = device;
	
	internal->fallbackShader = ShaderUtils::createDefaultShader(device);
	internal->fallbackShader->init();

	internal->cmd = CommandBuffer::create(device->getQueue(QueueFamily::Graphics));

	// Initialize dummy vertex buffer
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	vd.appendTexCoord();
	vd.appendFloat(DUMMY_VERTEX_ATTR, 4, false);

	internal->fallbackVertexBuffer.allocate(1, vd);
	{
		auto acc = VertexAccessor::create(internal->fallbackVertexBuffer);
		acc->setPosition(0, {0,0,0});
		acc->setNormal(0, {0,1,0});
		acc->setColor(0, Util::Color4f(1,1,1,1));
		acc->setTexCoord(0, {0,0});
	}
	internal->fallbackVertexBuffer.upload(MemoryUsage::GpuOnly);
	internal->fallbackVertexBuffer.releaseLocalData();

	// Initialize dummy texture
	ImageFormat format{};
	format.extent = {1,1,1};
	internal->dummyTexture = Texture::create(device, format);
	internal->dummyTexture->allocateLocalData();
	internal->dummyTexture->clear({1,1,1,1});
	internal->dummyTexture->upload(ResourceUsage::ShaderResource);
	setTexture(0, nullptr);

	//// Initially enable the depth test.
	internal->cmd->getPipeline().getDepthStencilState().setDepthTestEnabled(true);

	// Set dynamic state
	internal->cmd->getPipeline().getViewportState().setDynamicScissors(true);
	internal->cmd->getPipeline().getRasterizationState().setDynamicLineWidth(true);

	setFBO(nullptr);

	MaterialData tmp;
	tmp.setShadingModel(ShadingModel::Shadeless);
	setMaterial(tmp);

	Geometry::Rect_i windowRect{0, 0, static_cast<int32_t>(device->getWindow()->getWidth()), static_cast<int32_t>(device->getWindow()->getHeight())};
	setViewport({windowRect}, {windowRect});
	setWindowClientArea(windowRect);

	// set default camera matrix
	setMatrix_cameraToClipping(Geometry::Matrix4x4::orthographicProjection(-1, 1, -1, 1, -1, 1));

	applyChanges();
}

RenderingContext::RenderingContext() : RenderingContext(Device::getDefault()) {}

RenderingContext::~RenderingContext() {
	#ifdef PROFILING_ENABLED
		_out.flush();
	#endif
	RenderThread::sync(internal->submissionIndex);
	internal->device->waitIdle();
};

void RenderingContext::resetDisplayMeshFn() {
	using namespace std::placeholders;
	displayMeshFn = std::bind(&Rendering::Mesh::_display, _2, _1, _3, _4);
}

void RenderingContext::displayMesh(Mesh * mesh) {
	displayMeshFn(*this, mesh,0,mesh->isUsingIndexData()? mesh->getIndexCount() : mesh->getVertexCount());
}

const DeviceRef& RenderingContext::getDevice() const {
	return internal->device;
}

CommandBufferRef RenderingContext::getCommandBuffer() const {
	return internal->cmd;
}

const PipelineState& RenderingContext::getPipelineState() const {
	return internal->cmd->getPipeline();
}

const RenderingState& RenderingContext::getRenderingState() const{
	return internal->renderingState;
}
// helper ***************************************************************************

void RenderingContext::flush(bool wait) {
	SCOPED_PROFILING_COND("flush", PROFILING_CONDITION);
	if(internal->cmd->getCommandCount() == 0)
		return;

	auto cmd = internal->cmd;
	RenderThread::addTask([&,cmd]() {
		cmd->submit(wait);
	});

	auto newCmd = CommandBuffer::create(internal->device->getQueue(QueueFamily::Graphics), true);
	newCmd->setBindings(cmd->getBindings());
	newCmd->setPipeline(cmd->getPipeline());
	newCmd->setFBO(cmd->getFBO());
	internal->cmd = newCmd;

	internal->activeVBOs.clear();
	internal->activeIBO = nullptr;
	applyChanges();
}

void RenderingContext::present() {
	internal->cmd->prepareForPresent();
	flush();
	internal->submissionIndex = RenderThread::addTask([&]() {
		internal->device->present();
	});
	if(internal->submissionIndex - RenderThread::getProcessed() > internal->maxPendingSubmissions) {
		RenderThread::sync(internal->submissionIndex);
	}

	END_PROFILING_COND("RenderingContext", PROFILING_CONDITION);
	BEGIN_PROFILING_COND("RenderingContext", ++PROFILING_CONDITION);

	// reset rendering state
	// TODO: do explicit clearing?
	internal->renderingState.getLights().clear();
	internal->renderingState.getInstance().markDirty();
}


void RenderingContext::barrier(uint32_t flags) {
	applyChanges();
	WARN("RenderingContext: barrier() is currrently not supported");
}


// Applying changes ***************************************************************************

void RenderingContext::applyChanges(bool forced) {
	SCOPED_PROFILING_COND("applyChanges", PROFILING_CONDITION);

	// Set the shader
	Shader::Ref shader;
	if(!internal->activeShader || !internal->activeShader->init()) {
		// if there is no active shader, use the fallback
		shader = internal->fallbackShader;
	} else {
		shader = internal->activeShader;
	}

	if(shader != internal->cmd->getShader()) {
		// Shader changed: force apply
		internal->renderingState.apply(shader, true);
	} else {
		// apply rendering state
		internal->renderingState.apply(shader, forced);
	}
	internal->cmd->setShader(shader);

	// transfer updated global uniforms to the shader
	shader->_getUniformRegistry()->performGlobalSync(internal->globalUniforms, false);

	// apply uniforms
	shader->applyUniforms(forced);

	// bind uniform buffers
	for(auto& b : shader->getUniformBuffers()) {
		b.second->bind(internal->cmd, b.first.second, b.first.first);
	}
}

// Clear ***************************************************************************

void RenderingContext::clearColor(const Util::Color4f& color) {
	applyChanges();
	internal->cmd->setClearColor({color});
	internal->cmd->clear(true, false, false);
}

void RenderingContext::clearScreen(const Util::Color4f& color) {
	applyChanges();
	internal->cmd->setClearColor({color});
	internal->cmd->clear(true, true, true);
}

void RenderingContext::clearScreenRect(const Geometry::Rect_i& rect, const Util::Color4f& color, bool _clearDepth, bool _clearStencil) {
	applyChanges();
	internal->cmd->setClearColor({color});
	internal->cmd->clear(true, _clearDepth, _clearStencil, rect);
}

void RenderingContext::clearDepth(float clearValue) {
	applyChanges();
	internal->cmd->setClearDepthValue(clearValue);
	internal->cmd->clear(false, true, false);
}

void RenderingContext::clearStencil(uint32_t clearValue) {
	applyChanges();
	internal->cmd->setClearStencilValue(clearValue);
	internal->cmd->clear(false, false, true);
}

// AlphaTest ************************************************************************************

const AlphaTestParameters RenderingContext::getAlphaTestParameters() const {
	return internal->renderingState.getMaterial().isAlphaMaskEnabled() ? AlphaTestParameters(Comparison::LESS, internal->renderingState.getMaterial().getAlphaThreshold()) : AlphaTestParameters();
}

void RenderingContext::popAlphaTest() {
	WARN_AND_RETURN_IF(internal->alphaTestParameterStack.empty(), "popAlphaTest: Empty AlphaTest-Stack",);
	setAlphaTest(internal->alphaTestParameterStack.top());
	internal->alphaTestParameterStack.pop();
}

void RenderingContext::pushAlphaTest() {
	internal->alphaTestParameterStack.emplace(getAlphaTestParameters());
}

void RenderingContext::pushAndSetAlphaTest(const AlphaTestParameters & p) {
	pushAlphaTest();
	setAlphaTest(p);
}

void RenderingContext::setAlphaTest(const AlphaTestParameters & p) {
	if(p.isEnabled()) {
		WARN_IF(p.getMode() != Comparison::LESS, "setAlphaTest: Only Comparison::LESS is supported.");
		internal->renderingState.getMaterial().setAlphaMaskEnabled(true);
		internal->renderingState.getMaterial().setAlphaThreshold(p.getReferenceValue());
	} else {
		internal->renderingState.getMaterial().setAlphaMaskEnabled(false);
	}
}

// Blending ************************************************************************************

const BlendingParameters RenderingContext::getBlendingParameters() const {
	return BlendingParameters(internal->cmd->getPipeline().getColorBlendState());
}

const ColorBlendState& RenderingContext::getBlending() const {
	return internal->cmd->getPipeline().getColorBlendState();
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
	internal->colorBlendStack.emplace(internal->cmd->getPipeline().getColorBlendState());
}

void RenderingContext::setBlending(const BlendingParameters& p) {
	auto& state = internal->cmd->getPipeline().getColorBlendState();
	state = p.toBlendState().setColorWriteMask(state.getColorWriteMask());
}

void RenderingContext::setBlending(const ColorBlendState& s) {
	internal->cmd->getPipeline().setColorBlendState(s);
}


// ClipPlane ************************************************************************************

const ClipPlaneParameters RenderingContext::getClipPlane(uint8_t index) const { return {}; }


// ColorBuffer ************************************************************************************
const ColorBufferParameters RenderingContext::getColorBufferParameters() const {
	return ColorBufferParameters(internal->cmd->getPipeline().getColorBlendState().getColorWriteMask());
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
	internal->cmd->getPipeline().getColorBlendState().setColorWriteMask(p.getWriteMask());
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
const CullFaceParameters RenderingContext::getCullFaceParameters() const {
	return internal->cmd->getPipeline().getRasterizationState().getCullMode();
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
	internal->cmd->getPipeline().getRasterizationState().setCullMode(p.getCullMode());
}


// DepthStencil ************************************************************************************

const DepthStencilState& RenderingContext::getDepthStencil() const {
	return internal->cmd->getPipeline().getDepthStencilState();
}

void RenderingContext::popDepthStencil() {
	WARN_AND_RETURN_IF(internal->depthStencilStack.empty(), "popDepthStencil: Empty DepthStencil stack",);
	setDepthStencil(internal->depthStencilStack.top());
	internal->depthStencilStack.pop();
}

void RenderingContext::pushDepthStencil() {
	internal->depthStencilStack.emplace(internal->cmd->getPipeline().getDepthStencilState());
}

void RenderingContext::pushAndSetDepthStencil(const DepthStencilState& state) {
	pushDepthStencil();
	setDepthStencil(state);
}

void RenderingContext::setDepthStencil(const DepthStencilState& state) {
	internal->cmd->getPipeline().setDepthStencilState(state);
}

// DepthBuffer ************************************************************************************
const DepthBufferParameters RenderingContext::getDepthBufferParameters() const {
	auto state = internal->cmd->getPipeline().getDepthStencilState();
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
	auto& state = internal->cmd->getPipeline().getDepthStencilState();
	state.setDepthTestEnabled(p.isTestEnabled());
	state.setDepthWriteEnabled(p.isWritingEnabled());
	state.setDepthCompareOp(p.isTestEnabled() ? Comparison::functionToComparisonFunc(p.getFunction()) : ComparisonFunc::Disabled);
}

// Drawing ************************************************************************************

void RenderingContext::bindVertexBuffer(const BufferObjectRef& buffer, const VertexDescription& vd) {
	bindVertexBuffers({buffer}, {vd});
}


void RenderingContext::bindVertexBuffers(const std::vector<BufferObjectRef>& buffers, const std::vector<VertexDescription>& vds, const std::vector<uint32_t> rates) {
	SCOPED_PROFILING_COND("bindVertexBuffers", PROFILING_CONDITION);
	auto shader = internal->activeShader ? internal->activeShader : internal->fallbackShader;
	WARN_AND_RETURN_IF(!shader, "bindVertexBuffers: There is no bound shader.",);
	WARN_AND_RETURN_IF(buffers.size() != vds.size(), "bindVertexBuffers: Number of vertex descriptions does not match number of buffers.",);
	std::vector<uint32_t> inputRates(rates.begin(), rates.end());
	uint32_t bindingCount = static_cast<uint32_t>(buffers.size());

	std::vector<BufferObjectRef> boundBuffers(buffers.begin(), buffers.end());
	if(inputRates.size() < bindingCount)
		inputRates.resize(bindingCount, 0);
	
	const auto& fallbackVD = internal->fallbackVertexBuffer.getVertexDescription();
	VertexInputState state;
	bool hasUnusedAttributes = false;
	for(auto& location : shader->getVertexAttributeLocations()) {
		bool attrFound = false;
		for(uint32_t binding=0; binding<bindingCount; ++binding) {
			auto attr = vds[binding].getAttribute(location.first);
			if(!attr.empty()) {
				state.setAttribute({static_cast<uint32_t>(location.second), binding, toInternalFormat(attr), attr.getOffset()});
				attrFound = true;
				break;
			}
		}
		if(!attrFound) {
			// bind default attribute
			auto fallbackAttr = fallbackVD.getAttribute(location.first);
			if(fallbackAttr.empty())
				fallbackAttr = fallbackVD.getAttribute(DUMMY_VERTEX_ATTR);
			state.setAttribute({static_cast<uint32_t>(location.second), bindingCount, toInternalFormat(fallbackAttr), fallbackAttr.getOffset()});
			hasUnusedAttributes = true;
		}
	}

	for(uint32_t binding=0; binding<bindingCount; ++binding)
		state.setBinding({binding, static_cast<uint32_t>(vds[binding].getVertexSize()), inputRates[binding]});

	if(hasUnusedAttributes) {
		state.setBinding({bindingCount, 0, 1});
		boundBuffers.emplace_back(internal->fallbackVertexBuffer.getBuffer());
	}
	internal->cmd->getPipeline().setVertexInputState(state);
	if(boundBuffers != internal->activeVBOs) {
		internal->cmd->bindVertexBuffers(0, boundBuffers);
		internal->activeVBOs = boundBuffers;
	}
}

void RenderingContext::bindIndexBuffer(const BufferObjectRef& buffer) {
	if(buffer != internal->activeIBO) {
		internal->cmd->bindIndexBuffer(buffer);
		internal->activeIBO = buffer;
	}
}

void RenderingContext::draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount, uint32_t firstInstance) {
	applyChanges();
	internal->cmd->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderingContext::drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t instanceCount, uint32_t firstInstance) {
	applyChanges();
	internal->cmd->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void RenderingContext::drawIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride) {
	applyChanges();
	internal->cmd->drawIndirect(buffer, drawCount, stride);
}

void RenderingContext::setPrimitiveTopology(PrimitiveTopology topology) {
	internal->cmd->getPipeline().getInputAssemblyState().setTopology(topology);
}

// FBO ************************************************************************************

FBO * RenderingContext::getActiveFBO() const {
	return internal->cmd->getFBO().get();
}

FBORef RenderingContext::getFBO() const {
	return internal->cmd->getFBO();
}

void RenderingContext::popFBO() {
	WARN_AND_RETURN_IF(internal->fboStack.empty(), "popFBO: Empty FBO-Stack",);
	setFBO(internal->fboStack.top().get());
	internal->fboStack.pop();
}

void RenderingContext::pushFBO() {
	internal->fboStack.emplace(internal->cmd->getFBO());
}

void RenderingContext::pushAndSetFBO(const FBORef& fbo) {
	pushFBO();
	setFBO(fbo);
}

void RenderingContext::setFBO(const FBORef& fbo) {
	internal->cmd->setFBO(fbo);
}

// ImageBinding ************************************************************************************

ImageBindParameters RenderingContext::getBoundImage(uint8_t unit, uint8_t set) const {
	/*auto image = internal->bindingState.getBoundInputImage(set, unit, 0);
	ImageBindParameters p;
	if(!image)
		return p;
	p.setTexture(Texture::create(internal->device, image).detachAndDecrease());
	p.setLayer(image->getLayer());
	p.setLevel(image->getMipLevel());
	p.setMultiLayer(image->getLayerCount() > 1);
	auto usage = image->getImage()->getConfig().usage;
	p.setReadOperations(usage == ResourceUsage::ShaderResource || usage == ResourceUsage::ShaderWrite || usage == ResourceUsage::General);
	p.setWriteOperations(usage == ResourceUsage::ShaderWrite || usage == ResourceUsage::General);
	return p;*/
	return {};
}

void RenderingContext::pushBoundImage(uint8_t unit, uint8_t set) {
	//auto image = internal->bindingState.getBoundInputImage(set, unit, 0);
	//internal->imageStacks[unit].emplace(image);
}

void RenderingContext::pushAndSetBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set) {
	pushBoundImage(unit);
	setBoundImage(unit,iParam);
}

void RenderingContext::popBoundImage(uint8_t unit, uint8_t set) {
	//WARN_AND_RETURN_IF(internal->imageStacks[unit].empty(), "popBoundImage: Empty stack",);
	//internal->bindingState.bindInputImage(internal->imageStacks[unit].top(), set, unit, 0);
	//internal->imageStacks[unit].pop();
}

//! \note the texture in iParam may be null to unbind
void RenderingContext::setBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set) {
	//ImageView::Ref view = iParam.getTexture() ? iParam.getTexture()->getImageView() : nullptr; 
	//internal->bindingState.bindInputImage(view, set, unit, 0);
	// TODO: transfer image to correct usage type
}
	
// Lighting ************************************************************************************
const LightingParameters RenderingContext::getLightingParameters() const { return LightingParameters(true); }

size_t RenderingContext::enableLight(const LightParameters& _light) {
	LightData light;
	switch(_light.type) {
		case LightParameters::POINT:
			light.setType(LightType::Point);
			break;
		case LightParameters::DIRECTIONAL:
			light.setType(LightType::Directional);
			break;
		case LightParameters::SPOT:
			light.setType(LightType::Spot);
			break;
	}
	light.setPosition(_light.position);
	light.setDirection(_light.direction);
	light.setIntensity(_light.diffuse);
	light.setConeAngle(Geometry::Angle::deg(_light.cutoff));

	if(light.getType() != LightType::Directional) {
		// 0 = q*x^2 + l*x + (c - 1/a)
		float attThreshold = 0.01f;
		float tmp = _light.linear *_light.linear - 4.0 * _light.quadratic * (_light.constant - 1.0/attThreshold);
		// x = (-l +- sqrt(l*l - 4*q*(c-1/a)))/2q
	}

	return enableLight(light);
}

size_t RenderingContext::enableLight(const LightData& light) {
	return internal->renderingState.getLights().addLight(light);
}

void RenderingContext::disableLight(size_t lightId) {
	internal->renderingState.getLights().removeLight(lightId);
}

// Line ************************************************************************************
const LineParameters RenderingContext::getLineParameters() const {
	return {internal->cmd->getPipeline().getRasterizationState().getLineWidth()};
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
	internal->cmd->setLineWidth(p.getWidth());
}

// Point ************************************************************************************
const PointParameters RenderingContext::getPointParameters() const { 
	return {internal->renderingState.getInstance().getPointSize()};
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
	internal->renderingState.getInstance().setPointSize(p.getSize());
}

// PolygonMode ************************************************************************************
const PolygonModeParameters RenderingContext::getPolygonModeParameters() const {
	return PolygonModeParameters(internal->cmd->getPipeline().getRasterizationState().getPolygonMode());
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
	auto state = internal->cmd->getPipeline().getRasterizationState();
	state.setPolygonMode(PolygonModeParameters::modeToPolygonMode(p.getMode()));
	internal->cmd->getPipeline().setRasterizationState(state);
}

// PolygonOffset ************************************************************************************
const PolygonOffsetParameters RenderingContext::getPolygonOffsetParameters() const {
	const auto& state = internal->cmd->getPipeline().getRasterizationState();
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
	internal->cmd->getPipeline().getRasterizationState()
		.setDepthBiasEnabled(p.isEnabled())
		.setDepthBiasConstantFactor(p.getUnits())
		.setDepthBiasSlopeFactor(p.getFactor());
}

// PrimitiveRestart ************************************************************************************
const PrimitiveRestartParameters RenderingContext::getPrimitiveRestartParameters() const {
	const auto& state = internal->cmd->getPipeline().getInputAssemblyState();
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
	internal->cmd->getPipeline().getInputAssemblyState().setPrimitiveRestartEnabled(p.isEnabled());
}

// Rasterization ************************************************************************************

const RasterizationState& RenderingContext::getRasterization() const {
	return internal->cmd->getPipeline().getRasterizationState();
}

void RenderingContext::popRasterization() {
	WARN_AND_RETURN_IF(internal->rasterizationStack.empty(), "popRasterization: Empty Rasterization stack",);
	setRasterization(internal->rasterizationStack.top());
	internal->rasterizationStack.pop();
}

void RenderingContext::pushRasterization() {
	internal->rasterizationStack.emplace(internal->cmd->getPipeline().getRasterizationState());
}

void RenderingContext::pushAndSetRasterization(const RasterizationState& state) {
	pushRasterization();
	setRasterization(state);
}

void RenderingContext::setRasterization(const RasterizationState& state) {
	internal->cmd->getPipeline().setRasterizationState(state);
}

// Scissor ************************************************************************************

const ScissorParameters RenderingContext::getScissor() const {
	const auto& state = internal->cmd->getPipeline().getViewportState();
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
	Geometry::Rect_i scissor = scissorParameters.isEnabled() ? scissorParameters.getRect() : internal->cmd->getPipeline().getViewportState().getViewport().rect;
	internal->cmd->setScissor(scissor);
}

// Stencil ************************************************************************************
const StencilParameters RenderingContext::getStencilParamters() const {
	auto state = internal->cmd->getPipeline().getDepthStencilState();
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
	internal->cmd->getPipeline().getDepthStencilState()
		.setStencilTestEnabled(p.isEnabled())
		.setFront(p.getStencilOpState())
		.setBack(p.getStencilOpState());
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
	} else {
		internal->activeShader = shader;
	}
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

const ShaderRef& RenderingContext::getFallbackShader() const {
	return internal->fallbackShader;
}

void RenderingContext::_setUniformOnShader(const ShaderRef& shader, const Uniform& uniform, bool warnIfUnused, bool forced) {
	shader->_getUniformRegistry()->setUniform(uniform, warnIfUnused, forced);
}

// TEXTURES **********************************************************************************

const TextureRef RenderingContext::getTexture(uint32_t unit, uint32_t set) const {
	return internal->cmd->getBindings().getBinding(set, unit, 0).getTexture();
}

TexUnitUsageParameter RenderingContext::getTextureUsage(uint32_t unit) const {
	return TexUnitUsageParameter::TEXTURE_MAPPING;
}

void RenderingContext::pushTexture(uint32_t unit, uint32_t set) {
	internal->textureStacks[std::make_pair(unit,set)].emplace(getTexture(unit, set));
}

void RenderingContext::pushAndSetTexture(uint32_t unit, const TextureRef& texture, uint32_t set) {
	pushTexture(unit, set);
	setTexture(unit, texture, set);
}

void RenderingContext::popTexture(uint32_t unit, uint32_t set) {
	WARN_AND_RETURN_IF(internal->textureStacks[std::make_pair(unit,set)].empty(), "popTexture: Empty Texture-Stack",);
	setTexture(unit, internal->textureStacks[std::make_pair(unit,set)].top(), set);
	internal->textureStacks[std::make_pair(unit,set)].pop();
}

void RenderingContext::setTexture(uint32_t unit, const TextureRef& texture, uint32_t set) {
	if(texture)
		texture->upload();
	if(texture.isNotNull())
		internal->cmd->bindTexture(texture, set, unit, 0);
	else
		internal->cmd->bindTexture(internal->dummyTexture, set, unit, 0);
}

// PROJECTION MATRIX *************************************************************************

void RenderingContext::popMatrix_cameraToClipping() {
	WARN_AND_RETURN_IF(internal->cameraToClippingStack.empty(), "Cannot pop projection matrix. The stack is empty.",);
	setMatrix_cameraToClipping(internal->cameraToClippingStack.top());
	internal->cameraToClippingStack.pop();
}

void RenderingContext::pushMatrix_cameraToClipping() {
	internal->cameraToClippingStack.emplace(getMatrix_cameraToClipping());
}

void RenderingContext::pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix) {
	pushMatrix_cameraToClipping();
	setMatrix_cameraToClipping(matrix);
}
	
void RenderingContext::setMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix) {
	internal->renderingState.getCamera().setMatrixCameraToClipping(matrix);
}

const Geometry::Matrix4x4& RenderingContext::getMatrix_cameraToClipping() const {
	return internal->renderingState.getCamera().getMatrixCameraToClipping();
}

// CAMERA MATRIX *****************************************************************************

void RenderingContext::setMatrix_cameraToWorld(const Geometry::Matrix4x4& matrix) {
	internal->renderingState.getCamera().setMatrixCameraToWorld(matrix);
}
const Geometry::Matrix4x4& RenderingContext::getMatrix_worldToCamera() const {
	return internal->renderingState.getCamera().getMatrixWorldToCamera();
}
const Geometry::Matrix4x4& RenderingContext::getMatrix_cameraToWorld() const {
	return internal->renderingState.getCamera().getMatrixCameraToWorld();
}

// MODEL VIEW MATRIX *************************************************************************

void RenderingContext::resetMatrix() {
	internal->renderingState.getInstance().setMatrixModelToCamera(internal->renderingState.getCamera().getMatrixCameraToWorld());
}


void RenderingContext::pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	pushMatrix_modelToCamera();
	setMatrix_modelToCamera(matrix);
}

const Geometry::Matrix4x4& RenderingContext::getMatrix_modelToCamera() const {
	return internal->renderingState.getInstance().getMatrixModelToCamera();
}

void RenderingContext::pushMatrix_modelToCamera() {
	internal->modelToCameraStack.emplace(getMatrix_modelToCamera());
}

void RenderingContext::multMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	internal->renderingState.getInstance().multMatrixModelToCamera(matrix);
}

void RenderingContext::setMatrix_modelToCamera(const Geometry::Matrix4x4& matrix) {
	internal->renderingState.getInstance().setMatrixModelToCamera(matrix);
}

void RenderingContext::popMatrix_modelToCamera() {
	WARN_AND_RETURN_IF(internal->modelToCameraStack.empty(), "Cannot pop matrix. The stack is empty.",);
	setMatrix_modelToCamera(internal->modelToCameraStack.top());
	internal->modelToCameraStack.pop();
}

// MATERIAL **********************************************************************************


const MaterialData& RenderingContext::getActiveMaterial() const {
	return internal->renderingState.getMaterial();
}

const MaterialParameters RenderingContext::getMaterial() const {
	// convert from MaterialData
	MaterialParameters material;
	material.setAmbient(internal->renderingState.getMaterial().getAmbient());
	material.setDiffuse(internal->renderingState.getMaterial().getDiffuse());
	material.setSpecular(internal->renderingState.getMaterial().getSpecular());
	material.setEmission(internal->renderingState.getMaterial().getEmission());
	if(internal->renderingState.getMaterial().getShadingModel() == ShadingModel::Shadeless)
		material.enableColorMaterial();
	return material;
}

void RenderingContext::popMaterial() {
	WARN_AND_RETURN_IF(internal->materialStack.empty(), "Cannot pop material. The stack is empty.",);
	internal->materialStack.pop();
	if(internal->materialStack.empty()) {
		MaterialData tmp;
		tmp.setShadingModel(ShadingModel::Shadeless);
		setMaterial(tmp);
	} else {
		setMaterial(internal->materialStack.top());
	}
}

void RenderingContext::pushMaterial() {
	internal->materialStack.emplace(internal->renderingState.getMaterial());
}

void RenderingContext::pushAndSetMaterial(const MaterialData& material) {
	pushMaterial();
	setMaterial(material);
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

void RenderingContext::setMaterial(const MaterialParameters& _material) {
	// convert to MaterialData
	MaterialData material;
	material.setAmbient(_material.getAmbient());
	material.setDiffuse(_material.getDiffuse());
	material.setSpecular({_material.getSpecular().r(), _material.getSpecular().g(), _material.getSpecular().b(), _material.getShininess()});
	material.setEmission(_material.getEmission());
	material.setShadingModel(_material.getColorMaterial() ? ShadingModel::Shadeless : ShadingModel::Phong);
	setMaterial(material);
}

void RenderingContext::setMaterial(const MaterialData& material) {
	internal->renderingState.setMaterial(material);
}

// VIEWPORT **********************************************************************************

const Geometry::Rect_i& RenderingContext::getWindowClientArea() const {
	return internal->windowClientArea;
}

const Geometry::Rect_i& RenderingContext::getViewport() const {
	return internal->cmd->getPipeline().getViewportState().getViewport().rect;
}

const ViewportState& RenderingContext::getViewportState() const {
	return internal->cmd->getPipeline().getViewportState();
}

void RenderingContext::popViewport() {
	WARN_AND_RETURN_IF(internal->viewportStack.empty(), "Cannot pop viewport stack because it is empty. Ignoring call.",);
	setViewport(internal->viewportStack.top());
	internal->viewportStack.pop();
}

void RenderingContext::pushViewport() {
	internal->viewportStack.emplace(internal->cmd->getPipeline().getViewportState());
}

void RenderingContext::setViewport(const Geometry::Rect_i& viewport) {
	internal->cmd->getPipeline().getViewportState().setViewport(viewport);
}

void RenderingContext::setViewport(const Geometry::Rect_i& viewport, const Geometry::Rect_i& scissor) {
	 internal->cmd->getPipeline().getViewportState().setViewport(viewport);
	 internal->cmd->setScissor(scissor);
}

void RenderingContext::setViewport(const ViewportState& viewport) {
	internal->cmd->getPipeline().setViewportState(viewport);
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
