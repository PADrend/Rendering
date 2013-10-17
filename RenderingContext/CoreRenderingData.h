/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CORE_RENDERING_DATA_H_
#define CORE_RENDERING_DATA_H_

#include "ParameterStructs.h"
#include <cstddef>
#include <vector>
#include <cstdint>

namespace Rendering {

//! (internal) Used by the renderingContext to track changes made to the shader independent core-state of OpenGL.
class CoreRenderingData {

	//!	@name General
	//	@{
	private:
		enum type {
			BLENDING = 0,
			STENCIL = 1
		};
		static const uint8_t TYPE_COUNT = 2;

		std::vector<uint32_t> checkNumbers;

	public:
		CoreRenderingData() :
			checkNumbers(TYPE_COUNT),
			cullFaceParameters(),
			stencilParameters() {
		}
		~CoreRenderingData(){}
	//	@}

	// -------------------------------

	//!	@name Blending
	//	@{
	private:
		BlendingParameters blendingParameters;

	public:
		bool blendingParametersChanged(const CoreRenderingData & actual) const {
			return (checkNumbers[BLENDING] == actual.checkNumbers[BLENDING]) ? false :
					(blendingParameters != actual.blendingParameters);
		}
		const BlendingParameters & getBlendingParameters() const {
			return blendingParameters;
		}
		void setBlendingParameters(const BlendingParameters & p) {
			blendingParameters = p;
			++checkNumbers[BLENDING];
		}
		void updateBlendingParameters(const BlendingParameters & p,uint32_t _checkNumber) {
			blendingParameters = p;
			checkNumbers[BLENDING] = _checkNumber;
		}
		void updateBlendingParameters(const CoreRenderingData & other) {
			blendingParameters = other.blendingParameters;
			checkNumbers[BLENDING] = other.checkNumbers[BLENDING];
		}

	//	@}

	// ------

	//!	@name ColorBuffer
	//	@{
	private:
		ColorBufferParameters colorBufferParameters;
	public:
		bool colorBufferParametersChanged(const CoreRenderingData & actual) const {
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
		bool cullFaceParametersChanged(const CoreRenderingData & actual) const {
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
		bool depthBufferParametersChanged(const CoreRenderingData & actual) const {
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
		bool alphaTestParametersChanged(const CoreRenderingData & actual) const {
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
		bool lineParametersChanged(const CoreRenderingData & actual) const {
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
		bool lightingParametersChanged(const CoreRenderingData & actual) const {
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
		bool polygonModeParametersChanged(const CoreRenderingData & actual) const {
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
		bool polygonOffsetParametersChanged(const CoreRenderingData & actual) const {
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
		StencilParameters stencilParameters;

	public:
		bool stencilParametersChanged(const CoreRenderingData & actual) const {
			return (checkNumbers[STENCIL] == actual.checkNumbers[STENCIL]) ? false : (stencilParameters != actual.stencilParameters);
		}
		const StencilParameters & getStencilParameters() const {
			return stencilParameters;
		}
		void setStencilParameters(const StencilParameters & p) {
			stencilParameters = p;
			++checkNumbers[STENCIL];
		}
		void updateStencilParameters(const StencilParameters & p, uint32_t _checkNumber) {
			stencilParameters = p;
			checkNumbers[STENCIL] = _checkNumber;
		}
		void updateStencilParameters(const CoreRenderingData & other) {
			stencilParameters = other.stencilParameters;
			checkNumbers[STENCIL] = other.checkNumbers[STENCIL];
		}

	//	@}

};

}

#endif /* CORE_RENDERING_DATA_H_ */
