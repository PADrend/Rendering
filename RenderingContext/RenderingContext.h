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

#include "../Mesh/VertexDescription.h"
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
}
namespace Rendering {
class AlphaTestParameters;
class BlendingParameters;
class BufferObject;
class ClipPlaneParameters;
class ColorBufferParameters;
class CullFaceParameters;
class DepthBufferParameters;
class FBO;
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
class Texture;
class Uniform;
class UniformRegistry;
enum class TexUnitUsageParameter : uint8_t;

typedef Util::CountedObjectWrapper<BufferObject> CountedBufferObject;

//! @defgroup context Rendering Context

//! @ingroup context
class RenderingContext {

	//! @name General
	//	@{
private:
	// Use Pimpl idiom
	class InternalData;
	std::unique_ptr<InternalData> internalData;

	bool immediate;
	static bool compabilityMode;

public:

	RENDERINGAPI RenderingContext();
	RENDERINGAPI ~RenderingContext();

	//! has to return true iff normal display of mesh shall be executed
	typedef std::function<void (RenderingContext & rc,Mesh * mesh,uint32_t firstElement,uint32_t elementCount)> DisplayMeshFn;
private:
	DisplayMeshFn displayMeshFn;
public:
	void setDisplayMeshFn(DisplayMeshFn fn){ displayMeshFn = fn; };
	RENDERINGAPI void resetDisplayMeshFn();

	void displayMesh(Mesh * mesh,uint32_t firstElement,uint32_t elementCount){ displayMeshFn(*this, mesh,firstElement,elementCount); }
	RENDERINGAPI void displayMesh(Mesh * mesh);

	RENDERINGAPI void setImmediateMode(const bool enabled);
	bool getImmediateMode() const {
		return immediate;
	}
	static bool getCompabilityMode() {
		return compabilityMode;
	}

	RENDERINGAPI void applyChanges(bool forced = false);
	//	@}

	// -----------------------------------

	/*!	@name GL Helper */
	//	@{
	RENDERINGAPI static void clearScreen(const Util::Color4f & color);
	RENDERINGAPI static void initGLState();
	RENDERINGAPI void clearScreenRect(const Geometry::Rect_i & rect, const Util::Color4f & color, bool clearDepth=true);

	/*! On AMD/ATI-cards, if a Shader accesses a non-existing vertex attribute (even in a branch that should not be
		executed), it seems that it accesses the data of the default GL_VERTEX_ARRAY attribute. If this is not set, the
		fragment's calculation fails and the object gets invisible. This workaround checks if an ATI/AMD card is used and
		in the MeshVertexData::bind()-function, the GL_VERTEX_ARRAY client state is enabled even if it is not required by
		the used Shader.
		\note The workaround is enabled if the GL_RENDERER-string contains 'AMD' or 'ATI'
	*/
	RENDERINGAPI static bool useAMDAttrBugWorkaround();

	/**
	 * Flush the GL commands buffer.
	 * @see glFlush
	 */
	RENDERINGAPI static void flush();

	/**
	 * Block until all GL commands are complete.
	 * @see glFinish
	 */
	RENDERINGAPI static void finish();
	
	/**
	 * Defines a barrier ordering memory transactions.
	 * @see glMemoryBarrier
	 */
	RENDERINGAPI void barrier(uint32_t flags=0);
	
	// @}

	// --------------------------------------------------------------------
	// --------------------------------------------------------------------
	// Parameters (sorted alphabetically)

	//! @name AlphaTest
	//	@{
	RENDERINGAPI const AlphaTestParameters & getAlphaTestParameters() const;
	RENDERINGAPI void popAlphaTest();
	RENDERINGAPI void pushAlphaTest();
	RENDERINGAPI void pushAndSetAlphaTest(const AlphaTestParameters & alphaTestParameter);
	RENDERINGAPI void setAlphaTest(const AlphaTestParameters & alphaTestParameter);
	// @}
	
	// ------

	//! @name Atomic counters (extension ARB_shader_atomic_counters)
	//	@{
	RENDERINGAPI static bool isAtomicCountersSupported();
	RENDERINGAPI static uint32_t getMaxAtomicCounterBuffers();
	RENDERINGAPI static uint32_t getMaxAtomicCounterBufferSize();
	RENDERINGAPI Texture* getAtomicCounterTextureBuffer(uint32_t index)const;
	RENDERINGAPI void pushAtomicCounterTextureBuffer(uint32_t index);
	RENDERINGAPI void pushAndSetAtomicCounterTextureBuffer(uint32_t index, Texture* bufferDataTexture); 
	RENDERINGAPI void popAtomicCounterTextureBuffer(uint32_t pushAtomicCounterTexture);

	//! \note the texture may be null to unbind
	RENDERINGAPI void setAtomicCounterTextureBuffer(uint32_t pushAtomicCounterTexture, Texture* bufferDataTexture);
	// @}
	// ------

	//! @name Blending
	//	@{
	RENDERINGAPI const BlendingParameters & getBlendingParameters() const;
	RENDERINGAPI void popBlending();
	RENDERINGAPI void pushBlending();
	RENDERINGAPI void pushAndSetBlending(const BlendingParameters & blendingParameter);
	RENDERINGAPI void setBlending(const BlendingParameters & blendingParameter);
	// @}
	
	// ------

	//! @name Scissor
	//	@{
	RENDERINGAPI const ClipPlaneParameters & getClipPlane(uint8_t index) const;
	RENDERINGAPI void popClipPlane(uint8_t index);
	RENDERINGAPI void pushClipPlane(uint8_t index);
	RENDERINGAPI void pushAndSetClipPlane(uint8_t index, const ClipPlaneParameters & planeParameters);
	RENDERINGAPI void setClipPlane(uint8_t index, const ClipPlaneParameters & planeParameters);
	// @}
	
	// ------

	//! @name ColorBuffer
	//	@{
	RENDERINGAPI const ColorBufferParameters & getColorBufferParameters() const;
	RENDERINGAPI void popColorBuffer();
	RENDERINGAPI void pushColorBuffer();
	RENDERINGAPI void pushAndSetColorBuffer(const ColorBufferParameters & colorBufferParameter);
	RENDERINGAPI void setColorBuffer(const ColorBufferParameters & colorBufferParameter);

	/**
	 * Clear the color buffer.
	 *
	 * @param clearValue Color that the color buffer is filled with.
	 * The color components are clamped to [0, 1].
	 * @see glClearColor
	 * @see Parameter GL_COLOR_BUFFER_BIT of glClear
	 */
	RENDERINGAPI void clearColor(const Util::Color4f & clearValue);
	// @}
	// ------

	//! @name CullFace
	//	@{
	RENDERINGAPI const CullFaceParameters & getCullFaceParameters() const;
	RENDERINGAPI void popCullFace();
	RENDERINGAPI void pushCullFace();
	RENDERINGAPI void pushAndSetCullFace(const CullFaceParameters & cullFaceParameters);
	RENDERINGAPI void setCullFace(const CullFaceParameters & cullFaceParameters);
	// @}

	// ------

	//! @name DepthBuffer
	//	@{
	RENDERINGAPI const DepthBufferParameters & getDepthBufferParameters() const;
	RENDERINGAPI void popDepthBuffer();
	RENDERINGAPI void pushDepthBuffer();
	RENDERINGAPI void pushAndSetDepthBuffer(const DepthBufferParameters & depthBufferParameter);
	RENDERINGAPI void setDepthBuffer(const DepthBufferParameters & depthBufferParameter);

	/**
	 * Clear the depth buffer.
	 *
	 * @param clearValue Value that the depth buffer is filled with
	 * The value is clamped to [0, 1].
	 * @see glClearDepth
	 * @see Parameter GL_DEPTH_BUFFER_BIT of glClear
	 */
	RENDERINGAPI void clearDepth(float clearValue);
	// @}

	// ------

	//! @name FBO
	//	@{
	RENDERINGAPI FBO * getActiveFBO() const;
	RENDERINGAPI void popFBO();
	RENDERINGAPI void pushFBO();
	RENDERINGAPI void pushAndSetFBO(FBO * fbo);
	RENDERINGAPI void setFBO(FBO * fbo);
	// @}

	// ------

	//! @name Global Uniforms
	//	@{
	RENDERINGAPI void setGlobalUniform(const Uniform & u);
	RENDERINGAPI const Uniform & getGlobalUniform(const Util::StringIdentifier & uniformName);
	// @}

	// ------

	//! @name Image Binding (Image load and store)
	//	@{
	RENDERINGAPI static bool isImageBindingSupported();
	RENDERINGAPI ImageBindParameters getBoundImage(uint8_t unit)const;
	RENDERINGAPI void pushBoundImage(uint8_t unit);
	RENDERINGAPI void pushAndSetBoundImage(uint8_t unit, const ImageBindParameters& iParam); 
	RENDERINGAPI void popBoundImage(uint8_t unit);

	//! \note the texture in iParam may be null to unbind
	RENDERINGAPI void setBoundImage(uint8_t unit, const ImageBindParameters& iParam);
	// @}

	// ------

	//! @name Lighting
	//	@{
	RENDERINGAPI const LightingParameters & getLightingParameters() const;
	RENDERINGAPI void popLighting();
	RENDERINGAPI void pushLighting();
	RENDERINGAPI void pushAndSetLighting(const LightingParameters & lightingParameter);
	RENDERINGAPI void setLighting(const LightingParameters & lightingParameter);

	// ------

	//! @name Lights
	//	@{
	/**
	 * Activate the light given by the parameters.
	 *
	 * @param light Parameters of a light source.
	 * @return Light number that was used for this light. This number has to be used to deactivate the light.
	 */
	RENDERINGAPI uint8_t enableLight(const LightParameters & light);

	/**
	 * Deactivate a previuosly activated light.
	 *
	 * @param lightNumber Light number that was returned by @a enableLight.
	 */
	RENDERINGAPI void disableLight(uint8_t lightNumber);
	// @}

	// ------

	//! @name Line
	//	@{
	RENDERINGAPI const LineParameters & getLineParameters() const;
	RENDERINGAPI void popLine();
	RENDERINGAPI void pushLine();
	RENDERINGAPI void pushAndSetLine(const LineParameters & lineParameters);
	RENDERINGAPI void setLine(const LineParameters & lineParameters);
	// @}

	// ------

	//! @name Material
	//	@{
	//! Return the active material.
	RENDERINGAPI const MaterialParameters & getMaterial() const;
	//! Pop a material from the top of the stack and activate it. Deactivate material usage if stack is empty.
	RENDERINGAPI void popMaterial();
	//! Push the given material onto the material stack.
	RENDERINGAPI void pushMaterial();
	//! Push the given material onto the material stack and activate it.
	RENDERINGAPI void pushAndSetMaterial(const MaterialParameters & material);
	//! Convert the given color to a material, and call @a pushAndSetMaterial
	RENDERINGAPI void pushAndSetColorMaterial(const Util::Color4f & color);
	//! Activate the given material.
	RENDERINGAPI void setMaterial(const MaterialParameters & material);


	// @}
	// ------

	/*! @name Matrix CameraToWorld / WorldToCamera
	 camera matrix == inverse world matrix of camera node == default model view matrix	*/
	//	@{
	RENDERINGAPI void setMatrix_cameraToWorld(const Geometry::Matrix4x4 & matrix);	//!< formerly known as setInverseCameraMatrix
	RENDERINGAPI const Geometry::Matrix4x4 & getMatrix_worldToCamera() const;		//!< formerly known as getCameraMatrix
	RENDERINGAPI const Geometry::Matrix4x4 & getMatrix_cameraToWorld() const;		//!< formerly known as getInverseCameraMatrix
	//	@}

	// ------

	//! @name Matrix ModelToCamera (Legacy Model View Matrix)
	//	@{
	//! resets the model view matrix to the default (camera matrix)
	RENDERINGAPI void resetMatrix();  //! \note use renderingContext.setMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() ) instead!
	RENDERINGAPI const Geometry::Matrix4x4 & getMatrix_modelToCamera() const;		//!< formerly known as getMatrix
	RENDERINGAPI void multMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix);	//!< formerly known as multMatrix
	RENDERINGAPI void pushMatrix_modelToCamera();									//!< formerly known as pushMatrix
	RENDERINGAPI void pushAndSetMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix);
	RENDERINGAPI void setMatrix_modelToCamera(const Geometry::Matrix4x4 & matrix);	//!< formerly known as setMatrix
	RENDERINGAPI void popMatrix_modelToCamera();										//!< formerly known as popMatrix
	//	@}
	
	// ------

	//! @name Matrix CameraToClipping (Legacy Projection Matrix)
	//	@{
	RENDERINGAPI const Geometry::Matrix4x4 & getMatrix_cameraToClipping() const;			//! formerly known as getProjectionMatrix
	RENDERINGAPI void pushAndSetMatrix_cameraToClipping(const Geometry::Matrix4x4 & matrix);
	RENDERINGAPI void pushMatrix_cameraToClipping();										//! formerly known as pushProjectionMatrix
	RENDERINGAPI void popMatrix_cameraToClipping();										//! formerly known as popProjectionMatrix
	RENDERINGAPI void setMatrix_cameraToClipping(const Geometry::Matrix4x4 & matrix);	//! formerly known as setProjectionMatrix
	// @}
	
	// ------

	//! @name Point
	//	@{
	RENDERINGAPI const PointParameters & getPointParameters() const;
	RENDERINGAPI void popPointParameters();
	RENDERINGAPI void pushPointParameters();
	RENDERINGAPI void pushAndSetPointParameters(const PointParameters & pointParameters);
	RENDERINGAPI void setPointParameters(const PointParameters & pointParameters);
	// @}
	// ------

	//! @name PolygonMode
	//	@{
	RENDERINGAPI const PolygonModeParameters & getPolygonModeParameters() const;
	RENDERINGAPI void popPolygonMode();
	RENDERINGAPI void pushPolygonMode();
	RENDERINGAPI void pushAndSetPolygonMode(const PolygonModeParameters & polygonModeParameter);
	RENDERINGAPI void setPolygonMode(const PolygonModeParameters & polygonModeParameter);
	// @}

	// ------

	//! @name PolygonOffset
	//	@{
	RENDERINGAPI const PolygonOffsetParameters & getPolygonOffsetParameters() const;
	RENDERINGAPI void popPolygonOffset();
	RENDERINGAPI void pushPolygonOffset();
	RENDERINGAPI void pushAndSetPolygonOffset(const PolygonOffsetParameters & polygonOffsetParameter);
	RENDERINGAPI void setPolygonOffset(const PolygonOffsetParameters & polygonOffsetParameter);
	// @}
	
	// ------

	//! @name Primitive restart
	//	@{
	RENDERINGAPI const PrimitiveRestartParameters & getPrimitiveRestartParameters() const;
	RENDERINGAPI void popPrimitiveRestart();
	RENDERINGAPI void pushPrimitiveRestart();
	RENDERINGAPI void pushAndSetPrimitiveRestart(const PrimitiveRestartParameters & parameters);
	RENDERINGAPI void setPrimitiveRestart(const PrimitiveRestartParameters & parameters);
	// @}


	// ------

	//! @name Shader
	//	@{
	RENDERINGAPI void pushAndSetShader(Shader * shader);
	RENDERINGAPI void pushShader();
	RENDERINGAPI void popShader();
	RENDERINGAPI bool isShaderEnabled(Shader * shader);
	RENDERINGAPI Shader * getActiveShader();
	RENDERINGAPI const Shader * getActiveShader() const;
	RENDERINGAPI void setShader(Shader * shader); // shader may be nullptr
	RENDERINGAPI void dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY=1, uint32_t numGroupsZ=1);
	RENDERINGAPI void dispatchComputeIndirect(size_t offset=0);
	RENDERINGAPI void loadUniformSubroutines(uint32_t shaderStage, const std::vector<uint32_t>& indices);
	RENDERINGAPI void loadUniformSubroutines(uint32_t shaderStage, const std::vector<std::string>& names);
	RENDERINGAPI void drawMeshTask(uint32_t count=1, uint32_t first=0);

	//! (internal) called by Shader::setUniform(...)
	RENDERINGAPI void _setUniformOnShader(Shader * shader, const Uniform & uniform, bool warnIfUnused, bool forced);

	// @}

	// ------

	//! @name Scissor
	//	@{
	RENDERINGAPI const ScissorParameters & getScissor() const;
	RENDERINGAPI void popScissor();
	RENDERINGAPI void pushScissor();
	RENDERINGAPI void pushAndSetScissor(const ScissorParameters & scissorParameters);
	RENDERINGAPI void setScissor(const ScissorParameters & scissorParameters);
	// @}

// ------

	//! @name Stencil
	//	@{
	RENDERINGAPI const StencilParameters & getStencilParamters() const;
	RENDERINGAPI void popStencil();
	RENDERINGAPI void pushStencil();
	RENDERINGAPI void pushAndSetStencil(const StencilParameters & stencilParameter);
	RENDERINGAPI void setStencil(const StencilParameters & stencilParameter);

	/**
	 * Clear the stencil buffer.
	 *
	 * @param clearValue Value that the stencil buffer is filled with
	 * @see glClearStencil
	 * @see Parameter GL_STENCIL_BUFFER_BIT of glClear
	 */
	RENDERINGAPI void clearStencil(int32_t clearValue);
	// @}

	// ------

	/*! @name Textures
	 \todo Move array of activeTextures to RenderingStatus to allow delayed binding
	 */
	//	@{
	RENDERINGAPI Texture * getTexture(uint8_t unit)const;
	RENDERINGAPI TexUnitUsageParameter getTextureUsage(uint8_t unit)const;
	RENDERINGAPI void pushTexture(uint8_t unit);
	RENDERINGAPI void pushAndSetTexture(uint8_t unit, Texture * texture); // default usage = TexUnitUsageParameter::TEXTURE_MAPPING );
	RENDERINGAPI void pushAndSetTexture(uint8_t unit, Texture * texture, TexUnitUsageParameter usage);
	RENDERINGAPI void popTexture(uint8_t unit);

	//! \note texture may be nullptr
	RENDERINGAPI void setTexture(uint8_t unit, Texture * texture); // default: usage = TexUnitUsageParameter::TEXTURE_MAPPING);
	RENDERINGAPI void setTexture(uint8_t unit, Texture * texture, TexUnitUsageParameter usage);
	// @}
	
	// ------

	//! @name Transform Feedback
	//	@{
	RENDERINGAPI static bool isTransformFeedbackSupported();
	RENDERINGAPI static bool requestTransformFeedbackSupport(); //! like isTransformFeedbackSupported(), but once issues a warning on failure.
	
	RENDERINGAPI CountedBufferObject * getActiveTransformFeedbackBuffer() const;
	RENDERINGAPI void popTransformFeedbackBufferStatus();
	RENDERINGAPI void pushTransformFeedbackBufferStatus();
	RENDERINGAPI void setTransformFeedbackBuffer(CountedBufferObject * buffer);
	RENDERINGAPI void _startTransformFeedback(uint32_t);
	RENDERINGAPI void startTransformFeedback_lines();
	RENDERINGAPI void startTransformFeedback_points();
	RENDERINGAPI void startTransformFeedback_triangles();
	RENDERINGAPI void stopTransformFeedback();
	// @}

	// ------

	//! @name VBO Client States
	// @{
	//! Activate the given client state.
	RENDERINGAPI void enableClientState(uint32_t clientState);

	//! Deactivate all client states that were activated before.
	RENDERINGAPI void disableAllClientStates();

	//! Activate the texture coordinate client state for the given texture unit.
	RENDERINGAPI void enableTextureClientState(uint32_t textureUnit);

	//! Deactivate the texture coordinate client states for all texture units that were activated before.
	RENDERINGAPI void disableAllTextureClientStates();

	/**
	 * Bind a vertex attribute to a variable inside a shader program.
	 *
	 * @param attr Attribute description (including variable name)
	 * @param data Pointer to the vertex data (or @c nullptr if a buffer object is active)
	 * @param stride Size of a vertex in bytes
	 */
	RENDERINGAPI void enableVertexAttribArray(const VertexAttribute & attr, const uint8_t * data, int32_t stride);

	//! Disable all vertex attribute array.
	RENDERINGAPI void disableAllVertexAttribArrays();
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
	RENDERINGAPI const Geometry::Rect_i & getWindowClientArea() const;

	//! Read the current viewport.
	RENDERINGAPI const Geometry::Rect_i & getViewport() const;
	//! Restore the viewport from the top of the viewport stack.
	RENDERINGAPI void popViewport();

	//! Save the current viewport onto the viewport stack.
	RENDERINGAPI void pushViewport();

	//! Set the current viewport.
	RENDERINGAPI void setViewport(const Geometry::Rect_i & viewport);

	//! Save the current viewport onto the viewport stack and set the current viewport.
	RENDERINGAPI void pushAndSetViewport(const Geometry::Rect_i & viewport);

	RENDERINGAPI void setWindowClientArea(const Geometry::Rect_i & clientArea);
	// @}
};

}

#endif /* RENDERING_RENDERINCONTEXT_H_ */
