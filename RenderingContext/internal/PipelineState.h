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

#include "../RenderingParameters.h"
#include "../../Texture/Texture.h"
#include "../../FBO.h"
#include "../../Shader/Shader.h"
#include "../../Mesh/VertexAttribute.h"
#include "../../BufferObject.h"
#include <Util/References.h>
#include <Geometry/Rect.h>
#include <cstdint>

namespace Rendering {

//! (internal) Used by the renderingContext to track changes made to the shader independent core-state of OpenGL.
class PipelineState {
	// -------------------------------
	private:
		bool valid = false;
		bool debug = false;
	public:
		void invalidate() { valid = false; }
		bool isValid() const { return valid; }
		void setDebug(bool v) { debug = v; }
		void apply(const PipelineState& other);

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
		void updateShader(Util::Reference<Shader> s, uint32_t prog) {
			program = prog;
			shader = s;
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
		void updateVertexFormat(const PipelineState & other) {
			vertexFormat = other.vertexFormat;
			vertexFormatCheckNumber = other.vertexFormatCheckNumber;
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
		void updateVertexBinding(const PipelineState & other) {
			vertexBindings = other.vertexBindings;
			vertexBindingCheckNumber = other.vertexBindingCheckNumber;
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
		void updateBlendingParameters(const BlendingParameters & p,uint32_t _checkNumber) {
			blendingParameters = p;
			blendingCheckNumber = _checkNumber;
		}
		void updateBlendingParameters(const PipelineState & other) {
			blendingParameters = other.blendingParameters;
			blendingCheckNumber = other.blendingCheckNumber;
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

	//!	@name Lighting
	//	@{
	public:
		bool lightingParametersChanged(const PipelineState & actual) const __attribute((deprecated)) {
			return false;
		}
		const LightingParameters & getLightingParameters() const __attribute((deprecated)) {
			static LightingParameters lightingParameters;
			return lightingParameters;
		}
		void setLightingParameters(const LightingParameters & p) __attribute((deprecated)) {
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
		void updateStencilParameters(const StencilParameters & p, uint32_t _checkNumber) {
			stencilParameters = p;
			stencilCheckNumber = _checkNumber;
		}
		void updateStencilParameters(const PipelineState & other) {
			stencilParameters = other.stencilParameters;
			stencilCheckNumber = other.stencilCheckNumber;
		}
	//	@}

	//!	@name Textures
	//	@{
	private:
		uint32_t texturesCheckNumber = 0;
		std::array<Util::Reference<Texture>, MAX_TEXTURES> boundTextures;

	public:
		void setTexture(uint8_t unit, Util::Reference<Texture> texture) {
			++texturesCheckNumber;
			boundTextures.at(unit) = std::move(texture);
		}
		const Util::Reference<Texture> & getTexture(uint8_t unit) const {
			return boundTextures.at(unit);
		}
		bool texturesChanged(const PipelineState & actual) const {
			return (texturesCheckNumber == actual.texturesCheckNumber) ? false : (boundTextures != actual.boundTextures);
		}
		void updateTextures(const PipelineState & actual) {
			boundTextures = actual.boundTextures;
			texturesCheckNumber = actual.texturesCheckNumber;
		}
	//	@}
};

}

#endif /* CORE_RENDERING_DATA_H_ */
