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
#ifndef RENDERING_DATA_H_
#define RENDERING_DATA_H_

#include "RenderingParameters.h"
#include "../Texture/TextureType.h"
#include "../BufferView.h"
#include <Geometry/Matrix4x4.h>
#include <bitset>
#include <cassert>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstring>

namespace Rendering {
class UniformRegistry;

//! (internal) Used by shaders and the renderingContext to track the state of shader (and openGL) dependent properties.
class ProgramState {
public:
	void apply(UniformRegistry* uniformRegistry, const ProgramState & target, bool forced);
	
	// -------------------------------

	//!	@name Texture Units
	//	@{
	private:
		uint32_t textureUnitUsagesCheckNumber = 0;
		typedef std::vector<std::pair<TexUnitUsageParameter,TextureType>> TexUnitTypeVec_t;
		TexUnitTypeVec_t textureUnitParams = TexUnitTypeVec_t(MAX_TEXTURES,{TexUnitUsageParameter::DISABLED,TextureType::TEXTURE_2D});

	public:
		void setTextureUnitParams(uint8_t unit, TexUnitUsageParameter use, TextureType t ) {
			++textureUnitUsagesCheckNumber;
			textureUnitParams.at(unit) = std::make_pair(use,t);
		}
		const std::pair<TexUnitUsageParameter,TextureType> & getTextureUnitParams(uint8_t unit) const {
			return textureUnitParams.at(unit);
		}
		bool textureUnitsChanged(const ProgramState & actual) const {
			return (textureUnitUsagesCheckNumber == actual.textureUnitUsagesCheckNumber) ? false : 
					textureUnitParams != actual.textureUnitParams;
		}
		void updateTextureUnits(const ProgramState & actual) {
			textureUnitParams = actual.textureUnitParams;
			textureUnitUsagesCheckNumber = actual.textureUnitUsagesCheckNumber;
		}

};

}

#endif /* RENDERING_DATA_H_ */
