/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_PIPELINE_STATE_H_
#define RENDERING_CORE_PIPELINE_STATE_H_

#include "../Core/Common.h"

#include <Util/ReferenceCounter.h>
#include <Util/Hashing.h>
#include <Util/Graphics/Color.h>

#include <Geometry/Rect.h>

#include <functional>
#include <vector>
#include <map>

namespace Rendering {
class Shader;
using ShaderRef = Util::Reference<Shader>;
class FBO;
using FBORef = Util::Reference<FBO>;

/** @addtogroup context
 * @{
 * @defgroup rendering_state State
 * @}
 */

//! @addtogroup rendering_state
//! @{

//==================================================================
// VertexInputState

struct VertexInputBinding {
	uint32_t binding = 0;
	uint32_t stride = 0;
	uint32_t inputRate = 0;
};

//-------------

struct VertexInputAttribute {
	uint32_t location = 0;
	uint32_t binding = 0;
	InternalFormat format = InternalFormat::RGBA32Float;
	size_t offset = 0;
};

//-------------

//! Configures the mapping between mesh vertex attributes to the corresponding shader resource.
class VertexInputState {
public:
	//! Sets the vertex binding description of the binding given by @p index.
	VertexInputState& setBinding(const VertexInputBinding& value) { bindings[value.binding] = value; dirty=true; return *this; }
	//! Simultaneously sets all vertex binding descriptions.
	VertexInputState& setBindings(const std::map<uint32_t, VertexInputBinding>& values) { bindings = values; dirty=true; return *this; }

	//! @see{setBinding()}
	const VertexInputBinding& getBinding(uint32_t binding=0) const { return bindings.at(binding); }
	//! @see{setBinding()}
	bool hasBinding(uint32_t binding=0) const { return bindings.find(binding) != bindings.end(); }
	//! @see{setBindings()}
	const std::map<uint32_t, VertexInputBinding>& getBindings() const { return bindings; }
	//! @see{setBindingCount()}
	uint32_t getBindingCount() const { return bindings.size(); }
	
	//! Sets the vertex attribute description of the attribute given by @p index.
	VertexInputState& setAttribute(const VertexInputAttribute& value) { attributes[value.location] = value; dirty=true; return *this; }
	//! Simultaneously sets all vertex attribute descriptions.
	VertexInputState& setAttributes(const std::map<uint32_t, VertexInputAttribute>& values) { attributes = values; dirty=true; return *this; }

	//! @see{setAttribute()}
	const VertexInputAttribute& getAttribute(uint32_t location=0) const { return attributes.at(location); }
	//! @see{setAttribute()}
	bool hasAttribute(uint32_t location=0) const { return attributes.find(location) != attributes.end(); }
	//! @see{setAttributes()}
	const std::map<uint32_t, VertexInputAttribute>& getAttributes() const { return attributes; }
	//! @see{setAttributeCount()}
	uint32_t getAttributeCount() const { return attributes.size(); }

private:
	std::map<uint32_t, VertexInputBinding> bindings;
	std::map<uint32_t, VertexInputAttribute> attributes;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// InputAssemblyState

enum class PrimitiveTopology {
	PointList, //! Specifies a series of separate point primitives.
	LineList, //! Specifies a series of separate line primitives.
	LineStrip, //! Specifies a series of connected line primitives with consecutive lines sharing a vertex.
	TriangleList, //! Specifies a series of separate triangle primitives.
	TriangleStrip, //! Specifies a series of connected triangle primitives with consecutive triangles sharing an edge.
	TriangleFan, //! Specifies a series of connected triangle primitives with all triangles sharing a common vertex.
	LineListWithAdjacency, //! Specifies a series of separate line primitives with adjacency.
	LineStripWithAdjacency, //! Specifies a series of connected line primitives with adjacency, with consecutive primitives sharing three vertices.
	TriangleListWithAdjacency, //! Specifies a series of separate triangle primitives with adjacency.
	TriangleStripWithAdjacency, //! Specifies connected triangle primitives with adjacency, with consecutive triangles sharing an edge.
	PatchList, //! Specifies separate patch primitives.
};

//-------------

//! Configures how vertices form the geometry to draw.
class InputAssemblyState {
public:
	//! Defines how consecutive vertices are organized into primitives.
	InputAssemblyState& setTopology(PrimitiveTopology value) { topology = value; dirty = true; return *this; }
	//! Controls whether a special vertex index value is treated as restarting the assembly of primitives (0xffffffff for 32bit indices or 0xffff for 16bit indices).
	InputAssemblyState& setPrimitiveRestartEnabled(bool value) { primitiveRestartEnabled = value; dirty = true; return *this; }

	//! @see{setTopology()}
	PrimitiveTopology getTopology() const { return topology; }
	//! @see{setPrimitiveRestartEnabled()}
	bool isPrimitiveRestartEnabled() const { return primitiveRestartEnabled; }

private:
	PrimitiveTopology topology = PrimitiveTopology::TriangleList;
	bool primitiveRestartEnabled = false;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// ViewportState

struct Viewport {
	Viewport() = default;
	Viewport(const Geometry::Rect_i& rect, float minDepth = 0.0, float maxDepth = 1.0) : rect(rect), minDepth(minDepth), maxDepth(maxDepth) {}
	Viewport(int32_t x, int32_t y, int32_t w, int32_t h, float minDepth = 0.0, float maxDepth = 1.0) : rect(x,y,w,h), minDepth(minDepth), maxDepth(maxDepth) {}
	Viewport(int32_t w, int32_t h) : rect(0,0,w,h), minDepth(0), maxDepth(1) {}
	Geometry::Rect_i rect = {0, 0, 1, 1};
	float minDepth = 0.0;
	float maxDepth = 1.0;
};

//-------------

//! Configures the static viewport & scissor state of the GPU.
class ViewportState {
public:
	ViewportState() = default;
	ViewportState(const Viewport& viewport) : viewports({viewport}) {}
	ViewportState(const Viewport& viewport, const Geometry::Rect_i& scissor) : viewports({viewport}), scissors({scissor}) {}

	//! Sets the viewport transform of the viewport given by @p index.
	ViewportState& setViewport(const Viewport& value, uint32_t index=0);
	//! Simultaneously sets all viewports.
	ViewportState& setViewports(const std::vector<Viewport>& values);
	//! Controls if the viewport state is dynamic. If the viewport state is dynamic, all viewports in this state are ignored.
	ViewportState& setDynamicViewports(bool value) { dynamicViewports = value; return *this; }

	//! Sets the rectangular bounds of the scissor for the corresponding viewport given by @p index.
	ViewportState& setScissor(const Geometry::Rect_i& value, uint32_t index=0);
	//! Simultaneously sets all scissors.
	ViewportState& setScissors(const std::vector<Geometry::Rect_i>& values);
	//! Controls if the scissor state is dynamic. If the scissor state is dynamic, all scissors in this state are ignored.
	ViewportState& setDynamicScissors(bool value) { dynamicScissors = value; dirty = true; return *this; }

	//! Sets the number of viewports and scissors used by the pipeline.
	ViewportState& setViewportScissorCount(uint32_t value) { viewports.resize(value); scissors.resize(value); dirty = true; return *this; }

	//! @see{setViewport()}
	const Viewport& getViewport(uint32_t index=0) const { return viewports[index]; }
	//! @see{setViewports()}
	const std::vector<Viewport>& getViewports() const { return viewports; }
	//! @see{setDynamicViewports()}
	bool hasDynamicViewports() const { return dynamicViewports; }

	//! @see{setScissor()}
	const Geometry::Rect_i& getScissor(uint32_t index=0) const { return scissors[index]; }
	//! @see{setScissors()}
	const std::vector<Geometry::Rect_i>& getScissors() const { return scissors; }
	//! @see{setDynamicScissors()}
	bool hasDynamicScissors() const { return dynamicScissors; }
	
	//! @see{setViewportScissorCount()}
	uint32_t getViewportScissorCount() const { return viewports.size(); }
private:
	std::vector<Viewport> viewports{{}};
	std::vector<Geometry::Rect_i> scissors{{0,0,1,1}};
	bool dynamicViewports = false;
	bool dynamicScissors = false;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// RasterizationState

enum class PolygonMode {
	Fill, //! Specifies that polygons are rendered using polygon rasterization.
	Line, //! Specifies that polygon edges are drawn as line segments.
	Point //! Specifies that polygon vertices are drawn as points.
};

//-------------

enum class CullMode {
	None, //! Specifies that no triangles are discarded.
	Front, //! Specifies that front-facing triangles are discarded.
	Back, //! Specifies that back-facing triangles are discarded.
	FrontAndBack //! Specifies that all triangles are discarded.
};

//-------------

enum class FrontFace {
	CounterClockwise, //! Specifies that a triangle with positive area is considered front-facing.
	Clockwise //! Specifies that a triangle with negative area is considered front-facing.
};

//-------------

//! Configures the rasterization operations in the GPU.
class RasterizationState {
public:
	//! Controls whether to clamp the fragment’s depth values. Enabling depth clamp will also disable clipping primitives to the z planes of the frustrum.
	RasterizationState& setDepthClampEnabled(bool value) { depthClampEnable = value; dirty = true; return *this; }
	//! Controls whether primitives are discarded immediately before the rasterization stage.
	RasterizationState& setRasterizerDiscardEnabled(bool value) { rasterizerDiscardEnable = value; dirty = true; return *this; }
	//! Sets the triangle rendering mode.
	RasterizationState& setPolygonMode(PolygonMode value) { polygonMode = value; dirty = true; return *this; }
	//! Sets the triangle facing direction used for primitive culling.
	RasterizationState& setCullMode(CullMode value) { cullMode = value; dirty = true; return *this; }
	//! Sets the front-facing triangle orientation to be used for culling.
	RasterizationState& setFrontFace(FrontFace value) { frontFace = value; dirty = true; return *this; }
	//! Controls whether to bias fragment depth values.
	RasterizationState& setDepthBiasEnabled(bool value) { depthBiasEnable = value; dirty = true; return *this; }
	//! Sets a scalar factor controlling the constant depth value added to each fragment.
	RasterizationState& setDepthBiasConstantFactor(float value) { depthBiasConstantFactor = value; dirty = true; return *this; }
	//! Sets the maximum (or minimum) depth bias of a fragment.
	RasterizationState& setDepthBiasClamp(float value) { depthBiasClamp = value; dirty = true; return *this; }
	//! Sets a scalar factor applied to a fragment’s slope in depth bias calculations.
	RasterizationState& setDepthBiasSlopeFactor(float value) { depthBiasSlopeFactor = value; dirty = true; return *this; }
	//! Controls if the depth bias is dynamic. If it is dynamic, the depth bias values in this state are ignored.
	RasterizationState& setDynamicDepthBias(bool value) { dynamicDepthBias = value; dirty = true; return *this; }
	//! Sets the width of rasterized line segments.
	RasterizationState& setLineWidth(float value) { lineWidth = value; dirty = true; return *this; }
	//! Controls if the line width is dynamic. If it is dynamic, the value in this state is ignored.
	RasterizationState& setDynamicLineWidth(bool value) { dynamicLineWidth = value; dirty = true; return *this; }

	//! @see{setDepthClampEnabled()}
	bool isDepthClampEnabled() const { return depthClampEnable; }
	//! @see{setRasterizerDiscardEnabled()}
	bool isRasterizerDiscardEnabled() const { return rasterizerDiscardEnable; }
	//! @see{setPolygonMode()}
	PolygonMode getPolygonMode() const { return polygonMode; }
	//! @see{setCullMode()}
	CullMode getCullMode() const { return cullMode; }
	//! @see{setFrontFace()}
	FrontFace getFrontFace() const { return frontFace; }
	//! @see{setDepthBiasEnabled()}
	bool isDepthBiasEnabled() const { return depthBiasEnable; }
	//! @see{setDepthBiasConstantFactor()}
	float getDepthBiasConstantFactor() const { return depthBiasConstantFactor; }
	//! @see{setDepthBiasClamp()}
	float getDepthBiasClamp() const { return depthBiasClamp; }
	//! @see{setDepthBiasSlopeFactor()}
	float getDepthBiasSlopeFactor() const { return depthBiasSlopeFactor; }
	//! @see{setDynamicDepthBias()}
	float hasDynamicDepthBias() const { return dynamicDepthBias; }
	//! @see{setLineWidth()}
	float getLineWidth() const { return lineWidth; }
	//! @see{setDynamicLineWidth()}
	float hasDynamicLineWidth() const { return dynamicLineWidth; }

private:
	bool depthClampEnable = false;
	bool rasterizerDiscardEnable = false;
	PolygonMode polygonMode = PolygonMode::Fill;
	CullMode cullMode = CullMode::Back;
	FrontFace frontFace = FrontFace::CounterClockwise;
	bool depthBiasEnable = false;
	float depthBiasConstantFactor = 0;
	float depthBiasClamp = 0;
	float depthBiasSlopeFactor = 0;
	float lineWidth = 1.0;
	bool dynamicLineWidth = false;
	bool dynamicDepthBias = false;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// MultisampleState

//! Configures multisampling on the GPU.
class MultisampleState {
public:
	//! Sets the number of samples used in rasterization.
	MultisampleState& setSampleCount(uint32_t value) { sampleCount = value; dirty = true; return *this; };
	//! Enables or disables Sample Shading.
	MultisampleState& setSampleShadingEnabled(bool value) { sampleShadingEnable = value; dirty = true; return *this; };
	//! Specifies a minimum fraction of sample shading
	MultisampleState& setMinSampleShading(float value) { minSampleShading = value; dirty = true; return *this; };
	//! A bitmask of static coverage information that is ANDed with the coverage information generated during rasterization (currently supports max. 32 samples).
	MultisampleState& setSampleMask(uint32_t value) { sampleMask = value; dirty = true; return *this; };
	//! Controls whether a temporary coverage value is generated based on the alpha component of the fragment’s first color output.
	MultisampleState& setAlphaToCoverageEnabled(bool value) { alphaToCoverageEnable = value; dirty = true; return *this; };
	//! Controls whether the alpha component of the fragment’s first color output is replaced with one.
	MultisampleState& setAlphaToOneEnabled(bool value) { alphaToOneEnable = value; dirty = true; return *this; };

	//! @see{setSampleCount()}
	uint32_t getSampleCount() const { return sampleCount; };
	//! @see{setSampleShadingEnabled()}
	bool isSampleShadingEnabled() const { return sampleShadingEnable; };
	//! @see{setMinSampleShading()}
	float getMinSampleShading() const { return minSampleShading; };
	//! @see{setSampleMask()}
	uint32_t getSampleMask() const { return sampleMask; };
	//! @see{setAlphaToCoverageEnabled()}
	bool isAlphaToCoverageEnabled() const { return alphaToCoverageEnable; };
	//! @see{setAlphaToOneEnabled()}
	bool isAlphaToOneEnabled() const { return alphaToOneEnable; };
private:
	uint32_t sampleCount = 1;
	bool sampleShadingEnable = false;
	float minSampleShading = 1.0;
	uint32_t sampleMask = 0xffffffffu;
	bool alphaToCoverageEnable = false;
	bool alphaToOneEnable = false;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// DepthStencilState

enum class StencilOp {
	Keep, //! keeps the current value.
	Zero, //! sets the value to 0.
	Replace, //! sets the value to reference.
	IncrementAndClamp, //! increments the current value and clamps to the maximum representable unsigned value.
	DecrementAndClamp, //! decrements the current value and clamps to 0.
	Invert, //! bitwise-inverts the current value.
	IncrementAndWrap, //! increments the current value and wraps to 0 when the maximum value would have been exceeded.
	DecrementAndWrap, //! decrements the current value and wraps to the maximum possible value when the value would go below 0.
};

//-------------

struct StencilOpState {
	StencilOp failOp = StencilOp::Keep; //! Value specifying the action performed on samples that fail the stencil test.
	StencilOp passOp = StencilOp::Keep; //! Value specifying the action performed on samples that pass both the depth and stencil tests.
	StencilOp depthFailOp = StencilOp::Keep; //! Value specifying the action performed on samples that pass the stencil test and fail the depth test.
	ComparisonFunc compareOp = ComparisonFunc::Never; //! Value specifying the comparison operator used in the stencil test.
	uint32_t compareMask = 0; //! Selects the bits of the unsigned integer stencil values participating in the stencil test.
	uint32_t writeMask = 0; //! Selects the bits of the unsigned integer stencil values updated by the stencil test in the stencil framebuffer attachment.
	uint32_t reference = 0; //! An integer reference value that is used in the unsigned stencil comparison.
};

//-------------

//! Pipeline state controlling the depth bounds tests, stencil test, and depth test.
class DepthStencilState {
public:
//! Controls whether depth testing is enabled.
DepthStencilState& setDepthTestEnabled(bool value) { depthTestEnable = value; dirty = true; return *this; }
//! Controls whether depth writes are enabled when depth testing is enabled.
DepthStencilState& setDepthWriteEnabled(bool value) { depthWriteEnable = value; dirty = true; return *this; }
//! Sets the comparison operator used in the depth test.
DepthStencilState& setDepthCompareOp(ComparisonFunc value) { depthCompareOp = value; dirty = true; return *this; }
//! Controls whether depth bounds testing is enabled.
DepthStencilState& setDepthBoundsTestEnabled(bool value) { depthBoundsTestEnable = value; dirty = true; return *this; }
//! Controls whether stencil testing is enabled.
DepthStencilState& setStencilTestEnabled(bool value) { stencilTestEnable = value; dirty = true; return *this; }
//! Controls the front parameters of the stencil test.
DepthStencilState& setFront(StencilOpState value) { front = value; dirty = true; return *this; }
//! Controls the back parameters of the stencil test.
DepthStencilState& setBack(StencilOpState value) { back = value; dirty = true; return *this; }
//! Defines the min. value used in the depth bounds test.
DepthStencilState& setMinDepthBounds(float value) { minDepthBounds = value; dirty = true; return *this; }
//! Defines the max. value used in the depth bounds test.
DepthStencilState& setMaxDepthBounds(float value) { maxDepthBounds = value; dirty = true; return *this; }
//! Controls if the depth bounds are dynamic. If they are dynamic, the corresponding values in this state are ignored.
DepthStencilState& setDynamicDepthBounds(bool value) { dynamicDepthBounds = value; dirty = true; return *this; }
//! Controls if the stencil compare mask for both front and back is dynamic. If it is dynamic, the corresponding values in this state are ignored.
DepthStencilState& setDynamicCompareMask(bool value) { dynamicCompareMask = value; dirty = true; return *this; }
//! Controls if the stencil write mask for both front and back is dynamic. If it is dynamic, the corresponding values in this state are ignored.
DepthStencilState& setDynamicWriteMask(bool value) { dynamicWriteMask = value; dirty = true; return *this; }
//! Controls if the stencil reference for both front and back is dynamic. If it is dynamic, the corresponding values in this state are ignored.
DepthStencilState& setDynamicReference(bool value) { dynamicReference = value; dirty = true; return *this; }

//! @see{setDepthTestEnabled()}
bool isDepthTestEnabled() const { return depthTestEnable; }
//! @see{setDepthWriteEnabled()}
bool isDepthWriteEnabled() const { return depthWriteEnable; }
//! @see{setDepthCompareOp()}
ComparisonFunc getDepthCompareOp() const { return depthCompareOp; }
//! @see{setDepthBoundsTestEnabled()}
bool isDepthBoundsTestEnabled() const { return depthBoundsTestEnable; }
//! @see{setStencilTestEnabled()}
bool isStencilTestEnabled() const { return stencilTestEnable; }
//! @see{setFront()}
StencilOpState getFront() const { return front; }
//! @see{setBack()}
StencilOpState getBack() const { return back; }
//! @see{setMinDepthBounds()}
float getMinDepthBounds() const { return minDepthBounds; }
//! @see{setMaxDepthBounds()}
float getMaxDepthBounds() const { return maxDepthBounds; }
//! @see{setDynamicDepthBounds()}
bool hasDynamicDepthBounds() const { return dynamicDepthBounds; }
//! @see{setDynamicCompareMask()}
bool hasDynamicCompareMask() const { return dynamicCompareMask; }
//! @see{setDynamicWriteMask()}
bool hasDynamicWriteMask() const { return dynamicWriteMask; }
//! @see{setDynamicReference()}
bool hasDynamicReference() const { return dynamicReference; }

private:
	bool depthTestEnable = false;
	bool depthWriteEnable = true;
	ComparisonFunc depthCompareOp = ComparisonFunc::Less;
	bool depthBoundsTestEnable = false;
	bool stencilTestEnable = false;
	StencilOpState front;
	StencilOpState back;
	float minDepthBounds = 0;
	float maxDepthBounds = 1;
	bool dynamicDepthBounds = false;
	bool dynamicCompareMask = false;
	bool dynamicWriteMask = false;
	bool dynamicReference = false;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// ColorBlendState

enum class LogicOp {
	Clear, //! 0
	And, //! s ∧ d
	AndReverse, //! s ∧ ¬ d
	Copy, //! s
	AndInverted, //! ¬ s ∧ d
	NoOp, //! d
	Xor, //! s ⊕ d
	Or, //! s ∨ d
	Nor, //! ¬ (s ∨ d)
	Equivalent, //! ¬ (s ⊕ d)
	Invert, //! ¬ d
	OrReverse, //! s ∨ ¬ d
	CopyInverted, //! ¬ s
	OrInverted, //! ¬ s ∨ d
	Nand, //! ¬ (s ∧ d)
	Set, //! all 1s
};

//-------------

enum class BlendFactor {
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
	ConstantColor,
	OneMinusConstantColor,
	ConstantAlpha,
	OneMinusConstantAlpha,
	SrcAlphaSaturate,
	Src1Color,
	OneMinusSrc1Color,
	Src1Alpha,
	OneMinusSrc1Alpha
};

//-------------

enum class BlendOp {
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max,
};

//-------------

struct ColorBlendAttachmentState {
	bool blendEnable = false; //! Controls whether blending is enabled for the corresponding color attachment.
	BlendFactor srcColorBlendFactor = BlendFactor::Zero; //! Selects which blend factor is used to determine the source factors (Sr,Sg,Sb).
	BlendFactor dstColorBlendFactor = BlendFactor::One; //! Selects which blend factor is used to determine the destination factors (Dr,Dg,Db).
	BlendOp colorBlendOp = BlendOp::Add; //! Selects which blend operation is used to calculate the RGB values to write to the color attachment.
	BlendFactor srcAlphaBlendFactor = BlendFactor::Zero; //! Selects which blend factor is used to determine the source factor Sa.
	BlendFactor dstAlphaBlendFactor = BlendFactor::One; //! Selects which blend factor is used to determine the destination factor Da.
	BlendOp alphaBlendOp = BlendOp::Add; //! Selects which blend operation is use to calculate the alpha values to write to the color attachment.
	uint8_t colorWriteMask = 0x0fu; //! A bitmask specifying which of the RGBA components are enabled for writing.
};

//-------------

//! Blending combines the incoming source (s) fragment's color with the destination (d) color of each sample stored in the framebuffer.
class ColorBlendState {
public:
	//! Controls whether to apply Logical Operations.
	ColorBlendState& setLogicOpEnabled(bool value) { logicOpEnable = value; dirty = true; return *this; }
	//! Selects which logical operation to apply.
	ColorBlendState& setLogicOp(LogicOp value) { logicOp = value; dirty = true; return *this; }
	//! Sets the attachment state for the target given by @p index.
	ColorBlendState& setAttachment(const ColorBlendAttachmentState& value, uint32_t index=0);
	//! Simultaneously sets all attachments.
	ColorBlendState& setAttachments(const std::vector<ColorBlendAttachmentState>& values) { attachments = values; dirty = true; return *this; }
	//! Sets the number of attachments. This value must equal the color attachment count for the subpass in which the pipeline is used.
	ColorBlendState& setAttachmentCount(uint32_t value) { attachments.resize(value); dirty = true; return *this; }
	//! Sets the blend constant that is used in blending, depending on the blend factor.
	ColorBlendState& setConstantColor(const Util::Color4f& value) { constantColor = value; dirty = true; return *this; }
	//! Controls if the blend constant is dynamic. If it is dynamic, the value in this state is ignored.
	ColorBlendState& setDynamicConstantColor(bool value) { dynamicBlendConstant = value; dirty = true; return *this; }

	//! @see{setLogicOpEnabled()}
	bool isLogicOpEnabled() const { return logicOpEnable; }
	//! @see{setLogicOp()}
	LogicOp getLogicOp() const { return logicOp; }
	//! @see{setAttachment()}
	const ColorBlendAttachmentState& getAttachment(uint32_t index=0) const { return attachments[index]; }
	//! @see{setAttachments()}
	const std::vector<ColorBlendAttachmentState>& getAttachments() const { return attachments; }
	//! @see{setAttachmentCount()}
	uint32_t getAttachmentCount() const { return attachments.size(); }
	//! @see{setConstantColor()}
	Util::Color4f getConstantColor() const { return constantColor; }
	//! @see{setDynamicConstantColor()}
	bool hasDynamicConstantColor() const { return dynamicBlendConstant; }
private:
	bool logicOpEnable = false;
	LogicOp logicOp = LogicOp::Copy;
	std::vector<ColorBlendAttachmentState> attachments = {{}};
	Util::Color4f constantColor = {0,0,0,0};
	bool dynamicBlendConstant = false;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// FramebufferFormat

struct AttachmentFormat {
	AttachmentFormat(InternalFormat format = InternalFormat::Unknown, uint32_t samples = 1) : format(format), samples(samples) {}
	InternalFormat format;
	uint32_t samples;
};

class FramebufferFormat {
public:
	FramebufferFormat() {}
	FramebufferFormat(const FBORef& fbo);

	//! Sets the color attachment format for the framebuffer attachment given by @p index.
	FramebufferFormat& setColorAttachment(const AttachmentFormat& value, uint32_t index=0) { colorAttachments[index] = value; dirty = true; return *this; }
	//! Simultaneously sets all color attachments.
	FramebufferFormat& setColorAttachments(const std::vector<AttachmentFormat>& values) { colorAttachments = values; dirty = true; return *this; }
	//! Sets the number of color attachments of the framebuffer.
	FramebufferFormat& setColorAttachmentCount(uint32_t value) { colorAttachments.resize(value); dirty = true; return *this; }
	//! Sets the depth/stencil attachment of the framebuffer.
	FramebufferFormat& setDepthStencilAttachment(const AttachmentFormat& value, uint32_t index=0) { colorAttachments[index] = value; dirty = true; return *this; }

	//! @see{setColorAttachment()}
	const AttachmentFormat& getColorAttachment(uint32_t index=0) const { return colorAttachments[index]; }
	//! @see{setColorAttachments()}
	const std::vector<AttachmentFormat>& getColorAttachments() const { return colorAttachments; }
	//! @see{setColorAttachmentCount()}
	uint32_t getColorAttachmentCount() const { return colorAttachments.size(); }	
	//! @see{setDepthStencilAttachment()}
	const AttachmentFormat& getDepthStencilAttachment() const { return depthAttachment; }
	//! @see{setDepthStencilAttachment()}
	bool hasDepthStencilAttachment() const { return depthAttachment.samples > 0 && isDepthStencilFormat(depthAttachment.format); }
	
private:
	std::vector<AttachmentFormat> colorAttachments = {};
	AttachmentFormat depthAttachment = {InternalFormat::Unknown, 0};
public:
	void markAsDirty() { dirty = true; }
	void markAsChanged() { dirty = true; hash = 0; }
	void markAsUnchanged() { hash = Util::hash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != Util::hash(*this) : false; }
private:
	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// PipelineState

enum class PipelineType {
	Graphics = 0,
	Compute,
};

//-------------

class PipelineState : public Util::ReferenceCounter<PipelineState> {
public:
	PipelineState();
	~PipelineState() = default;
	PipelineState(PipelineState&& o);
	PipelineState(const PipelineState& o);
	PipelineState& operator=(PipelineState&& o);
	PipelineState& operator=(const PipelineState& o);

	PipelineState& reset();
	
	inline PipelineState& setType(PipelineType value) { type = value; dirty = true; return *this; }
	inline PipelineState& setVertexInputState(const VertexInputState& state) { vertexInput = state; vertexInput.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setInputAssemblyState(const InputAssemblyState& state) { inputAssembly = state; inputAssembly.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setViewportState(const ViewportState& state) { viewport = state; viewport.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setRasterizationState(const RasterizationState& state) { rasterization = state; rasterization.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setMultisampleState(const MultisampleState& state) { multisample = state; multisample.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setDepthStencilState(const DepthStencilState& state) { depthStencil = state; depthStencil.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setColorBlendState(const ColorBlendState& state) { colorBlend = state; colorBlend.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setFramebufferFormat(const FramebufferFormat& state) { attachments = state; attachments.markAsDirty(); dirty = true; return *this; }
	inline PipelineState& setFramebufferFormat(const FBORef& fbo) { attachments = fbo; dirty = true; return *this; }
	inline PipelineState& setEntryPoint(const std::string& value) { entrypoint = value; dirty = true; return *this; }
	PipelineState& setShader(const ShaderRef& value);

	PipelineType getType() const { return type; }
	
	const VertexInputState& getVertexInputState() const { return vertexInput; }
	VertexInputState& getVertexInputState() { dirty = true; return vertexInput; }

	const InputAssemblyState& getInputAssemblyState() const { return inputAssembly; }
	InputAssemblyState& getInputAssemblyState() { dirty = true; return inputAssembly; }

	const ViewportState& getViewportState() const { return viewport; }
	ViewportState& getViewportState() { dirty = true; return viewport; }

	const RasterizationState& getRasterizationState() const { return rasterization; }
	RasterizationState& getRasterizationState() { dirty = true; return rasterization; }

	const MultisampleState& getMultisampleState() const { return multisample; }
	MultisampleState& getMultisampleState() { dirty = true; return multisample; }

	const DepthStencilState& getDepthStencilState() const { return depthStencil; }
	DepthStencilState& getDepthStencilState() { dirty = true; return depthStencil; }

	const ColorBlendState& getColorBlendState() const { return colorBlend; }
	ColorBlendState& getColorBlendState() { dirty = true; return colorBlend; }

	const FramebufferFormat& getFramebufferFormat() const { return attachments; }
	FramebufferFormat& getFramebufferFormat() { dirty = true; return attachments; }

	const std::string& getEntryPoint() const { return entrypoint; }
	std::string& getEntryPoint() { dirty = true; return entrypoint; }

	const ShaderRef& getShader() const { return shader; }
	ShaderRef& getShader() { dirty = true; return shader; }

private:
	PipelineType type = PipelineType::Graphics;
	VertexInputState vertexInput;
	InputAssemblyState inputAssembly;
	ViewportState viewport;
	RasterizationState rasterization;
	MultisampleState multisample;
	DepthStencilState depthStencil;
	ColorBlendState colorBlend;
	FramebufferFormat attachments;
	ShaderRef shader;
	std::string entrypoint;

public:
	void markAsDirty() { dirty = true; }
	void markAsChanged();
	void markAsUnchanged();
	bool hasChanged() const;
private:
	bool dirty = true;
};

//! @}

} /* Rendering */

//==================================================================
// state hashing
//==================================================================

namespace std {

//-------------

template <> struct hash<Rendering::VertexInputBinding> {
	std::size_t operator()(const Rendering::VertexInputBinding& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.binding);
		Util::hash_combine(result, state.stride);
		Util::hash_combine(result, state.inputRate);
		return result;
	}
};


//-------------

template <> struct hash<Rendering::VertexInputAttribute> {
	std::size_t operator()(const Rendering::VertexInputAttribute& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.location);
		Util::hash_combine(result, state.binding);
		Util::hash_combine(result, state.format);
		Util::hash_combine(result, state.offset);
		return result;
	}
};

//-------------

template <> struct hash<Rendering::VertexInputState> {
	std::size_t operator()(const Rendering::VertexInputState& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.getBindingCount());
		for(const auto& b : state.getBindings())
			Util::hash_combine(result, b.second);
		Util::hash_combine(result, state.getAttributeCount());
		for(const auto& a : state.getAttributes())
			Util::hash_combine(result, a.second);
		return result;
	}
};

//-------------

template <> struct hash<Rendering::InputAssemblyState> {
	std::size_t operator()(const Rendering::InputAssemblyState& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.getTopology());
		Util::hash_combine(result, state.isPrimitiveRestartEnabled());
		return result;
	}
};

//-------------

template <> struct hash<Geometry::Rect_i> {
	std::size_t operator()(const Geometry::Rect_i &rect) const {
		std::size_t result = 0;
		Util::hash_combine(result, rect.getX());
		Util::hash_combine(result, rect.getY());
		Util::hash_combine(result, rect.getWidth());
		Util::hash_combine(result, rect.getHeight());
		return result;
	}
};

//-------------

template <> struct hash<Rendering::Viewport> {
	std::size_t operator()(const Rendering::Viewport &vp) const {
		std::size_t result = 0;
		Util::hash_combine(result, vp.rect);
		Util::hash_combine(result, vp.minDepth);
		Util::hash_combine(result, vp.maxDepth);
		return result;
	}
};

//-------------

template <> struct hash<Rendering::ViewportState> {
	std::size_t operator()(const Rendering::ViewportState& state) const {
		std::size_t result = 0;
		if(!state.hasDynamicViewports()) {
			Util::hash_combine(result, state.getViewportScissorCount());
			for(const auto& vp : state.getViewports())
				Util::hash_combine(result, vp);
		}
		if(!state.hasDynamicScissors()) {
			Util::hash_combine(result, state.getViewportScissorCount());
			for(const auto& r : state.getScissors())
				Util::hash_combine(result, r);
		}
		return result;
	}
};

//-------------

template <> struct hash<Rendering::RasterizationState> {
	std::size_t operator()(const Rendering::RasterizationState& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.isDepthClampEnabled());
		Util::hash_combine(result, state.isRasterizerDiscardEnabled());
		Util::hash_combine(result, state.getPolygonMode());
		Util::hash_combine(result, state.getCullMode());
		Util::hash_combine(result, state.getFrontFace());
		if(state.isDepthBiasEnabled() && !state.hasDynamicDepthBias()) {
			Util::hash_combine(result, state.getDepthBiasConstantFactor());
			Util::hash_combine(result, state.getDepthBiasClamp());
			Util::hash_combine(result, state.getDepthBiasSlopeFactor());
		}
		if(!state.hasDynamicLineWidth())
			Util::hash_combine(result, state.getLineWidth());
		return result;
	}
};

//-------------

template <> struct hash<Rendering::MultisampleState> {
	std::size_t operator()(const Rendering::MultisampleState& state) const {
		std::size_t result = 0;
		if(state.isSampleShadingEnabled()) {
			Util::hash_combine(result, state.getSampleCount());
			Util::hash_combine(result, state.getMinSampleShading());
			Util::hash_combine(result, state.getSampleMask());
			Util::hash_combine(result, state.isAlphaToCoverageEnabled());
			Util::hash_combine(result, state.isAlphaToOneEnabled());
		}
		return result;
	}
};

//-------------

template <> struct hash<Rendering::DepthStencilState> {
	std::size_t operator()(const Rendering::DepthStencilState& state) const {
		std::size_t result = 0;
		if(state.isDepthTestEnabled()) {
			Util::hash_combine(result, state.getDepthCompareOp());
			if(state.isDepthBoundsTestEnabled() && !state.hasDynamicDepthBounds()) {
				Util::hash_combine(result, state.getMinDepthBounds());
				Util::hash_combine(result, state.getMaxDepthBounds());
			}
			if(state.isStencilTestEnabled()) {
				Util::hash_combine(result, state.getFront().failOp);
				Util::hash_combine(result, state.getFront().passOp);
				Util::hash_combine(result, state.getFront().depthFailOp);
				Util::hash_combine(result, state.getFront().compareOp);
				Util::hash_combine(result, state.getBack().failOp);
				Util::hash_combine(result, state.getBack().passOp);
				Util::hash_combine(result, state.getBack().depthFailOp);
				Util::hash_combine(result, state.getBack().compareOp);
				if(!state.hasDynamicCompareMask()) {
					Util::hash_combine(result, state.getFront().compareMask);
					Util::hash_combine(result, state.getBack().compareMask);
				}
				if(!state.hasDynamicWriteMask()) {
					Util::hash_combine(result, state.getFront().writeMask);
					Util::hash_combine(result, state.getBack().writeMask);
				}
				if(!state.hasDynamicReference()) {
					Util::hash_combine(result, state.getFront().reference);
					Util::hash_combine(result, state.getBack().reference);
				}
			}
		}
		return result;
	}
};

//-------------

template <> struct hash<Rendering::ColorBlendAttachmentState> {
	std::size_t operator()(const Rendering::ColorBlendAttachmentState& state) const {
		std::size_t result = 0;
		if(state.blendEnable) {
			Util::hash_combine(result, state.srcColorBlendFactor);
			Util::hash_combine(result, state.dstColorBlendFactor);
			Util::hash_combine(result, state.colorBlendOp);
			Util::hash_combine(result, state.srcAlphaBlendFactor);
			Util::hash_combine(result, state.dstAlphaBlendFactor);
			Util::hash_combine(result, state.alphaBlendOp);
			Util::hash_combine(result, state.colorWriteMask);
		}
		return result;
	}
};

//-------------

template <> struct hash<Rendering::ColorBlendState> {
	std::size_t operator()(const Rendering::ColorBlendState& state) const {
		std::size_t result = 0;
		if(state.isLogicOpEnabled()) {
			Util::hash_combine(result, state.getLogicOp());
		} else {
			if(!state.hasDynamicConstantColor()) {
				Util::hash_combine(result, state.getConstantColor().getR());
				Util::hash_combine(result, state.getConstantColor().getG());
				Util::hash_combine(result, state.getConstantColor().getB());
				Util::hash_combine(result, state.getConstantColor().getA());
			}
			Util::hash_combine(result, state.getAttachmentCount());
			for(auto& a : state.getAttachments())
				Util::hash_combine(result, a);
		}
		return result;
	}
};

//-------------

template <> struct hash<Rendering::FramebufferFormat> {
	std::size_t operator()(const Rendering::FramebufferFormat& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.getColorAttachmentCount());
		for(auto& a : state.getColorAttachments()) {
			Util::hash_combine(result, a.format);
			Util::hash_combine(result, a.samples);
		}
		if(state.hasDepthStencilAttachment()) {
			Util::hash_combine(result, state.getDepthStencilAttachment().format);
			Util::hash_combine(result, state.getDepthStencilAttachment().samples);
		}
		return result;
	}
};

//-------------

template <> struct hash<Rendering::PipelineState> {
	std::size_t operator()(const Rendering::PipelineState& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.getType());	
		Util::hash_combine(result, state.getShader().get());
		Util::hash_combine(result, state.getEntryPoint());
		if(state.getType() == Rendering::PipelineType::Graphics) {
			Util::hash_combine(result, state.getColorBlendState());
			Util::hash_combine(result, state.getDepthStencilState());
			Util::hash_combine(result, state.getFramebufferFormat());
			Util::hash_combine(result, state.getInputAssemblyState());
			Util::hash_combine(result, state.getMultisampleState());
			Util::hash_combine(result, state.getRasterizationState());
			Util::hash_combine(result, state.getVertexInputState());
			Util::hash_combine(result, state.getViewportState());
		}
		return result;
	}
};

//-------------

}

#endif /* end of include guard: RENDERING_CORE_PIPELINE_STATE_H_ */