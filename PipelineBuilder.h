/*
	This file is part of the Platform for Algorithm Development and Rendering (PADrend).
	Web page: http://www.padrend.de/
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2022 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_PIPELINEBUILDER_H_
#define RENDERING_PIPELINEBUILDER_H_

#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>

#include <nvrhi/nvrhi.h>

#include <memory>

namespace Rendering {
class RenderDevice;
using RenderDeviceHandle = Util::Reference<RenderDevice>;

class PipelineBuilder : public Util::ReferenceCounter<PipelineBuilder> {
	PROVIDES_TYPE_NAME_NV(PipelineBuilder)
public:
	PipelineBuilder(const RenderDeviceHandle& device);
	~PipelineBuilder();
	PipelineBuilder(const PipelineBuilder&) = delete;
	PipelineBuilder(PipelineBuilder&& o);
	PipelineBuilder& operator=(const PipelineBuilder&) = delete;
	PipelineBuilder& operator=(PipelineBuilder&& o);
	
	//! @name Pipelines
	// @{

	//! Create a graphics pipeline from the active state.
	nvrhi::GraphicsPipelineHandle createGraphicsPipeline() const;

	//! Create a compute pipeline from the active state.
	nvrhi::ComputePipelineHandle createComputePipeline() const;

	//! Create a meshlet pipeline from the active state.
	nvrhi::MeshletPipelineHandle createMeshletPipeline() const;

	// @}
	
	//! @name Shader
	// @{

	//! Set the active shader of the specified type.
	void setShader(const nvrhi::ShaderHandle& shader, nvrhi::ShaderType type);
	
	//! Get the active shader of the specified type.
	nvrhi::ShaderHandle getShader(nvrhi::ShaderType type) const;

	//! pushes the currently active shader to the shader stack.
	void pushShader(nvrhi::ShaderType type);
	
	//! removes and activates the last shader on the shader stack.
	void popShader(nvrhi::ShaderType type);

	//! Push and set the active shader of the specified type.
	void pushAndSetShader(const nvrhi::ShaderHandle& shader, nvrhi::ShaderType type) {
		pushShader(type);
		setShader(shader, type);
	}

	//! Set the compute shader
	void setComputeShader(const nvrhi::ShaderHandle& cs) {
		setShader(cs, nvrhi::ShaderType::Compute);
	}

	//! Push and set the compute shader
	void pushAndSetComputeShader(const nvrhi::ShaderHandle& cs) {
		pushAndSetShader(cs, nvrhi::ShaderType::Compute);
	}

	//! Set the graphics shaders
	void setGraphicsShaders(const nvrhi::ShaderHandle& vs, const nvrhi::ShaderHandle& ps, const nvrhi::ShaderHandle& gs = nullptr, const nvrhi::ShaderHandle& hs = nullptr, const nvrhi::ShaderHandle& ds = nullptr) {
		setShader(vs, nvrhi::ShaderType::Vertex);
		setShader(ps, nvrhi::ShaderType::Pixel);
		setShader(gs, nvrhi::ShaderType::Geometry);
		setShader(hs, nvrhi::ShaderType::Hull);
		setShader(ds, nvrhi::ShaderType::Domain);
	}

	//! Push and set the graphics shaders.
	void pushAndSetGraphicsShaders(const nvrhi::ShaderHandle& vs, const nvrhi::ShaderHandle& ps, const nvrhi::ShaderHandle& gs = nullptr, const nvrhi::ShaderHandle& hs = nullptr, const nvrhi::ShaderHandle& ds = nullptr) {
		pushAndSetShader(vs, nvrhi::ShaderType::Vertex);
		pushAndSetShader(ps, nvrhi::ShaderType::Pixel);
		pushAndSetShader(gs, nvrhi::ShaderType::Geometry);
		pushAndSetShader(hs, nvrhi::ShaderType::Hull);
		pushAndSetShader(ds, nvrhi::ShaderType::Domain);
	}

	//! Set the mesh shaders
	void setMeshShaders(const nvrhi::ShaderHandle& ms, const nvrhi::ShaderHandle& ps, const nvrhi::ShaderHandle& as = nullptr) {
		setShader(ms, nvrhi::ShaderType::Mesh);
		setShader(ps, nvrhi::ShaderType::Pixel);
		setShader(as, nvrhi::ShaderType::Amplification);
	}

	//! Push and set the mesh shaders
	void pushAndSetMeshShaders(const nvrhi::ShaderHandle& ms, const nvrhi::ShaderHandle& ps, const nvrhi::ShaderHandle& as = nullptr) {
		pushAndSetShader(ms, nvrhi::ShaderType::Mesh);
		pushAndSetShader(ps, nvrhi::ShaderType::Pixel);
		pushAndSetShader(as, nvrhi::ShaderType::Amplification);
	}

	// @}

	//! @name Binding layout
	// @{

	//! Set the active binding layout.
	void setBindingLayout(const nvrhi::BindingLayoutHandle& layout, uint32_t index=0);
	
	//! Get the active binding layout.
	nvrhi::BindingLayoutHandle getBindingLayout(uint32_t index=0) const;

	//! pushes the currently active binding layout to the binding layout stack.
	void pushBindingLayout(uint32_t index=0);
	
	//! removes and activates the last binding layout on the binding layout stack.
	void popBindingLayout(uint32_t index=0);

	//! Push and set the active binding layout.
	void pushAndSetBindingLayout(const nvrhi::BindingLayoutHandle& layout, uint32_t index=0) {
		pushBindingLayout(index);
		setBindingLayout(layout, index);
	}

	// @}

	//! @name Framebuffer
	// @{

	//! Set the active framebuffer.
	void setFramebuffer(const nvrhi::FramebufferHandle& framebuffer);
	
	//! Get the active framebuffer.
	nvrhi::FramebufferHandle getFramebuffer() const;

	//! pushes the currently active framebuffer to the framebuffer stack.
	void pushFramebuffer();
	
	//! removes and activates the last framebuffer on the framebuffer stack.
	void popFramebuffer();

	//! Push and set the active framebuffer.
	void pushAndSetFramebuffer(const nvrhi::FramebufferHandle& framebuffer) {
		pushFramebuffer();
		setFramebuffer(framebuffer);
	}

	// @}

	//! @name Input layout
	// @{

	//! Set the active input layout.
	void setInputLayout(const nvrhi::InputLayoutHandle& layout);
	
	//! Get the active input layout.
	nvrhi::InputLayoutHandle getInputLayout() const;

	//! pushes the currently active input layout to the input layout stack.
	void pushInputLayout();
	
	//! removes and activates the last input layout on the input layout stack.
	void popInputLayout();

	//! Push and set the active input layout.
	void pushAndSetInputLayout(const nvrhi::InputLayoutHandle& layout) {
		pushInputLayout();
		setInputLayout(layout);
	}

	// @}

	//! @name Primitive type
	// @{

	//! Set the active primitive type.
	void setPrimitiveType(nvrhi::PrimitiveType type);
	
	//! Get the active primitive type.
	nvrhi::PrimitiveType getPrimitiveType() const;

	//! pushes the currently active primitive type to the primitive type stack.
	void pushPrimitiveType();
	
	//! removes and activates the last primitive type on the primitive type stack.
	void popPrimitiveType();

	//! Push and set the active primitive type.
	void pushAndSetPrimitiveType(nvrhi::PrimitiveType type) {
		pushPrimitiveType();
		setPrimitiveType(type);
	}

	// @}

	//! @name patch control points
	// @{

	//! Set the active patch control points.
	void setPatchControlPoints(uint32_t points);
	
	//! Get the active patch control points.
	uint32_t getPatchControlPoints() const;

	//! pushes the currently active patch control points to the patch control points stack.
	void pushPatchControlPoints();
	
	//! removes and activates the last patch control points on the patch control points stack.
	void popPatchControlPoints();

	//! Push and set the active patch control points.
	void pushAndSetPatchControlPoints(uint32_t points) {
		pushPatchControlPoints();
		setPatchControlPoints(points);
	}

	// @}

	//! @name Render state
	// @{

	//! Set the active render state.
	void setRenderState(const nvrhi::RenderState& state) {
		setBlendState(state.blendState);
		setDepthStencilState(state.depthStencilState);
		setRasterState(state.rasterState);
	}
	
	//! Get the active render state.
	const nvrhi::RenderState& getRenderState() const;
	
	//! pushes the currently active render state to the render state stack.
	void pushRenderState() {
		pushBlendState();
		pushDepthStencilState();
		pushRasterState();
	}
	
	//! removes and activates the last render state on the render state stack.
	void popRenderState() {
		popBlendState();
		popDepthStencilState();
		popRasterState();
	}

	//! Push and set the active render state.
	void pushAndSetRenderState(const nvrhi::RenderState& state) {
		pushRenderState();
		setRenderState(state);
	}

	// @}

	//! @name Blend state
	// @{

	//! Set the active blend state.
	void setBlendState(const nvrhi::BlendState& state);
	
	//! Get the active blend state.
	const nvrhi::BlendState& getBlendState() const;
	
	//! pushes the currently active blend state to the blend state stack.
	void pushBlendState();
	
	//! removes and activates the last blend state on the blend state stack.
	void popBlendState();

	//! Push and set the active blend state.
	void pushAndSetBlendState(const nvrhi::BlendState& state) {
		pushBlendState();
		setBlendState(state);
	}

	// @}

	//! @name Raster state
	// @{	

	//! Set the active raster state.
	void setRasterState(const nvrhi::RasterState& state);
	
	//! Get the active raster state.
	const nvrhi::RasterState& getRasterState() const;

	//! pushes the currently active raster state to the raster state stack.
	void pushRasterState();
	
	//! removes and activates the last raster state on the raster state stack.
	void popRasterState();

	//! Push and set the active raster state.
	void pushAndSetRasterState(const nvrhi::RasterState& state) {
		pushRasterState();
		setRasterState(state);
	}

	// @}

	//! @name Depth/stencil state
	// @{

	//! Set the active depth/stencil state.
	void setDepthStencilState(const nvrhi::DepthStencilState& state);
	
	//! Get the active depth/stencil state.
	const nvrhi::DepthStencilState& getDepthStencilState() const;

	//! pushes the currently active depth/stencil state to the depth/stencil state stack.
	void pushDepthStencilState();
	
	//! removes and activates the last depth/stencil state on the depth/stencil state stack.
	void popDepthStencilState();

	//! Push and set the active depth/stencil state.
	void pushAndSetDepthStencilState(const nvrhi::DepthStencilState& state) {
		pushDepthStencilState();
		setDepthStencilState(state);
	}

	// @}

	//! @name Variable-rate shading state
	// @{
	
	//! Set the active variable-rate shading state.
	void setVariableRateShadingState(const nvrhi::VariableRateShadingState& state);
	
	//! Get the active variable-rate shading state.
	const nvrhi::VariableRateShadingState& getVariableRateShadingState() const;

	//! pushes the currently active variable-rate shading state to the variable-rate shading state stack.
	void pushVariableRateShadingState();
	
	//! removes and activates the last variable-rate shading state on the variable-rate shading state stack.
	void popVariableRateShadingState();

	//! Push and set the active variable-rate shading state.
	void pushAndSetVariableRateShadingState(const nvrhi::VariableRateShadingState& state) {
		pushVariableRateShadingState();
		setVariableRateShadingState(state);
	}

	// @}

private:
	struct Internal;
	std::unique_ptr<Internal> data;
};

} // Rendering

#endif // RENDERING_PIPELINEBUILDER_H_