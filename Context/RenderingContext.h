/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_RENDERINCONTEXT_H_
#define RENDERING_RENDERINCONTEXT_H_

#include <Util/CountedObjectWrapper.h>
#include <cstdint>
#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace Geometry {
template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4;
template<typename _T> class _Rect;
typedef _Rect<int> Rect_i;
}
namespace Util {
class Color4f;
class StringIdentifier;
struct AttributeFormat;
}
namespace Rendering {
class AlphaTestParameters;
class BlendingParameters;
class BufferObject;
class ClipPlaneParameters;
class ColorBlendState;
class ColorBufferParameters;
class CommandBuffer;
class CullFaceParameters;
class DepthBufferParameters;
class DepthStencilState;
class Device;
class FBO;
class ImageBindParameters;
class InputAssemblyState;
class LightData;
class LightingParameters;
class LightParameters;
class LineParameters;
class MaterialData;
class MaterialParameters;
class Mesh;
class MultisampleState;
class PointParameters;
class PolygonModeParameters;
class PolygonOffsetParameters;
class PrimitiveRestartParameters;
class RasterizationState;
class ScissorParameters;
class Shader;
class Shader;
class StencilParameters;
class Texture;
class Uniform;
class UniformRegistry;
class VertexDescription;
class VertexInputState;
class ViewportState;
class PipelineState;
class RenderingState;
enum class PrimitiveTopology;
enum class TexUnitUsageParameter : uint8_t;
using BufferObjectRef = Util::Reference<BufferObject>;
using CommandBufferRef = Util::Reference<CommandBuffer>;
using DeviceRef = Util::Reference<Device>;
using FBORef = Util::Reference<FBO>;
using ShaderRef = Util::Reference<Shader>;
using TextureRef = Util::Reference<Texture>;
using MeshRef = Util::Reference<Mesh>;

//! @defgroup context Rendering Context

//! @ingroup context
class RenderingContext {

	//! @name General
	//	@{
private:
	// Use Pimpl idiom
	class InternalData;
	std::unique_ptr<InternalData> internal;

public:

	RENDERINGAPI RenderingContext();
	RENDERINGAPI RenderingContext(const DeviceRef& device);
	RENDERINGAPI ~RenderingContext();

	//! has to return true iff normal display of mesh shall be executed
	typedef std::function<void (RenderingContext& rc,Mesh * mesh,uint32_t firstElement,uint32_t elementCount)> DisplayMeshFn;
private:
	DisplayMeshFn displayMeshFn;
public:
	void setDisplayMeshFn(DisplayMeshFn fn){ displayMeshFn = fn; };
	RENDERINGAPI void resetDisplayMeshFn();

	void displayMesh(Mesh * mesh,uint32_t firstElement,uint32_t elementCount){ displayMeshFn(*this, mesh,firstElement,elementCount); }
	RENDERINGAPI void displayMesh(Mesh * mesh);

	void displayMesh(const MeshRef& mesh,uint32_t firstElement,uint32_t elementCount){ displayMeshFn(*this, mesh.get(),firstElement,elementCount); }
	void displayMesh(const MeshRef& mesh) { displayMesh(mesh.get()); }

	[[deprecated]]
	void setImmediateMode(const bool enabled) {}
	[[deprecated]]
	bool getImmediateMode() const { return false; }
	[[deprecated]]
	static bool getCompabilityMode() { return false; }

	RENDERINGAPI void applyChanges(bool forced = false);

	RENDERINGAPI const DeviceRef& getDevice() const;
	RENDERINGAPI CommandBufferRef getCommandBuffer() const;
	RENDERINGAPI const PipelineState& getPipelineState() const;
	RENDERINGAPI const RenderingState& getRenderingState() const;
	//	@}

	// -----------------------------------

	/*!	@name GL Helper */
	//	@{

	[[deprecated]]
	static void initGLState() {}
	
	[[deprecated]]
	static bool useAMDAttrBugWorkaround() { return false;}

	/**
	 * Flush the GL commands buffer.
	 * @see glFlush
	 */
	RENDERINGAPI void flush(bool wait=false);

	/**
	 * Block until all GL commands are complete.
	 * @see glFinish
	 */
	void finish() { flush(true); }

	//! Renders to screen& swaps buffers
	RENDERINGAPI void present();
	
	/**
	 * Defines a barrier ordering memory transactions.
	 * @see glMemoryBarrier
	 */
	RENDERINGAPI void barrier(uint32_t flags=0);
	
	// @}

	/*!	@name Clear */
	//	@{
	RENDERINGAPI void clearColor(const Util::Color4f& color);
	RENDERINGAPI void clearDepth(float clearValue);
	RENDERINGAPI void clearStencil(uint32_t clearValue);
	RENDERINGAPI void clearScreen(const Util::Color4f& color);
	RENDERINGAPI void clearScreenRect(const Geometry::Rect_i& rect, const Util::Color4f& color, bool clearDepth=true, bool clearStencil=true);
	// @}
	// --------------------------------------------------------------------
	// --------------------------------------------------------------------
	// Parameters (sorted alphabetically)

	//! @name AlphaTest (deprecated)
	//	@{
	[[deprecated]]
	RENDERINGAPI const AlphaTestParameters getAlphaTestParameters() const;
	[[deprecated]]
	RENDERINGAPI void popAlphaTest();
	[[deprecated]]
	RENDERINGAPI void pushAlphaTest();
	[[deprecated]]
	RENDERINGAPI void pushAndSetAlphaTest(const AlphaTestParameters& alphaTestParameter);
	[[deprecated]]
	RENDERINGAPI void setAlphaTest(const AlphaTestParameters& alphaTestParameter);
	// @}
	
	// ------

	//! @name Atomic counters (deprecated)
	//	@{
	[[deprecated]]
	static bool isAtomicCountersSupported() { return false; }
	[[deprecated]]
	static uint32_t getMaxAtomicCounterBuffers() { return 0; }
	[[deprecated]]
	static uint32_t getMaxAtomicCounterBufferSize() { return 0; }
	[[deprecated]]
	Texture* getAtomicCounterTextureBuffer(uint32_t index) const { return nullptr; }
	[[deprecated]]
	void pushAtomicCounterTextureBuffer(uint32_t index) {}
	[[deprecated]]
	void pushAndSetAtomicCounterTextureBuffer(uint32_t index, Texture* bufferDataTexture) {}
	[[deprecated]]
	void popAtomicCounterTextureBuffer(uint32_t pushAtomicCounterTexture) {}
	[[deprecated]]
	void setAtomicCounterTextureBuffer(uint32_t pushAtomicCounterTexture, Texture* bufferDataTexture) {}
	// @}
	// ------

	//! @name Blending
	//	@{
	RENDERINGAPI const ColorBlendState& getBlending() const;
	RENDERINGAPI void popBlending();
	RENDERINGAPI void pushBlending();
	RENDERINGAPI void pushAndSetBlending(const ColorBlendState& state);
	RENDERINGAPI void setBlending(const ColorBlendState& state);

	[[deprecated]]
	RENDERINGAPI const BlendingParameters getBlendingParameters() const;
	[[deprecated]]
	RENDERINGAPI void pushAndSetBlending(const BlendingParameters& blendingParameter);
	[[deprecated]]
	RENDERINGAPI void setBlending(const BlendingParameters& blendingParameter);
	// @}
	
	// ------

	//! @name Clip plane
	//	@{
	[[deprecated]]
	RENDERINGAPI const ClipPlaneParameters getClipPlane(uint8_t index) const;
	[[deprecated]]
	void popClipPlane(uint8_t index) {}
	[[deprecated]]
	void pushClipPlane(uint8_t index) {}
	[[deprecated]]
	void pushAndSetClipPlane(uint8_t index, const ClipPlaneParameters& planeParameters) {}
	[[deprecated]]
	void setClipPlane(uint8_t index, const ClipPlaneParameters& planeParameters) {}
	// @}
	
	// ------

	//! @name ColorBuffer
	//	@{
	[[deprecated]]
	RENDERINGAPI const ColorBufferParameters getColorBufferParameters() const;
	[[deprecated]]
	RENDERINGAPI void popColorBuffer();
	[[deprecated]]
	RENDERINGAPI void pushColorBuffer();
	[[deprecated]]
	RENDERINGAPI void pushAndSetColorBuffer(const ColorBufferParameters& colorBufferParameter);
	[[deprecated]]
	RENDERINGAPI void setColorBuffer(const ColorBufferParameters& colorBufferParameter);

	// @}
	// ------
	
	//! @name Compute
	//	@{
	RENDERINGAPI void dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY=1, uint32_t numGroupsZ=1);
	RENDERINGAPI void dispatchComputeIndirect(size_t offset=0);
	[[deprecated]]
	RENDERINGAPI void loadUniformSubroutines(uint32_t shaderStage, const std::vector<uint32_t>& indices);
	[[deprecated]]
	RENDERINGAPI void loadUniformSubroutines(uint32_t shaderStage, const std::vector<std::string>& names);
	// @}


	//! @name CullFace
	//	@{
	[[deprecated]]
	RENDERINGAPI const CullFaceParameters getCullFaceParameters() const;
	[[deprecated]]
	RENDERINGAPI void popCullFace();
	[[deprecated]]
	RENDERINGAPI void pushCullFace();
	[[deprecated]]
	RENDERINGAPI void pushAndSetCullFace(const CullFaceParameters& cullFaceParameters);
	[[deprecated]]
	RENDERINGAPI void setCullFace(const CullFaceParameters& cullFaceParameters);
	// @}
	
	// ------

	//! @name DepthStencil
	//	@{
	RENDERINGAPI const DepthStencilState& getDepthStencil() const;
	RENDERINGAPI void popDepthStencil();
	RENDERINGAPI void pushDepthStencil();
	RENDERINGAPI void pushAndSetDepthStencil(const DepthStencilState& state);
	RENDERINGAPI void setDepthStencil(const DepthStencilState& state);
	// @}

	// ------

	//! @name DepthBuffer
	//	@{
	[[deprecated]]
	RENDERINGAPI const DepthBufferParameters getDepthBufferParameters() const;
	[[deprecated]]
	RENDERINGAPI void popDepthBuffer();
	[[deprecated]]
	RENDERINGAPI void pushDepthBuffer();
	[[deprecated]]
	RENDERINGAPI void pushAndSetDepthBuffer(const DepthBufferParameters& depthBufferParameter);
	[[deprecated]]
	RENDERINGAPI void setDepthBuffer(const DepthBufferParameters& depthBufferParameter);

	// @}

	// ------

	//! @name Drawing
	//	@{
	RENDERINGAPI void bindVertexBuffer(const BufferObjectRef& buffer, const VertexDescription& vd);
	RENDERINGAPI void bindVertexBuffers(const std::vector<BufferObjectRef>& buffers, const std::vector<VertexDescription>& vds, const std::vector<uint32_t> rates={});
	RENDERINGAPI void bindIndexBuffer(const BufferObjectRef& buffer);
	RENDERINGAPI void draw(uint32_t vertexCount, uint32_t firstVertex=0, uint32_t instanceCount=1, uint32_t firstInstance=0);
	RENDERINGAPI void drawIndexed(uint32_t indexCount, uint32_t firstIndex=0, uint32_t vertexOffset=0, uint32_t instanceCount=1, uint32_t firstInstance=0);
	RENDERINGAPI void drawIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride);
	RENDERINGAPI void setPrimitiveTopology(PrimitiveTopology topology);
	// @}

	// ------

	//! @name FBO
	//	@{
	[[deprecated]]
	RENDERINGAPI FBO * getActiveFBO() const;
	RENDERINGAPI FBORef getFBO() const;
	RENDERINGAPI void popFBO();
	RENDERINGAPI void pushFBO();
	RENDERINGAPI void pushAndSetFBO(const FBORef& fbo);
	RENDERINGAPI void setFBO(const FBORef& fbo);
	// @}

	// ------

	//! @name Global Uniforms
	//	@{
	RENDERINGAPI void setGlobalUniform(const Uniform& u);
	RENDERINGAPI const Uniform& getGlobalUniform(const Util::StringIdentifier& uniformName);
	// @}

	// ------

	//! @name Image Binding (Image load and store)
	//	@{
	static bool isImageBindingSupported() { return true; }
	RENDERINGAPI ImageBindParameters getBoundImage(uint8_t unit, uint8_t set=0) const;
	RENDERINGAPI void pushBoundImage(uint8_t unit, uint8_t set=0);
	RENDERINGAPI void pushAndSetBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set=0); 
	RENDERINGAPI void popBoundImage(uint8_t unit, uint8_t set=0);
	//! \note the texture in iParam may be null to unbind
	RENDERINGAPI void setBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set=0);
	// @}

	// ------

	//! @name Lighting
	//	@{
	[[deprecated]]
	RENDERINGAPI const LightingParameters getLightingParameters() const;
	[[deprecated]]
	void popLighting() {}
	[[deprecated]]
	void pushLighting() {}
	[[deprecated]]
	void pushAndSetLighting(const LightingParameters& lightingParameter) {}
	[[deprecated]]
	void setLighting(const LightingParameters& lightingParameter) {}

	// ------

	//! @name Lights
	//	@{
	/**
	 * Activate the light given by the parameters.
	 *
	 * @param light Parameters of a light source.
	 * @return Light number that was used for this light. This number has to be used to deactivate the light.
	 */
	RENDERINGAPI size_t enableLight(const LightData& light);
	[[deprecated]]
	RENDERINGAPI size_t enableLight(const LightParameters& light);

	/**
	 * Deactivate a previuosly activated light.
	 *
	 * @param lightNumber Light number that was returned by @a enableLight.
	 */
	RENDERINGAPI void disableLight(size_t lightNumber);
	// @}

	// ------

	//! @name Line
	//	@{
	[[deprecated]]
	RENDERINGAPI const LineParameters getLineParameters() const;
	[[deprecated]]
	RENDERINGAPI void popLine();
	[[deprecated]]
	RENDERINGAPI void pushLine();
	[[deprecated]]
	RENDERINGAPI void pushAndSetLine(const LineParameters& lineParameters);
	[[deprecated]]
	RENDERINGAPI void setLine(const LineParameters& lineParameters);
	// @}

	// ------

	//! @name Material
	//	@{
	//! Return the active material.
	RENDERINGAPI const MaterialData& getActiveMaterial() const;
	[[deprecated]]
	RENDERINGAPI const MaterialParameters getMaterial() const;
	//! Pop a material from the top of the stack and activate it. Deactivate material usage if stack is empty.
	RENDERINGAPI void popMaterial();
	//! Push the given material onto the material stack.
	RENDERINGAPI void pushMaterial();
	//! Push the given material onto the material stack and activate it.
	RENDERINGAPI void pushAndSetMaterial(const MaterialData& material);
	[[deprecated]]
	RENDERINGAPI void pushAndSetMaterial(const MaterialParameters& material);
	//! Convert the given color to a material, and call @a pushAndSetMaterial
	RENDERINGAPI void pushAndSetColorMaterial(const Util::Color4f& color);
	//! Activate the given material.
	RENDERINGAPI void setMaterial(const MaterialData& material);
	[[deprecated]]
	RENDERINGAPI void setMaterial(const MaterialParameters& material);

	// @}
	// ------

	/*! @name Matrix CameraToWorld / WorldToCamera
	 camera matrix == inverse world matrix of camera node == default model view matrix	*/
	//	@{
	RENDERINGAPI void setMatrix_cameraToWorld(const Geometry::Matrix4x4& matrix);	//!< formerly known as setInverseCameraMatrix
	RENDERINGAPI const Geometry::Matrix4x4& getMatrix_worldToCamera() const;		//!< formerly known as getCameraMatrix
	RENDERINGAPI const Geometry::Matrix4x4& getMatrix_cameraToWorld() const;		//!< formerly known as getInverseCameraMatrix
	//	@}

	// ------

	//! @name Matrix ModelToCamera (Legacy Model View Matrix)
	//	@{
	//! resets the model view matrix to the default (camera matrix)
	RENDERINGAPI void resetMatrix();  //! \note use renderingContext.setMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() ) instead!
	RENDERINGAPI const Geometry::Matrix4x4& getMatrix_modelToCamera() const;		//!< formerly known as getMatrix
	RENDERINGAPI void multMatrix_modelToCamera(const Geometry::Matrix4x4& matrix);	//!< formerly known as multMatrix
	RENDERINGAPI void pushMatrix_modelToCamera();									//!< formerly known as pushMatrix
	RENDERINGAPI void pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4& matrix);
	RENDERINGAPI void setMatrix_modelToCamera(const Geometry::Matrix4x4& matrix);	//!< formerly known as setMatrix
	RENDERINGAPI void popMatrix_modelToCamera();										//!< formerly known as popMatrix
	//	@}
	
	// ------

	//! @name Matrix CameraToClipping (Legacy Projection Matrix)
	//	@{
	RENDERINGAPI const Geometry::Matrix4x4& getMatrix_cameraToClipping() const;			//! formerly known as getProjectionMatrix
	RENDERINGAPI void pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix);
	RENDERINGAPI void pushMatrix_cameraToClipping();										//! formerly known as pushProjectionMatrix
	RENDERINGAPI void popMatrix_cameraToClipping();										//! formerly known as popProjectionMatrix
	RENDERINGAPI void setMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix);	//! formerly known as setProjectionMatrix
	// @}
	
	// ------

	//! @name Point
	//	@{
	[[deprecated]]
	RENDERINGAPI const PointParameters getPointParameters() const;
	[[deprecated]]
	RENDERINGAPI void popPointParameters();
	[[deprecated]]
	RENDERINGAPI void pushPointParameters();
	[[deprecated]]
	RENDERINGAPI void pushAndSetPointParameters(const PointParameters& pointParameters);
	[[deprecated]]
	RENDERINGAPI void setPointParameters(const PointParameters& pointParameters);
	// @}
	// ------

	//! @name PolygonMode
	//	@{
	[[deprecated]]
	RENDERINGAPI const PolygonModeParameters getPolygonModeParameters() const;
	[[deprecated]]
	RENDERINGAPI void popPolygonMode();
	[[deprecated]]
	RENDERINGAPI void pushPolygonMode();
	[[deprecated]]
	RENDERINGAPI void pushAndSetPolygonMode(const PolygonModeParameters& polygonModeParameter);
	[[deprecated]]
	RENDERINGAPI void setPolygonMode(const PolygonModeParameters& polygonModeParameter);
	// @}

	// ------

	//! @name PolygonOffset
	//	@{
	[[deprecated]]
	RENDERINGAPI const PolygonOffsetParameters getPolygonOffsetParameters() const;
	[[deprecated]]
	RENDERINGAPI void popPolygonOffset();
	[[deprecated]]
	RENDERINGAPI void pushPolygonOffset();
	[[deprecated]]
	RENDERINGAPI void pushAndSetPolygonOffset(const PolygonOffsetParameters& polygonOffsetParameter);
	[[deprecated]]
	RENDERINGAPI void setPolygonOffset(const PolygonOffsetParameters& polygonOffsetParameter);
	// @}
	
	// ------

	//! @name Primitive restart
	//	@{
	[[deprecated]]
	RENDERINGAPI const PrimitiveRestartParameters getPrimitiveRestartParameters() const;
	[[deprecated]]
	RENDERINGAPI void popPrimitiveRestart();
	[[deprecated]]
	RENDERINGAPI void pushPrimitiveRestart();
	[[deprecated]]
	RENDERINGAPI void pushAndSetPrimitiveRestart(const PrimitiveRestartParameters& parameters);
	[[deprecated]]
	RENDERINGAPI void setPrimitiveRestart(const PrimitiveRestartParameters& parameters);
	// @}
	
	// ------

	//! @name Rasterization
	//	@{
	RENDERINGAPI const RasterizationState& getRasterization() const;
	RENDERINGAPI void popRasterization();
	RENDERINGAPI void pushRasterization();
	RENDERINGAPI void pushAndSetRasterization(const RasterizationState& state);
	RENDERINGAPI void setRasterization(const RasterizationState& state);
	// @}


	// ------

	//! @name Shader
	//	@{
	RENDERINGAPI void pushAndSetShader(const ShaderRef& shader);
	RENDERINGAPI void pushShader();
	RENDERINGAPI void popShader();
	RENDERINGAPI bool isShaderEnabled(const ShaderRef& shader);
	RENDERINGAPI const ShaderRef& getActiveShader() const;
	RENDERINGAPI const ShaderRef& getFallbackShader() const;
	RENDERINGAPI void setShader(const ShaderRef& shader);

	//! (internal) called by Shader::setUniform(...)
	RENDERINGAPI void _setUniformOnShader(const ShaderRef& shader, const Uniform& uniform, bool warnIfUnused, bool forced);

	// @}

	// ------

	//! @name Scissor
	//	@{
	[[deprecated]]
	RENDERINGAPI const ScissorParameters getScissor() const;
	[[deprecated]]
	RENDERINGAPI void popScissor();
	[[deprecated]]
	RENDERINGAPI void pushScissor();
	[[deprecated]]
	RENDERINGAPI void pushAndSetScissor(const ScissorParameters& scissorParameters);
	[[deprecated]]
	RENDERINGAPI void setScissor(const ScissorParameters& scissorParameters);
	// @}

// ------

	//! @name Stencil
	//	@{
	[[deprecated]]
	RENDERINGAPI const StencilParameters getStencilParamters() const;
	[[deprecated]]
	RENDERINGAPI void popStencil();
	[[deprecated]]
	RENDERINGAPI void pushStencil();
	[[deprecated]]
	RENDERINGAPI void pushAndSetStencil(const StencilParameters& stencilParameter);
	[[deprecated]]
	RENDERINGAPI void setStencil(const StencilParameters& stencilParameter);

	// @}

	// ------

	/*! @name Textures
	 \todo Move array of activeTextures to RenderingStatus to allow delayed binding
	 */
	//	@{
	RENDERINGAPI const TextureRef getTexture(uint32_t index=0, uint32_t binding=0, uint32_t set=0) const;
	[[deprecated]]
	RENDERINGAPI TexUnitUsageParameter getTextureUsage(uint32_t unit) const;
	RENDERINGAPI void pushTexture(uint32_t index=0, uint32_t binding=0, uint32_t set=0);
	RENDERINGAPI void pushAndSetTexture(const TextureRef& texture, uint32_t index=0, uint32_t binding=0, uint32_t set=0);
	[[deprecated]]
	void pushAndSetTexture(uint32_t unit, const TextureRef& texture, TexUnitUsageParameter usage) { pushAndSetTexture(texture, unit, 0, 0); }
	[[deprecated]]
	void pushAndSetTexture(uint32_t unit, const TextureRef& texture) { pushAndSetTexture(texture, unit, 0, 0); }
	RENDERINGAPI void popTexture(uint32_t index=0, uint32_t binding=0, uint32_t set=0);

	//! \note texture may be nullptr
	RENDERINGAPI void setTexture(const TextureRef& texture, uint32_t index=0, uint32_t binding=0, uint32_t set=0);
	[[deprecated]]
	void setTexture(uint32_t unit, const TextureRef& texture, TexUnitUsageParameter usage) { setTexture(texture, unit, 0, 0); }
	[[deprecated]]
	void setTexture(uint32_t unit, const TextureRef& texture) { setTexture(texture, unit, 0, 0); }
	// @}
	
	// ------

	//! @name Transform Feedback
	//	@{
	[[deprecated]]
	static bool isTransformFeedbackSupported() { return false; };
	[[deprecated]]
	static bool requestTransformFeedbackSupport() { return false; };
	[[deprecated]]
	BufferObject * getActiveTransformFeedbackBuffer() const { return nullptr; }
	[[deprecated]]
	void popTransformFeedbackBufferStatus() {}
	[[deprecated]]
	void pushTransformFeedbackBufferStatus() {}
	[[deprecated]]
	void setTransformFeedbackBuffer(BufferObject * buffer) {}
	[[deprecated]]
	void _startTransformFeedback(uint32_t) {}
	[[deprecated]]
	void startTransformFeedback_lines() {}
	[[deprecated]]
	void startTransformFeedback_points() {}
	[[deprecated]]
	void startTransformFeedback_triangles() {}
	[[deprecated]]
	void stopTransformFeedback() {}

	// @}

	// ------

	//! @name VBO Client States
	// @{
	//! Activate the given client state.
	[[deprecated]]
	void enableClientState(uint32_t clientState) {}

	//! Deactivate all client states that were activated before.
	[[deprecated]]
	void disableAllClientStates() {}

	//! Activate the texture coordinate client state for the given texture unit.
	[[deprecated]]
	void enableTextureClientState(uint32_t textureUnit) {}

	//! Deactivate the texture coordinate client states for all texture units that were activated before.
	[[deprecated]]
	void disableAllTextureClientStates() {}

	/**
	 * Bind a vertex attribute to a variable inside a shader program.
	 *
	 * @param attr Attribute description (including variable name)
	 * @param data Pointer to the vertex data (or @c nullptr if a buffer object is active)
	 * @param stride Size of a vertex in bytes
	 */
	[[deprecated]]
	void enableVertexAttribArray(const Util::AttributeFormat& attr, const uint8_t * data, int32_t stride) {}

	//! Disable all vertex attribute array.
	[[deprecated]]
	void disableAllVertexAttribArrays() {}
	// @}

	// ------

	//! @name Viewport and window's size
	// @{
	/**
	 * Get the OpenGL window's client area.
	 * In almost all cases, the position will be (0, 0).
	 * The width and height differs with the size of the window.
	 * @note This value has to be set manually by calling setWindowClientArea() after creating the RenderingContext
	 */
	RENDERINGAPI const Geometry::Rect_i& getWindowClientArea() const;

	//! Read the current viewport.
	RENDERINGAPI const Geometry::Rect_i& getViewport() const;

	//! Read the current viewport state.
	RENDERINGAPI const ViewportState& getViewportState() const;

	//! Restore the viewport from the top of the viewport stack.
	RENDERINGAPI void popViewport();

	//! Save the current viewport onto the viewport stack.
	RENDERINGAPI void pushViewport();

	//! Set the current viewport.
	RENDERINGAPI void setViewport(const Geometry::Rect_i& viewport);
	RENDERINGAPI void setViewport(const Geometry::Rect_i& viewport, const Geometry::Rect_i& scissor);
	RENDERINGAPI void setViewport(const ViewportState& viewport);

	//! Save the current viewport onto the viewport stack and set the current viewport.
	RENDERINGAPI void pushAndSetViewport(const Geometry::Rect_i& viewport);
	RENDERINGAPI void pushAndSetViewport(const Geometry::Rect_i& viewport, const Geometry::Rect_i& scissor);
	RENDERINGAPI void pushAndSetViewport(const ViewportState& viewport);

	RENDERINGAPI void setWindowClientArea(const Geometry::Rect_i& clientArea);
	// @}
};

}

#endif /* RENDERING_RENDERINCONTEXT_H_ */
