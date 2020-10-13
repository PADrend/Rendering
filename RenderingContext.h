/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2018 Sascha Brandt <sascha@brandt.graphics>
	
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

namespace Geometry {
template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4;
template<typename _T> class _Rect;
typedef _Rect<int> Rect_i;
}
namespace Util {
class Color4f;
class StringIdentifier;
class AttributeFormat;
}
namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class FBO;
using FBORef = Util::Reference<FBO>;
class Shader;
using ShaderRef = Util::Reference<Shader>;
class Texture;
using TextureRef = Util::Reference<Texture>;
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class AlphaTestParameters;
class BlendingParameters;
class ClipPlaneParameters;
class ColorBufferParameters;
class CullFaceParameters;
class DepthBufferParameters;
class ImageBindParameters;
class LightParameters;
class LightingParameters;
class LineParameters;
class MaterialParameters;
class Mesh;
class PointParameters;
class PolygonModeParameters;
class PolygonOffsetParameters;
class PrimitiveRestartParameters;
class ScissorParameters;
class StencilParameters;
class Shader;
class Uniform;
class UniformRegistry;
class VertexDescription;
enum class TexUnitUsageParameter : uint8_t;
enum class PrimitiveTopology;

class VertexInputState;
class InputAssemblyState;
class ViewportState;
class RasterizationState;
class MultisampleState;
class DepthStencilState;
class ColorBlendState;

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

	RenderingContext();
	RenderingContext(const DeviceRef& device);
	~RenderingContext();

	//! has to return true iff normal display of mesh shall be executed
	typedef std::function<void (RenderingContext& rc,Mesh * mesh,uint32_t firstElement,uint32_t elementCount)> DisplayMeshFn;
private:
	DisplayMeshFn displayMeshFn;
public:
	void setDisplayMeshFn(DisplayMeshFn fn){ displayMeshFn = fn; };
	void resetDisplayMeshFn();

	void displayMesh(Mesh * mesh,uint32_t firstElement,uint32_t elementCount){ displayMeshFn(*this, mesh,firstElement,elementCount); }
	void displayMesh(Mesh * mesh);

	[[deprecated]]
	void setImmediateMode(const bool enabled) {}
	[[deprecated]]
	bool getImmediateMode() const { return false; }
	[[deprecated]]
	static bool getCompabilityMode() { return false; }

	void applyChanges(bool forced = false);
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
	void flush(bool wait=false);

	/**
	 * Block until all GL commands are complete.
	 * @see glFinish
	 */
	void finish() { flush(true); }

	//! Renders to screen& swaps buffers
	void present();
	
	/**
	 * Defines a barrier ordering memory transactions.
	 * @see glMemoryBarrier
	 */
	void barrier(uint32_t flags=0);
	
	// @}

	/*!	@name Clear */
	//	@{
	void clearColor(const Util::Color4f& color);
	void clearDepth(float clearValue);
	void clearStencil(int32_t clearValue);
	void clearScreen(const Util::Color4f& color);
	void clearScreenRect(const Geometry::Rect_i& rect, const Util::Color4f& color, bool clearDepth=true);
	// @}
	// --------------------------------------------------------------------
	// --------------------------------------------------------------------
	// Parameters (sorted alphabetically)

	//! @name AlphaTest (deprecated)
	//	@{
	[[deprecated]]
	const AlphaTestParameters& getAlphaTestParameters() const;
	[[deprecated]]
	void popAlphaTest() {}
	[[deprecated]]
	void pushAlphaTest() {}
	[[deprecated]]
	void pushAndSetAlphaTest(const AlphaTestParameters& alphaTestParameter) {}
	[[deprecated]]
	void setAlphaTest(const AlphaTestParameters& alphaTestParameter) {}
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
	const ColorBlendState& getBlending() const;
	void popBlending();
	void pushBlending();
	void pushAndSetBlending(const ColorBlendState& state);
	void setBlending(const ColorBlendState& state);

	[[deprecated]]
	const BlendingParameters& getBlendingParameters() const;
	[[deprecated]]
	void pushAndSetBlending(const BlendingParameters& blendingParameter);
	[[deprecated]]
	void setBlending(const BlendingParameters& blendingParameter);
	// @}
	
	// ------

	//! @name Clip plane
	//	@{
	[[deprecated]]
	const ClipPlaneParameters& getClipPlane(uint8_t index) const;
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
	const ColorBufferParameters& getColorBufferParameters() const;
	[[deprecated]]
	void popColorBuffer();
	[[deprecated]]
	void pushColorBuffer();
	[[deprecated]]
	void pushAndSetColorBuffer(const ColorBufferParameters& colorBufferParameter);
	[[deprecated]]
	void setColorBuffer(const ColorBufferParameters& colorBufferParameter);

	// @}
	// ------
	
	//! @name Compute
	//	@{
	void dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY=1, uint32_t numGroupsZ=1);
	void dispatchComputeIndirect(size_t offset=0);
	[[deprecated]]
	void loadUniformSubroutines(uint32_t shaderStage, const std::vector<uint32_t>& indices);
	[[deprecated]]
	void loadUniformSubroutines(uint32_t shaderStage, const std::vector<std::string>& names);
	// @}


	//! @name CullFace
	//	@{
	[[deprecated]]
	const CullFaceParameters& getCullFaceParameters() const;
	[[deprecated]]
	void popCullFace();
	[[deprecated]]
	void pushCullFace();
	[[deprecated]]
	void pushAndSetCullFace(const CullFaceParameters& cullFaceParameters);
	[[deprecated]]
	void setCullFace(const CullFaceParameters& cullFaceParameters);
	// @}
	
	// ------

	//! @name DepthStencil
	//	@{
	const DepthStencilState& getDepthStencil() const;
	void popDepthStencil();
	void pushDepthStencil();
	void pushAndSetDepthStencil(const DepthStencilState& state);
	void setDepthStencil(const DepthStencilState& state);
	// @}

	// ------

	//! @name DepthBuffer
	//	@{
	[[deprecated]]
	const DepthBufferParameters& getDepthBufferParameters() const;
	[[deprecated]]
	void popDepthBuffer();
	[[deprecated]]
	void pushDepthBuffer();
	[[deprecated]]
	void pushAndSetDepthBuffer(const DepthBufferParameters& depthBufferParameter);
	[[deprecated]]
	void setDepthBuffer(const DepthBufferParameters& depthBufferParameter);

	// @}

	// ------

	//! @name Drawing
	//	@{
	void bindVertexBuffer(const BufferObjectRef& buffer, const VertexDescription& vd);
	void bindIndexBuffer(const BufferObjectRef& buffer);
	void draw(uint32_t vertexCount, uint32_t firstVertex=0, uint32_t instanceCount=1, uint32_t firstInstance=0);
	void drawIndexed(uint32_t indexCount, uint32_t firstIndex=0, uint32_t vertexOffset=0, uint32_t instanceCount=1, uint32_t firstInstance=0);
	void setPrimitiveTopology(PrimitiveTopology topology);
	// @}

	// ------

	//! @name FBO
	//	@{
	[[deprecated]]
	FBO * getActiveFBO() const;
	FBORef getFBO() const;
	void popFBO();
	void pushFBO();
	void pushAndSetFBO(const FBORef& fbo);
	void setFBO(const FBORef& fbo);
	// @}

	// ------

	//! @name Global Uniforms
	//	@{
	void setGlobalUniform(const Uniform& u);
	const Uniform& getGlobalUniform(const Util::StringIdentifier& uniformName);
	// @}

	// ------

	//! @name Image Binding (Image load and store)
	//	@{
	static bool isImageBindingSupported() { return true; }
	ImageBindParameters getBoundImage(uint8_t unit, uint8_t set=0) const;
	void pushBoundImage(uint8_t unit, uint8_t set=0);
	void pushAndSetBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set=0); 
	void popBoundImage(uint8_t unit, uint8_t set=0);
	//! \note the texture in iParam may be null to unbind
	void setBoundImage(uint8_t unit, const ImageBindParameters& iParam, uint8_t set=0);
	// @}

	// ------

	//! @name Lighting
	//	@{
	const LightingParameters& getLightingParameters() const;
	void popLighting();
	void pushLighting();
	void pushAndSetLighting(const LightingParameters& lightingParameter);
	void setLighting(const LightingParameters& lightingParameter);

	// ------

	//! @name Lights
	//	@{
	/**
	 * Activate the light given by the parameters.
	 *
	 * @param light Parameters of a light source.
	 * @return Light number that was used for this light. This number has to be used to deactivate the light.
	 */
	uint8_t enableLight(const LightParameters& light);

	/**
	 * Deactivate a previuosly activated light.
	 *
	 * @param lightNumber Light number that was returned by @a enableLight.
	 */
	void disableLight(uint8_t lightNumber);
	// @}

	// ------

	//! @name Line
	//	@{
	[[deprecated]]
	const LineParameters& getLineParameters() const;
	[[deprecated]]
	void popLine();
	[[deprecated]]
	void pushLine();
	[[deprecated]]
	void pushAndSetLine(const LineParameters& lineParameters);
	[[deprecated]]
	void setLine(const LineParameters& lineParameters);
	// @}

	// ------

	//! @name Material
	//	@{
	//! Return the active material.
	const MaterialParameters& getMaterial() const;
	//! Pop a material from the top of the stack and activate it. Deactivate material usage if stack is empty.
	void popMaterial();
	//! Push the given material onto the material stack.
	void pushMaterial();
	//! Push the given material onto the material stack and activate it.
	void pushAndSetMaterial(const MaterialParameters& material);
	//! Convert the given color to a material, and call @a pushAndSetMaterial
	void pushAndSetColorMaterial(const Util::Color4f& color);
	//! Activate the given material.
	void setMaterial(const MaterialParameters& material);


	// @}
	// ------

	/*! @name Matrix CameraToWorld / WorldToCamera
	 camera matrix == inverse world matrix of camera node == default model view matrix	*/
	//	@{
	void setMatrix_cameraToWorld(const Geometry::Matrix4x4& matrix);	//!< formerly known as setInverseCameraMatrix
	const Geometry::Matrix4x4& getMatrix_worldToCamera() const;		//!< formerly known as getCameraMatrix
	const Geometry::Matrix4x4& getMatrix_cameraToWorld() const;		//!< formerly known as getInverseCameraMatrix
	//	@}

	// ------

	//! @name Matrix ModelToCamera (Legacy Model View Matrix)
	//	@{
	//! resets the model view matrix to the default (camera matrix)
	void resetMatrix();  //! \note use renderingContext.setMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() ) instead!
	const Geometry::Matrix4x4& getMatrix_modelToCamera() const;		//!< formerly known as getMatrix
	void multMatrix_modelToCamera(const Geometry::Matrix4x4& matrix);	//!< formerly known as multMatrix
	void pushMatrix_modelToCamera();									//!< formerly known as pushMatrix
	void pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4& matrix);
	void setMatrix_modelToCamera(const Geometry::Matrix4x4& matrix);	//!< formerly known as setMatrix
	void popMatrix_modelToCamera();										//!< formerly known as popMatrix
	//	@}
	
	// ------

	//! @name Matrix CameraToClipping (Legacy Projection Matrix)
	//	@{
	const Geometry::Matrix4x4& getMatrix_cameraToClipping() const;			//! formerly known as getProjectionMatrix
	void pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix);
	void pushMatrix_cameraToClipping();										//! formerly known as pushProjectionMatrix
	void popMatrix_cameraToClipping();										//! formerly known as popProjectionMatrix
	void setMatrix_cameraToClipping(const Geometry::Matrix4x4& matrix);	//! formerly known as setProjectionMatrix
	// @}
	
	// ------

	//! @name Point
	//	@{
	[[deprecated]]
	const PointParameters& getPointParameters() const;
	[[deprecated]]
	void popPointParameters();
	[[deprecated]]
	void pushPointParameters();
	[[deprecated]]
	void pushAndSetPointParameters(const PointParameters& pointParameters);
	[[deprecated]]
	void setPointParameters(const PointParameters& pointParameters);
	// @}
	// ------

	//! @name PolygonMode
	//	@{
	[[deprecated]]
	const PolygonModeParameters& getPolygonModeParameters() const;
	[[deprecated]]
	void popPolygonMode();
	[[deprecated]]
	void pushPolygonMode();
	[[deprecated]]
	void pushAndSetPolygonMode(const PolygonModeParameters& polygonModeParameter);
	[[deprecated]]
	void setPolygonMode(const PolygonModeParameters& polygonModeParameter);
	// @}

	// ------

	//! @name PolygonOffset
	//	@{
	[[deprecated]]
	const PolygonOffsetParameters& getPolygonOffsetParameters() const;
	[[deprecated]]
	void popPolygonOffset();
	[[deprecated]]
	void pushPolygonOffset();
	[[deprecated]]
	void pushAndSetPolygonOffset(const PolygonOffsetParameters& polygonOffsetParameter);
	[[deprecated]]
	void setPolygonOffset(const PolygonOffsetParameters& polygonOffsetParameter);
	// @}
	
	// ------

	//! @name Primitive restart
	//	@{
	[[deprecated]]
	const PrimitiveRestartParameters& getPrimitiveRestartParameters() const;
	[[deprecated]]
	void popPrimitiveRestart();
	[[deprecated]]
	void pushPrimitiveRestart();
	[[deprecated]]
	void pushAndSetPrimitiveRestart(const PrimitiveRestartParameters& parameters);
	[[deprecated]]
	void setPrimitiveRestart(const PrimitiveRestartParameters& parameters);
	// @}
	
	// ------

	//! @name Rasterization
	//	@{
	const RasterizationState& getRasterization() const;
	void popRasterization();
	void pushRasterization();
	void pushAndSetRasterization(const RasterizationState& state);
	void setRasterization(const RasterizationState& state);
	// @}


	// ------

	//! @name Shader
	//	@{
	void pushAndSetShader(const ShaderRef& shader);
	void pushShader();
	void popShader();
	bool isShaderEnabled(const ShaderRef& shader);
	const ShaderRef& getActiveShader() const;
	void setShader(const ShaderRef& shader);

	//! (internal) called by Shader::setUniform(...)
	void _setUniformOnShader(const ShaderRef& shader, const Uniform& uniform, bool warnIfUnused, bool forced);

	// @}

	// ------

	//! @name Scissor
	//	@{
	[[deprecated]]
	const ScissorParameters& getScissor() const;
	[[deprecated]]
	void popScissor();
	[[deprecated]]
	void pushScissor();
	[[deprecated]]
	void pushAndSetScissor(const ScissorParameters& scissorParameters);
	[[deprecated]]
	void setScissor(const ScissorParameters& scissorParameters);
	// @}

// ------

	//! @name Stencil
	//	@{
	[[deprecated]]
	const StencilParameters& getStencilParamters() const;
	[[deprecated]]
	void popStencil();
	[[deprecated]]
	void pushStencil();
	[[deprecated]]
	void pushAndSetStencil(const StencilParameters& stencilParameter);
	[[deprecated]]
	void setStencil(const StencilParameters& stencilParameter);

	// @}

	// ------

	/*! @name Textures
	 \todo Move array of activeTextures to RenderingStatus to allow delayed binding
	 */
	//	@{
	const TextureRef& getTexture(uint8_t unit, uint8_t set=0) const;
	[[deprecated]]
	TexUnitUsageParameter getTextureUsage(uint8_t unit) const;
	void pushTexture(uint8_t unit, uint8_t set=0);
	void pushAndSetTexture(uint8_t unit, const TextureRef& texture, uint8_t set=0);
	[[deprecated]]
	void pushAndSetTexture(uint8_t unit, const TextureRef& texture, TexUnitUsageParameter usage, uint8_t set=0) {
		pushAndSetTexture(unit, texture, set);
	}
	void popTexture(uint8_t unit, uint8_t set=0);

	//! \note texture may be nullptr
	void setTexture(uint8_t unit, const TextureRef& texture, uint8_t set=0);
	[[deprecated]]
	void setTexture(uint8_t unit, const TextureRef& texture, TexUnitUsageParameter usage, uint8_t set=0) {
		setTexture(unit, texture, set);
	}
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
	const Geometry::Rect_i& getWindowClientArea() const;

	//! Read the current viewport.
	const Geometry::Rect_i& getViewport() const;

	//! Read the current viewport state.
	const ViewportState& getViewportState() const;

	//! Restore the viewport from the top of the viewport stack.
	void popViewport();

	//! Save the current viewport onto the viewport stack.
	void pushViewport();

	//! Set the current viewport.
	void setViewport(const Geometry::Rect_i& viewport);
	void setViewport(const Geometry::Rect_i& viewport, const Geometry::Rect_i& scissor);
	void setViewport(const ViewportState& viewport);

	//! Save the current viewport onto the viewport stack and set the current viewport.
	void pushAndSetViewport(const Geometry::Rect_i& viewport);
	void pushAndSetViewport(const Geometry::Rect_i& viewport, const Geometry::Rect_i& scissor);
	void pushAndSetViewport(const ViewportState& viewport);

	void setWindowClientArea(const Geometry::Rect_i& clientArea);
	// @}
};

}

#endif /* RENDERING_RENDERINCONTEXT_H_ */
