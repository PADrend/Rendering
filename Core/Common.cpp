/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Common.h"
#include <Util/Resources/AttributeFormat.h>

namespace Rendering {

//-----------------

InternalFormat toInternalFormat(const Util::AttributeFormat& attr) {
	switch(attr.getDataType()) {
		case Util::TypeConstant::UINT8:
			switch(attr.getComponentCount()) {
				case 1: return attr.isNormalized() ? InternalFormat::R8Unorm : InternalFormat::R8Uint;
				case 2: return attr.isNormalized() ? InternalFormat::RG8Unorm : InternalFormat::RG8Uint;
				case 4: return attr.isNormalized() ? InternalFormat::RGBA8Unorm : InternalFormat::RGBA8Uint;
				default: break;
			}
			break;
		case Util::TypeConstant::UINT16:
			switch(attr.getComponentCount()) {
				case 1: return attr.isNormalized() ? InternalFormat::R16Unorm : InternalFormat::R16Uint;
				case 2: return attr.isNormalized() ? InternalFormat::RG16Unorm : InternalFormat::RG16Uint;
				case 3: return attr.isNormalized() ? InternalFormat::RGB16Unorm : InternalFormat::RGB16Uint;
				case 4: return attr.isNormalized() ? InternalFormat::RGBA16Unorm : InternalFormat::RGBA16Uint;
				default: break;
			}
			break;
		case Util::TypeConstant::UINT32:
			switch(attr.getComponentCount()) {
				case 1: return InternalFormat::R32Uint;
				case 2: return InternalFormat::RG32Uint;
				case 3: return InternalFormat::RGB32Uint;
				case 4: return InternalFormat::RGBA32Uint;
				default: break;
			}
			break;
		case Util::TypeConstant::INT8:
			switch(attr.getComponentCount()) {
				case 1: return attr.isNormalized() ? InternalFormat::R8Snorm : InternalFormat::R8Int;
				case 2: return attr.isNormalized() ? InternalFormat::RG8Snorm : InternalFormat::RG8Int;
				case 4: return attr.isNormalized() ? InternalFormat::RGBA8Snorm : InternalFormat::RGBA8Int;
				default: break;
			}
			break;
		case Util::TypeConstant::INT16:
			switch(attr.getComponentCount()) {
				case 1: return attr.isNormalized() ? InternalFormat::R16Snorm : InternalFormat::R16Int;
				case 2: return attr.isNormalized() ? InternalFormat::RG16Snorm : InternalFormat::RG16Int;
				case 3: return attr.isNormalized() ? InternalFormat::RGB16Snorm : InternalFormat::RGB16Int;
				case 4: return InternalFormat::RGBA16Int;
				default: break;
			}
			break;
		case Util::TypeConstant::INT32:
			switch(attr.getComponentCount()) {
				case 1: return InternalFormat::R32Int;
				case 2: return InternalFormat::RG32Int;
				case 3: return InternalFormat::RGB32Int;
				case 4: return InternalFormat::RGBA32Int;
				default: break;
			}
			break;
		case Util::TypeConstant::FLOAT:
			switch(attr.getComponentCount()) {
				case 1: return attr.isNormalized() ? InternalFormat::R32Float : InternalFormat::R32Float;
				case 2: return attr.isNormalized() ? InternalFormat::RG32Float : InternalFormat::RG32Float;
				case 3: return attr.isNormalized() ? InternalFormat::RGB32Float : InternalFormat::RGB32Float;
				case 4: return attr.isNormalized() ? InternalFormat::RGBA32Float : InternalFormat::RGBA32Float;
				default: break;
			}
			break;
		case Util::TypeConstant::HALF:
			switch(attr.getComponentCount()) {
				case 1: return InternalFormat::R16Float;
				case 2: return InternalFormat::RG16Float;
				case 3: return InternalFormat::RGB16Float;
				case 4: return InternalFormat::RGBA16Float;
				default: break;
			}
			break;
		default: break;
	}
	return InternalFormat::Unknown;
}

//-----------------

bool isDepthStencilFormat(InternalFormat format) {
	switch (format) {
		case InternalFormat::D32Float:
		case InternalFormat::D16Unorm:
		case InternalFormat::D32FloatS8X24:
		case InternalFormat::D24UnormS8:
			return true;
		default: return false;
	}
};

//-----------------

} /* Rendering */