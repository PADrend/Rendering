/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Pipeline.h"
#include "Device.h"
#include "PipelineCache.h"
#include "../FBO.h"
#include "../Shader/Shader.h"

#include <Util/Utils.h>
#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

namespace Rendering {

vk::Format getVkFormat(const InternalFormat& format);

//---------------

static vk::PipelineInputAssemblyStateCreateInfo convertInputAssemblyState(const InputAssemblyState& state) {
	vk::PipelineInputAssemblyStateCreateInfo info{};
	switch(state.getTopology()) {
		case PrimitiveTopology::PointList: info.topology = vk::PrimitiveTopology::ePointList; break;
		case PrimitiveTopology::LineList: info.topology = vk::PrimitiveTopology::eLineList; break;
		case PrimitiveTopology::LineStrip: info.topology = vk::PrimitiveTopology::eLineStrip; break;
		case PrimitiveTopology::TriangleList: info.topology = vk::PrimitiveTopology::eTriangleList; break;
		case PrimitiveTopology::TriangleStrip: info.topology = vk::PrimitiveTopology::eTriangleStrip; break;
		case PrimitiveTopology::TriangleFan: info.topology = vk::PrimitiveTopology::eTriangleFan; break;
		case PrimitiveTopology::LineListWithAdjacency: info.topology = vk::PrimitiveTopology::eLineListWithAdjacency; break;
		case PrimitiveTopology::LineStripWithAdjacency: info.topology = vk::PrimitiveTopology::eLineStripWithAdjacency; break;
		case PrimitiveTopology::TriangleListWithAdjacency: info.topology = vk::PrimitiveTopology::eTriangleListWithAdjacency; break;
		case PrimitiveTopology::TriangleStripWithAdjacency: info.topology = vk::PrimitiveTopology::eTriangleStripWithAdjacency; break;
		case PrimitiveTopology::PatchList: info.topology = vk::PrimitiveTopology::ePatchList; break;
	}
	info.primitiveRestartEnable = state.isPrimitiveRestartEnabled();
	return info;
}

//---------------

static vk::PipelineRasterizationStateCreateInfo convertRasterizationState(const RasterizationState& state) {
	vk::PipelineRasterizationStateCreateInfo info{{},
		state.isDepthClampEnabled(), state.isRasterizerDiscardEnabled(), 
		{}, {}, {}, 
		state.isDepthBiasEnabled(), state.getDepthBiasConstantFactor(), state.getDepthBiasClamp(), state.getDepthBiasSlopeFactor(), 
		state.getLineWidth()
	};
	switch(state.getPolygonMode()) {
		case PolygonMode::Fill: info.polygonMode = vk::PolygonMode::eFill; break;
		case PolygonMode::Line: info.polygonMode = vk::PolygonMode::eLine; break;
		case PolygonMode::Point: info.polygonMode = vk::PolygonMode::ePoint; break;
	}	
	switch(state.getCullMode()) {
		case CullMode::None: info.cullMode = vk::CullModeFlagBits::eNone; break;
		case CullMode::Front: info.cullMode = vk::CullModeFlagBits::eFront; break;
		case CullMode::Back: info.cullMode = vk::CullModeFlagBits::eBack; break;
		case CullMode::FrontAndBack: info.cullMode = vk::CullModeFlagBits::eFrontAndBack; break;
	}	
	switch(state.getFrontFace()) {
		case FrontFace::CounterClockwise: info.frontFace = vk::FrontFace::eCounterClockwise; break;
		case FrontFace::Clockwise: info.frontFace = vk::FrontFace::eClockwise; break;
	}
	return info;
}

//---------------

static vk::CompareOp convertCompareOp(const ComparisonFunc& op) {
	switch(op) {
		case ComparisonFunc::Never: return vk::CompareOp::eNever;
		case ComparisonFunc::Less: return vk::CompareOp::eLess;
		case ComparisonFunc::Equal: return vk::CompareOp::eEqual;
		case ComparisonFunc::LessOrEqual: return vk::CompareOp::eLessOrEqual;
		case ComparisonFunc::Greater: return vk::CompareOp::eGreater;
		case ComparisonFunc::NotEqual: return vk::CompareOp::eNotEqual;
		case ComparisonFunc::GreaterOrEqual: return vk::CompareOp::eGreaterOrEqual;
		case ComparisonFunc::Always: return vk::CompareOp::eAlways;
	}
	return vk::CompareOp::eNever;
}

//---------------

static vk::StencilOp convertStencilOp(const StencilOp& op) {
	switch(op) {
		case StencilOp::Keep: return vk::StencilOp::eKeep;
		case StencilOp::Zero: return vk::StencilOp::eZero;
		case StencilOp::Replace: return vk::StencilOp::eReplace;
		case StencilOp::IncrementAndClamp: return vk::StencilOp::eIncrementAndClamp;
		case StencilOp::DecrementAndClamp: return vk::StencilOp::eDecrementAndClamp;
		case StencilOp::Invert: return vk::StencilOp::eInvert;
		case StencilOp::IncrementAndWrap: return vk::StencilOp::eIncrementAndWrap;
		case StencilOp::DecrementAndWrap: return vk::StencilOp::eDecrementAndWrap;
	}
	return vk::StencilOp::eKeep;
}

//---------------

static vk::StencilOpState convertStencilOpState(const StencilOpState& state) {
	return {
		convertStencilOp(state.failOp), convertStencilOp(state.passOp), convertStencilOp(state.depthFailOp),
		convertCompareOp(state.compareOp), state.compareMask, state.writeMask, state.reference
	};
}

//---------------

static vk::PipelineDepthStencilStateCreateInfo convertDepthStencilState(const DepthStencilState& state) {
	return {{},
		state.isDepthTestEnabled(), state.isDepthWriteEnabled(), convertCompareOp(state.getDepthCompareOp()),
		state.isDepthBoundsTestEnabled(), state.isStencilTestEnabled(), {}, {},
		state.getMinDepthBounds(), state.getMaxDepthBounds()
	};
}

//---------------

static vk::LogicOp convertLogicOp(const LogicOp& op) {
	switch(op) {
		case LogicOp::Clear: return vk::LogicOp::eClear;
		case LogicOp::And: return vk::LogicOp::eAnd;
		case LogicOp::AndReverse: return vk::LogicOp::eAndReverse;
		case LogicOp::Copy: return vk::LogicOp::eCopy;
		case LogicOp::AndInverted: return vk::LogicOp::eAndInverted;
		case LogicOp::NoOp: return vk::LogicOp::eNoOp;
		case LogicOp::Xor: return vk::LogicOp::eXor;
		case LogicOp::Or: return vk::LogicOp::eOr;
		case LogicOp::Nor: return vk::LogicOp::eNor;
		case LogicOp::Equivalent: return vk::LogicOp::eEquivalent;
		case LogicOp::Invert: return vk::LogicOp::eInvert;
		case LogicOp::OrReverse: return vk::LogicOp::eOrReverse;
		case LogicOp::CopyInverted: return vk::LogicOp::eCopyInverted;
		case LogicOp::OrInverted: return vk::LogicOp::eOrInverted;
		case LogicOp::Nand: return vk::LogicOp::eNand;
		case LogicOp::Set: return vk::LogicOp::eSet;
	}
	return vk::LogicOp::eNoOp;
}

//---------------

static vk::BlendFactor convertBlendFactor(const BlendFactor& op) {
	switch(op) {
		case BlendFactor::Zero: return vk::BlendFactor::eZero;
		case BlendFactor::One: return vk::BlendFactor::eOne;
		case BlendFactor::SrcColor: return vk::BlendFactor::eSrcColor;
		case BlendFactor::OneMinusSrcColor: return vk::BlendFactor::eOneMinusSrcColor;
		case BlendFactor::DstColor: return vk::BlendFactor::eDstColor;
		case BlendFactor::OneMinusDstColor: return vk::BlendFactor::eOneMinusDstColor;
		case BlendFactor::SrcAlpha: return vk::BlendFactor::eSrcAlpha;
		case BlendFactor::OneMinusSrcAlpha: return vk::BlendFactor::eOneMinusSrcAlpha;
		case BlendFactor::DstAlpha: return vk::BlendFactor::eDstAlpha;
		case BlendFactor::OneMinusDstAlpha: return vk::BlendFactor::eOneMinusDstAlpha;
		case BlendFactor::ConstantColor: return vk::BlendFactor::eConstantColor;
		case BlendFactor::OneMinusConstantColor: return vk::BlendFactor::eOneMinusConstantColor;
		case BlendFactor::ConstantAlpha: return vk::BlendFactor::eConstantAlpha;
		case BlendFactor::OneMinusConstantAlpha: return vk::BlendFactor::eOneMinusConstantAlpha;
		case BlendFactor::SrcAlphaSaturate: return vk::BlendFactor::eSrcAlphaSaturate;
		case BlendFactor::Src1Color: return vk::BlendFactor::eSrc1Color;
		case BlendFactor::OneMinusSrc1Color: return vk::BlendFactor::eOneMinusSrc1Color;
		case BlendFactor::Src1Alpha: return vk::BlendFactor::eSrc1Alpha;
		case BlendFactor::OneMinusSrc1Alpha: return vk::BlendFactor::eOneMinusSrc1Alpha;
	}
	return vk::BlendFactor::eZero;
}

//---------------

static vk::BlendOp convertBlendOp(const BlendOp& op) {
	switch(op) {
		case BlendOp::Add: return vk::BlendOp::eAdd;
		case BlendOp::Subtract: return vk::BlendOp::eSubtract;
		case BlendOp::ReverseSubtract: return vk::BlendOp::eReverseSubtract;
		case BlendOp::Min: return vk::BlendOp::eMin;
		case BlendOp::Max: return vk::BlendOp::eMax;
	}
	return vk::BlendOp::eAdd;
}

//---------------

static vk::PipelineColorBlendAttachmentState convertColorBlendAttachmentState(const ColorBlendAttachmentState& state) {
	return {
		state.blendEnable,
		convertBlendFactor(state.srcColorBlendFactor), convertBlendFactor(state.dstColorBlendFactor), convertBlendOp(state.colorBlendOp),
		convertBlendFactor(state.srcAlphaBlendFactor), convertBlendFactor(state.dstAlphaBlendFactor), convertBlendOp(state.alphaBlendOp),
		static_cast<vk::ColorComponentFlags>(state.colorWriteMask)
	};
}

//---------------

Pipeline::~Pipeline() = default;

//---------------

Pipeline::Pipeline(PipelineType type, const PipelineState& state, const Ref& parent) : type(type), state(state), parent(parent) { }

//---------------

bool Pipeline::initGraphics(const PipelineCacheRef& cache) {
	if(type != PipelineType::Graphics)
		return false;

	auto shader = state.getShader();
	if(!shader || !shader->init())
		return false;

	auto fbo = state.getFBO();
	if(!fbo || !fbo->validate())
		return false;
	
	hash = state.getHash();
	
	// Create new pipeline
	vk::Device vkDevice(cache->getApiHandle());
	vk::PipelineCache vkCache(cache->getApiHandle());

	vk::GraphicsPipelineCreateInfo info{};
	info.layout = shader->getPipelineLayout();
	
	// Convert shader stages
	std::vector<vk::PipelineShaderStageCreateInfo> stages;
	for(auto& e : shader->getShaderModules()) {
		vk::PipelineShaderStageCreateInfo stage{};
		switch(e.first) {
			case ShaderStage::Vertex: stage.stage = vk::ShaderStageFlagBits::eVertex; break;
			case ShaderStage::TessellationControl: stage.stage = vk::ShaderStageFlagBits::eTessellationControl; break;
			case ShaderStage::TessellationEvaluation: stage.stage = vk::ShaderStageFlagBits::eTessellationEvaluation; break;
			case ShaderStage::Geometry: stage.stage = vk::ShaderStageFlagBits::eGeometry; break;
			case ShaderStage::Fragment: stage.stage = vk::ShaderStageFlagBits::eFragment; break;
			case ShaderStage::Compute: stage.stage = vk::ShaderStageFlagBits::eCompute; break;
			default:
				continue;
		}
		stage.module = e.second;
		stage.pName = state.getEntryPoint().c_str();
		stage.pSpecializationInfo = nullptr; // TODO
		stages.emplace_back(stage);
	}
	info.stageCount = stages.size();
	info.pStages = stages.data();

	// Convert vertex input state
	std::vector<vk::VertexInputBindingDescription> bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
	for(auto& b : state.getVertexInputState().getBindings())
		bindings.emplace_back(b.binding, b.stride, static_cast<vk::VertexInputRate>(b.inputRate));
	for(auto& a : state.getVertexInputState().getAttributes())
		attributes.emplace_back(a.location, a.binding, static_cast<vk::Format>(getVkFormat(a.format)), a.offset);
	vk::PipelineVertexInputStateCreateInfo vertexInput{{}, 
		static_cast<uint32_t>(bindings.size()), bindings.data(), 
		static_cast<uint32_t>(attributes.size()), attributes.data()
	};
	info.pVertexInputState = &vertexInput;

	// Convert input assembly state
	auto inputAssembly = convertInputAssemblyState(state.getInputAssemblyState());
	info.pInputAssemblyState = &inputAssembly;

	// Convert viewport state
	std::vector<vk::Viewport> viewports;
	std::vector<vk::Rect2D> scissors;
	for(auto& v : state.getViewportState().getViewports())
		viewports.emplace_back(v.rect.getX(), v.rect.getY(), v.rect.getWidth(), v.rect.getHeight(), v.minDepth, v.maxDepth);
	for(auto& s : state.getViewportState().getScissors())
		scissors.emplace_back(vk::Rect2D{{static_cast<int32_t>(s.getX()), static_cast<int32_t>(s.getY())}, {static_cast<uint32_t>(s.getWidth()), static_cast<uint32_t>(s.getHeight())}});
	vk::PipelineViewportStateCreateInfo viewport{{}, 
		static_cast<uint32_t>(viewports.size()), viewports.data(), 
		static_cast<uint32_t>(scissors.size()), scissors.data()
	};
	info.pViewportState = &viewport;

	// Convert rasterization state
	auto rasterization = convertRasterizationState(state.getRasterizationState());
	info.pRasterizationState = &rasterization;

	// Convert multisample state
	auto& ms = state.getMultisampleState();
	vk::SampleMask sampleMask = ms.getSampleMask();
	vk::PipelineMultisampleStateCreateInfo multisample{{}, 
		static_cast<vk::SampleCountFlagBits>(ms.getSampleCount()), 
		ms.isSampleShadingEnabled(), ms.getMinSampleShading(), &sampleMask,
		ms.isAlphaToCoverageEnabled(), ms.isAlphaToOneEnabled()
	};
	info.pMultisampleState = &multisample;
	
	// Convert depth/stencil state
	auto depthStencil = convertDepthStencilState(state.getDepthStencilState());
	info.pDepthStencilState = &depthStencil;

	// Convert blend state
	auto& bs = state.getColorBlendState();
	std::vector<vk::PipelineColorBlendAttachmentState> attachments;
	for(uint32_t i=0; i<cache->getDevice()->getMaxFramebufferAttachments(); ++i) {
		if(i<bs.getAttachmentCount())
			attachments.emplace_back(convertColorBlendAttachmentState(bs.getAttachment(i)));
		else
			attachments.emplace_back(convertColorBlendAttachmentState({}));
	}
	vk::PipelineColorBlendStateCreateInfo colorBlend{{},
		bs.isLogicOpEnabled(), convertLogicOp(bs.getLogicOp()),
		static_cast<uint32_t>(attachments.size()), attachments.data(),
		{bs.getConstantColor().getR(), bs.getConstantColor().getG(), bs.getConstantColor().getB(), bs.getConstantColor().getA()}
	};
	info.pColorBlendState = &colorBlend;

	// Convert dynamic state
	std::vector<vk::DynamicState> dynamic;
	if(state.getViewportState().hasDynamicViewports()) dynamic.emplace_back(vk::DynamicState::eViewport);
	if(state.getViewportState().hasDynamicScissors()) dynamic.emplace_back(vk::DynamicState::eScissor);
	if(state.getRasterizationState().hasDynamicLineWidth()) dynamic.emplace_back(vk::DynamicState::eLineWidth);
	if(state.getRasterizationState().hasDynamicDepthBias()) dynamic.emplace_back(vk::DynamicState::eDepthBias);
	if(state.getColorBlendState().hasDynamicConstantColor()) dynamic.emplace_back(vk::DynamicState::eBlendConstants);
	if(state.getDepthStencilState().hasDynamicDepthBounds()) dynamic.emplace_back(vk::DynamicState::eDepthBounds);
	if(state.getDepthStencilState().hasDynamicCompareMask()) dynamic.emplace_back(vk::DynamicState::eStencilCompareMask);
	if(state.getDepthStencilState().hasDynamicWriteMask()) dynamic.emplace_back(vk::DynamicState::eStencilWriteMask);
	if(state.getDepthStencilState().hasDynamicReference()) dynamic.emplace_back(vk::DynamicState::eStencilReference);
	vk::PipelineDynamicStateCreateInfo dynamicState{{}, static_cast<uint32_t>(dynamic.size()), dynamic.data()};
	info.pDynamicState = &dynamicState;

	// Render pass
	info.renderPass = fbo->getRenderPass();
	info.subpass = 0; // TODO: allow multiple subpasses

	// Derived pipeline
	if(parent)
		info.basePipelineHandle = parent->handle;

	// Create pipeline
	handle = std::move(PipelineHandle(vkDevice.createGraphicsPipeline(vkCache, info), vkDevice));
	
	return handle;
}

//---------------

bool Pipeline::initCompute(const PipelineCacheRef& cache) {
	if(type != PipelineType::Compute)
		return false;

	auto shader = state.getShader();
	if(!shader || !shader->init())
		return false;
	
	if(state.getEntryPoint().empty())
		return false;

	hash = 0;
	Util::hash_combine(hash, shader->getLayoutHash());
	Util::hash_combine(hash, state.getEntryPoint());

	// Create new pipeline
	vk::Device vkDevice(cache->getApiHandle());
	vk::PipelineCache vkCache(cache->getApiHandle());

	vk::ComputePipelineCreateInfo info{};
	info.layout = shader->getPipelineLayout();
	info.stage.stage = vk::ShaderStageFlagBits::eCompute;
	info.stage.pName = state.getEntryPoint().c_str();
	info.stage.pSpecializationInfo = nullptr; // TODO
	auto moduleEntry = shader->getShaderModules().find(ShaderStage::Compute);	
	if(moduleEntry == shader->getShaderModules().end() || !moduleEntry->second)
		return false;
	info.stage.module = moduleEntry->second;

	// Derived pipeline
	if(parent)
		info.basePipelineHandle = parent->handle;
		
	// Create pipeline
	handle = std::move(PipelineHandle(vkDevice.createComputePipeline(vkCache, info), vkDevice));	
	return handle;
}

//---------------

bool Pipeline::init(const PipelineCacheRef& cache) {
	switch(type) {
		case PipelineType::Graphics: return initGraphics(cache);
		case PipelineType::Compute: return initCompute(cache);
	}
	return false;
}

//---------------

} /* Rendering */