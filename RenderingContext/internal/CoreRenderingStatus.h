/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CORE_RENDERING_DATA_H_
#define CORE_RENDERING_DATA_H_

#include "../RenderingParameters.h"
#include "../../Texture/Texture.h"
#include <Util/References.h>
#include <cstdint>

namespace Rendering {

//! (internal) Used by the renderingContext to track changes made to the shader independent core-state of OpenGL.
class CoreRenderingStatus {
	//!	@name Construction
	//	@{
	public:
		CoreRenderingStatus() :
			blendingCheckNumber(0),
			blendingParameters(),
			colorBufferParameters(), 
			cullFaceParameters(),
			depthBufferParameters(),
			alphaTestParameters(),
			lineParameters(),
			lightingParameters(),
			polygonModeParameters(),
			polygonOffsetParameters(),
			stencilCheckNumber(),
			stencilParameters(),
			texturesCheckNumber(),
			boundTextures() {
		}
	//	@}

	// -------------------------------

	//!	@name Blending
	//	@{
	private:
		uint32_t blendingCheckNumber;
		BlendingParameters blendingParameters;

	public:
		bool blendingParametersChanged(const CoreRenderingStatus & actual) const {
			return (blendingCheckNumber == actual.blendingCheckNumber) ? false :
					(blendingParameters != actual.blendingParameters);
		}
		const BlendingParameters & getBlendingParameters() const {
			return blendingParameters;
		}
		void setBlendingParameters(const BlendingParameters & p) {
			blendingParameters = p;
			++blendingCheckNumber;
		}
		void updateBlendingParameters(const BlendingParameters & p,uint32_t _checkNumber) {
			blendingParameters = p;
			blendingCheckNumber = _checkNumber;
		}
		void updateBlendingParameters(const CoreRenderingStatus & other) {
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
		bool colorBufferParametersChanged(const CoreRenderingStatus & actual) const {
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
		bool cullFaceParametersChanged(const CoreRenderingStatus & actual) const {
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
		bool depthBufferParametersChanged(const CoreRenderingStatus & actual) const {
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

	//!	@name AlphaTest
	//	@{
	private:
		AlphaTestParameters alphaTestParameters;
	public:
		bool alphaTestParametersChanged(const CoreRenderingStatus & actual) const {
			return alphaTestParameters!=actual.alphaTestParameters;
		}
		const AlphaTestParameters & getAlphaTestParameters()const {
			return alphaTestParameters;
		}
		void setAlphaTestParameters(const AlphaTestParameters & p){
			alphaTestParameters=p;
		}

	//	@}

	// ------

	//!	@name Line
	//	@{
	private:
		LineParameters lineParameters;
	public:
		bool lineParametersChanged(const CoreRenderingStatus & actual) const {
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

	//!	@name Lighting
	//	@{
	private:
		LightingParameters lightingParameters;
	public:
		bool lightingParametersChanged(const CoreRenderingStatus & actual) const {
			return lightingParameters != actual.lightingParameters;
		}
		const LightingParameters & getLightingParameters() const {
			return lightingParameters;
		}
		void setLightingParameters(const LightingParameters & p) {
			lightingParameters = p;
		}
	//	@}

	// ------

	//!	@name PolygonMode
	//	@{
	private:
		PolygonModeParameters polygonModeParameters;
	public:
		bool polygonModeParametersChanged(const CoreRenderingStatus & actual) const {
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
		bool polygonOffsetParametersChanged(const CoreRenderingStatus & actual) const {
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
		uint32_t stencilCheckNumber;
		StencilParameters stencilParameters;

	public:
		bool stencilParametersChanged(const CoreRenderingStatus & actual) const {
			return (stencilCheckNumber == actual.stencilCheckNumber) ? false : (stencilParameters != actual.stencilParameters);
		}
		const StencilParameters & getStencilParameters() const {
			return stencilParameters;
		}
		void setStencilParameters(const StencilParameters & p) {
			stencilParameters = p;
			++stencilCheckNumber;
		}
		void updateStencilParameters(const StencilParameters & p, uint32_t _checkNumber) {
			stencilParameters = p;
			stencilCheckNumber = _checkNumber;
		}
		void updateStencilParameters(const CoreRenderingStatus & other) {
			stencilParameters = other.stencilParameters;
			stencilCheckNumber = other.stencilCheckNumber;
		}
	//	@}

	//!	@name Textures
	//	@{
	private:
		uint32_t texturesCheckNumber;
		std::array<Util::Reference<Texture>, MAX_TEXTURES> boundTextures;

	public:
		void setTexture(uint8_t unit, Util::Reference<Texture> texture) {
			++texturesCheckNumber;
			boundTextures.at(unit) = std::move(texture);
		}
		const Util::Reference<Texture> & getTexture(uint8_t unit) const {
			return boundTextures.at(unit);
		}
		bool texturesChanged(const CoreRenderingStatus & actual) const {
			return (texturesCheckNumber == actual.texturesCheckNumber) ? false : (boundTextures != actual.boundTextures);
		}
		void updateTextures(const CoreRenderingStatus & actual) {
			boundTextures = actual.boundTextures;
			texturesCheckNumber = actual.texturesCheckNumber;
		}
	//	@}
};

}

#endif /* CORE_RENDERING_DATA_H_ */
