/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CORE_RENDERING_DATA_H_
#define CORE_RENDERING_DATA_H_

#include "RenderingParameters.h"
#include "../Texture/Texture.h"
#include "../FBO.h"
#include "../VAO.h"
#include "../Shader/Shader.h"
#include "../Mesh/VertexAttribute.h"
#include <Util/References.h>
#include <Geometry/Rect.h>
#include <cstdint>
#include <bitset>

namespace Rendering {

//! (internal) Used by the renderingContext to track changes made to the shader independent core-state of OpenGL.
class PipelineState {
// -------------------------------
public:
	enum ChangedBits : uint8_t {
		VIEWPORT_BIT,
		SCISSOR_BIT,
		FBO_BIT,
		PROGRAM_BIT,
		VAO_BIT,
		VERTEX_FORMAT_BIT,
		VERTEX_BINDING_BIT,
		ELEMENT_BINDING_BIT,
		BLEND_BIT,
		BLEND_ENABLED_BIT,
		BLEND_FUNC_BIT,
		BLEND_COLOR_BIT,
		BLEND_EQUATION_BIT,
		COLOR_BUFFER_BIT,
		CULL_FACE_BIT,
		DEPTH_BUFFER_BIT,
		LINE_PARAM_BIT,
		POLYGON_MODE_BIT,
		POLYGON_OFFSET_BIT,
		STENCIL_BIT,
		STENCIL_ENABLED_BIT,
		STENCIL_FUNC_BIT,
		STENCIL_OP_BIT,
		TEXTURE_BINDING_BIT,
	};
	struct StateDiff_t {
		std::bitset<TEXTURE_BINDING_BIT+1> state;
		std::bitset<16> format;
		std::bitset<16> vertexBinding;
	};
	StateDiff_t makeDiff(const PipelineState& other, bool forced=false);
	void apply(const StateDiff_t& diff);

//!	@name Viewport
//	@{
private:
	Geometry::Rect_i viewport;
public:
	bool viewportChanged(const PipelineState & actual) const {
		return viewport != actual.viewport;
	}
	void setViewport(const Geometry::Rect_i& vp) {
		viewport = vp;
	}
	const Geometry::Rect_i& getViewport() const {
		return viewport;
	}
//	@}

// ------

//!	@name Scissor
//	@{
private:
	ScissorParameters scissor;
public:
	bool scissorParametersChanged(const PipelineState & actual) const {
		return scissor != actual.scissor;
	}
	void setScissorParameters(const ScissorParameters& p) {
		scissor = p;
	}
	const ScissorParameters& getScissorParameters() const {
		return scissor;
	}
//	@}

// ------

//!	@name FBO
//	@{
private:
	Util::Reference<FBO> fbo;
	uint32_t activeFBO = 0;
public:
	bool fboChanged(const PipelineState & actual) const {
		return activeFBO != actual.activeFBO;
	}
	void setFBO(Util::Reference<FBO> p) {
		fbo = p;
		activeFBO = 0;
		if(fbo.isNotNull()) {
			fbo->prepare();
			activeFBO = fbo->getHandle();
		}
	}
	const Util::Reference<FBO>& getFBO() const {
		return fbo;
	}
//	@}

//!	@name Shader
//	@{
private:
	Util::Reference<Shader> shader;
	uint32_t program = 0;
public:
	bool shaderChanged(const PipelineState & actual) const {
		return program != actual.program;
	}
	void setShader(Util::Reference<Shader> s) {
		if(s.isNull()) {
			shader = s;
			program = 0;
		} else if(s->init()) {
			shader = s;
			program = shader->getShaderProg();
		}
	}
	const Util::Reference<Shader>& getShader() const {
		return shader;
	}
	bool isShaderValid() const {
		return program > 0 && shader.isNotNull() && shader->getShaderProg() == program;
	}
//	@}

// ------

//!	@name Vertex format & binding
//	@{
public:
  static const uint32_t MAX_VERTEXBINDINGS = 16;
  static const uint32_t MAX_VERTEXATTRIBS = 16;
private:
	uint32_t vertexFormatCheckNumber = 0;
	std::array<std::pair<VertexAttribute, uint32_t>, MAX_VERTEXATTRIBS> vertexFormat;
	
	uint32_t vertexBindingCheckNumber = 0;
	std::array<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>, MAX_VERTEXBINDINGS> vertexBindings;
	uint32_t elementBinding = 0;
	
	Util::Reference<VAO> vao = nullptr;
public:
	bool vertexFormatChanged(const PipelineState & actual) const {
		return (vertexFormatCheckNumber == actual.vertexFormatCheckNumber) ? false :
				(vertexFormat != actual.vertexFormat);
	}
	void setVertexFormat(uint32_t location, const VertexAttribute& attr, uint32_t binding=0) {
		std::pair<VertexAttribute, uint32_t> p = {attr, binding};
		if(vertexFormat.at(location) != p)
			++vertexFormatCheckNumber;
		vertexFormat.at(location) = std::move(p);
	}
	void resetVertexFormats(uint32_t binding) {
		for(uint_fast8_t i=0; i<MAX_VERTEXATTRIBS; ++i) {
			if(vertexFormat.at(i).second == binding)
				setVertexFormat(i, {}, binding);
		}
	}
	const std::pair<VertexAttribute, uint32_t>& getVertexFormat(uint32_t location) const {
		return vertexFormat.at(location);
	}
	
	// vbo binding
	bool vertexBindingChanged(const PipelineState & actual) const {
		return (vertexBindingCheckNumber == actual.vertexBindingCheckNumber) ? false :
				(vertexBindings != actual.vertexBindings);
	}
	void setVertexBinding(uint32_t binding, uint32_t bufferId, uint32_t offset, uint32_t stride, uint32_t divisor) {
		++vertexBindingCheckNumber;
		vertexBindings.at(binding) = {bufferId, offset, stride, divisor};
	}
	std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> getVertexBinding(uint32_t binding) const {
		return vertexBindings.at(binding);
	}
		
	// element binding
	bool elementBindingChanged(const PipelineState & actual) const {
		return elementBinding != actual.elementBinding;
	}
	void setElementBinding(uint32_t bufferId) {
		elementBinding = bufferId;
	}
	uint32_t getElementBinding() const {
		return elementBinding;
	}
		
	// vao binding
	bool vertexArrayChanged(const PipelineState & actual) const {
		return vao != actual.vao;
	}
	void setVertexArray(VAO* _vao) {
		vao = _vao;
	}
	Util::Reference<VAO> getVertexArray() const {
		return vao;
	}
	
// ------

//!	@name Blending
//	@{
private:
	uint32_t blendingCheckNumber = 0;
	BlendingParameters blendingParameters;

public:
	bool blendingParametersChanged(const PipelineState & actual) const {
		return (blendingCheckNumber == actual.blendingCheckNumber) ? false :
				(blendingParameters != actual.blendingParameters);
	}
	const BlendingParameters & getBlendingParameters() const {
		return blendingParameters;
	}
	void setBlendingParameters(const BlendingParameters & p) {
		++blendingCheckNumber;
		blendingParameters = p;
	}

//	@}

// ------

//!	@name ColorBuffer
//	@{
private:
	ColorBufferParameters colorBufferParameters;
public:
	bool colorBufferParametersChanged(const PipelineState & actual) const {
		return colorBufferParameters != actual.colorBufferParameters;
	}
	const ColorBufferParameters & getColorBufferParameters() const {
		return colorBufferParameters;
	}
	void setColorBufferParameters(const ColorBufferParameters & p) {
		colorBufferParameters = p;
	}
//	@}

// ------

//!	@name CullFace
//	@{
private:
	CullFaceParameters cullFaceParameters;
public:
	bool cullFaceParametersChanged(const PipelineState & actual) const {
		return cullFaceParameters!=actual.cullFaceParameters;
	}
	const CullFaceParameters & getCullFaceParameters()const {
		return cullFaceParameters;
	}
	void setCullFaceParameters(const CullFaceParameters & p){
		cullFaceParameters=p;
	}

//	@}

// ------

//!	@name DepthBuffer
//	@{
private:
	DepthBufferParameters depthBufferParameters;
public:
	bool depthBufferParametersChanged(const PipelineState & actual) const {
		return depthBufferParameters != actual.depthBufferParameters;
	}
	const DepthBufferParameters & getDepthBufferParameters() const {
		return depthBufferParameters;
	}
	void setDepthBufferParameters(const DepthBufferParameters & p) {
		depthBufferParameters = p;
	}
//	@}

// ------

//!	@name Line
//	@{
private:
	LineParameters lineParameters;
public:
	bool lineParametersChanged(const PipelineState & actual) const {
		return lineParameters != actual.lineParameters;
	}
	const LineParameters & getLineParameters() const {
		return lineParameters;
	}
	void setLineParameters(const LineParameters & p) {
		lineParameters = p;
	}
//	@}

// ------

//!	@name PolygonMode
//	@{
private:
	PolygonModeParameters polygonModeParameters;
public:
	bool polygonModeParametersChanged(const PipelineState & actual) const {
		return polygonModeParameters!=actual.polygonModeParameters;
	}
	const PolygonModeParameters & getPolygonModeParameters()const {
		return polygonModeParameters;
	}
	void setPolygonModeParameters(const PolygonModeParameters & p){
		polygonModeParameters=p;
	}

//	@}

// ------

//!	@name PolygonOffset
//	@{
private:
	PolygonOffsetParameters polygonOffsetParameters;
public:
	bool polygonOffsetParametersChanged(const PipelineState & actual) const {
		return polygonOffsetParameters != actual.polygonOffsetParameters;
	}
	const PolygonOffsetParameters & getPolygonOffsetParameters() const {
		return polygonOffsetParameters;
	}
	void setPolygonOffsetParameters(const PolygonOffsetParameters & p) {
		polygonOffsetParameters = p;
	}
//	@}

// ------

//!	@name Stencil
//	@{
private:
	uint32_t stencilCheckNumber = 0;
	StencilParameters stencilParameters;

public:
	bool stencilParametersChanged(const PipelineState & actual) const {
		return (stencilCheckNumber == actual.stencilCheckNumber) ? false : (stencilParameters != actual.stencilParameters);
	}
	const StencilParameters & getStencilParameters() const {
		return stencilParameters;
	}
	void setStencilParameters(const StencilParameters & p) {
		if(stencilParameters != p)
			++stencilCheckNumber;
		stencilParameters = p;
	}
//	@}

//!	@name Textures
//	@{
private:
	uint32_t texturesCheckNumber = 0;
	std::array<Util::Reference<Texture>, MAX_TEXTURES> boundTextures;
	std::array<uint32_t, MAX_TEXTURES> boundGLTextures;

public:
	void setTexture(uint8_t unit, Util::Reference<Texture> texture) {
		++texturesCheckNumber;
		boundGLTextures.at(unit) = texture.isNotNull() ? texture->getGLId() : 0;
		boundTextures.at(unit) = std::move(texture);
	}
	const Util::Reference<Texture> & getTexture(uint8_t unit) const {
		return boundTextures.at(unit);
	}
	bool texturesChanged(const PipelineState & actual) const {
		return (texturesCheckNumber == actual.texturesCheckNumber) ? false : (boundTextures != actual.boundTextures);
	}
//	@}
};

}

#endif /* CORE_RENDERING_DATA_H_ */
