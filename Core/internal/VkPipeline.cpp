/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Common.h"
#include "../Device.h"
#include "../ResourceCache.h"
#include "../../FBO.h"
#include "../../Shader/Shader.h"
#include "../../State/ShaderLayout.h"
#include "../../State/PipelineState.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

// defined in VkUtils.cpp
vk::Format getVkFormat(const InternalFormat& format);
vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);
vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);

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
		default: return vk::CompareOp::eNever;
	}
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
		default: return vk::LogicOp::eNoOp;
	}
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

static vk::PipelineColorBlendAttachmentState convertColorBlendAttachmentState(const ColorBlendState& state) {
	vk::ColorComponentFlags mask{};
	return {
		state.isBlendingEnabled(),
		convertBlendFactor(state.getSrcColorBlendFactor()), convertBlendFactor(state.getDstColorBlendFactor()), convertBlendOp(state.getColorBlendOp()),
		convertBlendFactor(state.getSrcAlphaBlendFactor()), convertBlendFactor(state.getDstAlphaBlendFactor()), convertBlendOp(state.getAlphaBlendOp()),
		static_cast<vk::ColorComponentFlags>(state.getColorWriteMask())
	};
}

//---------------

ApiBaseHandle::Ref createDescriptorSetLayoutHandle(Device* device, const ShaderResourceLayoutSet& layoutSet) {
	vk::Device vkDevice(device->getApiHandle());
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	for(const auto& it : layoutSet.getLayouts()) {
		auto layout = it.second;
		if(!hasBindingPoint(layout.type))
			continue; // Skip resources whitout a binding point

		vk::DescriptorSetLayoutBinding binding{};
		binding.binding = it.first;
		binding.descriptorCount = layout.elementCount;
		binding.descriptorType = getVkDescriptorType(layout.type, layout.dynamic);
		binding.stageFlags = getVkStageFlags(layout.stages);

		bindings.emplace_back(binding);
	}

	return DescriptorSetLayoutHandle::create(vkDevice.createDescriptorSetLayout({{}, static_cast<uint32_t>(bindings.size()), bindings.data()}), vkDevice).get();
}

//---------------

ApiBaseHandle::Ref createPipelineLayoutHandle(Device* device, const ShaderLayout& layout) {
	vk::Device vkDevice(device->getApiHandle());
	uint32_t maxSet = 0;
	for(auto& set : layout.getLayoutSets()) {
		maxSet = std::max(maxSet, set.first);
	}

	auto emptyLayout = device->getResourceCache()->createDescriptorSetLayout({});
	std::vector<vk::DescriptorSetLayout> layouts(maxSet+1, vk::DescriptorSetLayout(emptyLayout));
	for(auto& set : layout.getLayoutSets()) {
		layouts[set.first] = device->getResourceCache()->createDescriptorSetLayout(set.second);
	}

	std::vector<vk::PushConstantRange> pushConstantRanges;
	for(auto& range : layout.getPushConstantRanges()) {
		pushConstantRanges.emplace_back(getVkStageFlags(range.stages), range.offset, range.size);
	}

	return PipelineLayoutHandle::create(vkDevice.createPipelineLayout({{},
		static_cast<uint32_t>(layouts.size()), layouts.data(),
		static_cast<uint32_t>(pushConstantRanges.size()), pushConstantRanges.data(),
	}), vkDevice).get();
}

//---------------

ApiBaseHandle::Ref createComputePipelineHandle(Device* device, const PipelineState& state, VkPipeline parent) {

	// Create new pipeline
	vk::Device vkDevice(device->getApiHandle());
	vk::PipelineCache vkCache(device->getPipelineCache());
	auto shader = state.getShader();

	vk::ComputePipelineCreateInfo info{};
	info.layout = shader->getLayoutHandle();
	info.stage.stage = vk::ShaderStageFlagBits::eCompute;
	info.stage.pName = state.getEntryPoint().c_str();
	info.stage.pSpecializationInfo = nullptr; // TODO
	auto moduleEntry = shader->getShaderModules().find(ShaderStage::Compute);
	if(moduleEntry == shader->getShaderModules().end() || !moduleEntry->second)
		return nullptr;
	info.stage.module = moduleEntry->second;

	// Derived pipeline
	info.basePipelineHandle = parent;
		
	// Create pipeline
	return PipelineHandle::create(vkDevice.createComputePipeline(vkCache, info), vkDevice).get();
}

//---------------

ApiBaseHandle::Ref createGraphicsPipelineHandle(Device* device, const PipelineState& state, VkPipeline parent) {

	// Create new pipeline
	vk::Device vkDevice(device->getApiHandle());
	vk::PipelineCache vkCache(device->getPipelineCache());
	auto shader = state.getShader();

	vk::GraphicsPipelineCreateInfo info{};
	info.layout = shader->getLayoutHandle();
	
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
		bindings.emplace_back(b.second.binding, b.second.stride, static_cast<vk::VertexInputRate>(b.second.inputRate));
	for(auto& a : state.getVertexInputState().getAttributes())
		attributes.emplace_back(a.second.location, a.second.binding, getVkFormat(a.second.format), a.second.offset);
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
	for(uint32_t i=0; i<state.getFramebufferFormat().getColorAttachmentCount(); ++i) {
		attachments.emplace_back(convertColorBlendAttachmentState(bs));
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
	info.renderPass = device->getResourceCache()->createRenderPass(state.getFramebufferFormat());
	info.subpass = 0; // TODO: allow multiple subpasses

	// Derived pipeline
	info.basePipelineHandle = parent;

	// Create pipeline
	return PipelineHandle::create(vkDevice.createGraphicsPipeline(vkCache, info), vkDevice).get();
}

//---------------

ApiBaseHandle::Ref createPipelineHandle(Device* device, const PipelineState& state, VkPipeline parent) {
	switch(state.getType()) {
		case PipelineType::Compute: return createComputePipelineHandle(device, state, parent);
		case PipelineType::Graphics: return createGraphicsPipelineHandle(device, state, parent);
		default: return nullptr;
	}
}

//---------------

} /* Rendering */