/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Common.h"
#include <Util/Resources/AttributeFormat.h>
#include <Util/Graphics/PixelFormat.h>

#include <sstream>

namespace Rendering {

//-----------------

std::string toString(MemoryUsage usage) {
	switch(usage) {
		case MemoryUsage::Unknown: return "Unknown";
		case MemoryUsage::CpuOnly: return "CpuOnly";
		case MemoryUsage::GpuOnly: return "GpuOnly";
		case MemoryUsage::CpuToGpu: return "CpuToGpu";
		case MemoryUsage::GpuToCpu: return "GpuToCpu";
	}
}

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

Util::AttributeFormat toAttributeFormat(InternalFormat format) {
	switch (format) {
		case InternalFormat::R8Unorm:					return {{"R8Unorm"},				Util::TypeConstant::UINT8, 1, true};
		case InternalFormat::R8Snorm:					return {{"R8Snorm"},				Util::TypeConstant::INT8, 1, true};
		case InternalFormat::R16Unorm:				return {{"R16Unorm"},				Util::TypeConstant::UINT16, 1, true};
		case InternalFormat::R16Snorm:				return {{"R16Snorm"},				Util::TypeConstant::INT16, 1, true};
		case InternalFormat::RG8Unorm:				return {{"RG8Unorm"},				Util::TypeConstant::UINT8, 2, true};
		case InternalFormat::RG8Snorm:				return {{"RG8Snorm"},				Util::TypeConstant::INT8, 2, true};
		case InternalFormat::RG16Unorm:				return {{"RG16Unorm"},			Util::TypeConstant::UINT16, 2, true};
		case InternalFormat::RG16Snorm:				return {{"RG16Snorm"},			Util::TypeConstant::INT16, 2, true};
		case InternalFormat::RGB16Unorm:			return {{"RGB16Unorm"},			Util::TypeConstant::UINT16, 3, true};
		case InternalFormat::RGB16Snorm:			return {{"RGB16Snorm"},			Util::TypeConstant::INT16, 3, true};
		case InternalFormat::RGBA8Unorm:			return {{"RGBA8Unorm"},			Util::TypeConstant::UINT8, 4, true};
		case InternalFormat::RGBA8Snorm:			return {{"RGBA8Snorm"},			Util::TypeConstant::INT8, 4, true};
		case InternalFormat::RGBA16Unorm:			return {{"RGBA16Unorm"},		Util::TypeConstant::UINT16, 4, true};
		case InternalFormat::R16Float:				return {{"R16Float"},				Util::TypeConstant::HALF, 1, false};
		case InternalFormat::RG16Float:				return {{"RG16Float"},			Util::TypeConstant::HALF, 2, false};
		case InternalFormat::RGB16Float:			return {{"RGB16Float"},			Util::TypeConstant::HALF, 3, false};
		case InternalFormat::RGBA16Float:			return {{"RGBA16Float"},		Util::TypeConstant::HALF, 4, false};
		case InternalFormat::R32Float:				return {{"R32Float"},				Util::TypeConstant::FLOAT, 1, false};
		case InternalFormat::RG32Float:				return {{"RG32Float"},			Util::TypeConstant::FLOAT, 2, false};
		case InternalFormat::RGB32Float:			return {{"RGB32Float"},			Util::TypeConstant::FLOAT, 3, false};
		case InternalFormat::RGBA32Float:			return {{"RGBA32Float"},		Util::TypeConstant::FLOAT, 4, false};
		case InternalFormat::R11G11B10Float:	return {{"R11G11B10Float"},	Util::TypeConstant::UINT32, 1, false, Util::PixelFormat::INTERNAL_TYPE_R11G11B10_FLOAT};
		case InternalFormat::R8Int:						return {{"R8Int"},					Util::TypeConstant::INT8, 1, false};
		case InternalFormat::R8Uint:					return {{"R8Uint"},					Util::TypeConstant::UINT8, 1, false};
		case InternalFormat::R16Int:					return {{"R16Int"},					Util::TypeConstant::INT16, 1, false};
		case InternalFormat::R16Uint:					return {{"R16Uint"},				Util::TypeConstant::UINT16, 1, false};
		case InternalFormat::R32Int:					return {{"R32Int"},					Util::TypeConstant::INT32, 1, false};
		case InternalFormat::R32Uint:					return {{"R32Uint"},				Util::TypeConstant::UINT32, 1, false};
		case InternalFormat::RG8Int:					return {{"RG8Int"},					Util::TypeConstant::INT8, 2, false};
		case InternalFormat::RG8Uint:					return {{"RG8Uint"},				Util::TypeConstant::UINT8, 2, false};
		case InternalFormat::RG16Int:					return {{"RG16Int"},				Util::TypeConstant::INT16, 2, false};
		case InternalFormat::RG16Uint:				return {{"RG16Uint"},				Util::TypeConstant::UINT16, 2, false};
		case InternalFormat::RG32Int:					return {{"RG32Int"},				Util::TypeConstant::INT32, 2, false};
		case InternalFormat::RG32Uint:				return {{"RG32Uint"},				Util::TypeConstant::UINT32, 2, false};
		case InternalFormat::RGB16Int:				return {{"RGB16Int"},				Util::TypeConstant::INT16, 3, false};
		case InternalFormat::RGB16Uint:				return {{"RGB16Uint"},			Util::TypeConstant::UINT16, 3, false};
		case InternalFormat::RGB32Int:				return {{"RGB32Int"},				Util::TypeConstant::INT32, 3, false};
		case InternalFormat::RGB32Uint:				return {{"RGB32Uint"},			Util::TypeConstant::UINT32, 3, false};
		case InternalFormat::RGBA8Int:				return {{"RGBA8Int"},				Util::TypeConstant::INT8, 4, false};
		case InternalFormat::RGBA8Uint:				return {{"RGBA8Uint"},			Util::TypeConstant::UINT8, 4, false};
		case InternalFormat::RGBA16Int:				return {{"RGBA16Int"},			Util::TypeConstant::INT16, 4, false};
		case InternalFormat::RGBA16Uint:			return {{"RGBA16Uint"},			Util::TypeConstant::UINT16, 4, false};
		case InternalFormat::RGBA32Int:				return {{"RGBA32Int"},			Util::TypeConstant::INT32, 4, false};
		case InternalFormat::RGBA32Uint:			return {{"RGBA32Uint"},			Util::TypeConstant::UINT32, 4, false};
		case InternalFormat::BGRA8Unorm:			return {{"BGRA8Unorm"},			Util::TypeConstant::UINT8, 4, true, Util::PixelFormat::INTERNAL_TYPE_BGRA};
		default: return {};
	}
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
}

//-----------------

uint8_t getDataSize(InternalFormat format) {
	switch (format) {
		case InternalFormat::R8Unorm: return 8;
		case InternalFormat::R8Snorm: return 8;
		case InternalFormat::R16Unorm: return 16;
		case InternalFormat::R16Snorm: return 16;
		case InternalFormat::RG8Unorm: return 16;
		case InternalFormat::RG8Snorm: return 16;
		case InternalFormat::RG16Unorm: return 32;
		case InternalFormat::RG16Snorm: return 32;
		case InternalFormat::RGB16Unorm: return 48;
		case InternalFormat::RGB16Snorm: return 48;
		case InternalFormat::RGB5A1Unorm: return 16;
		case InternalFormat::RGBA8Unorm: return 32;
		case InternalFormat::RGBA8Snorm: return 32;
		case InternalFormat::RGB10A2Unorm: return 32;
		case InternalFormat::RGB10A2Uint: return 32;
		case InternalFormat::RGBA16Unorm: return 64;
		case InternalFormat::RGBA8UnormSrgb: return 32;
		case InternalFormat::R16Float: return 16;
		case InternalFormat::RG16Float: return 32;
		case InternalFormat::RGB16Float: return 48;
		case InternalFormat::RGBA16Float: return 64;
		case InternalFormat::R32Float: return 32;
		case InternalFormat::RG32Float: return 64;
		case InternalFormat::RGB32Float: return 96;
		case InternalFormat::RGBA32Float: return 128;
		case InternalFormat::R11G11B10Float: return 32;
		case InternalFormat::RGB9E5Float: return 32;
		case InternalFormat::R8Int: return 8;
		case InternalFormat::R8Uint: return 8;
		case InternalFormat::R16Int: return 16;
		case InternalFormat::R16Uint: return 16;
		case InternalFormat::R32Int: return 32;
		case InternalFormat::R32Uint: return 32;
		case InternalFormat::RG8Int: return 16;
		case InternalFormat::RG8Uint: return 16;
		case InternalFormat::RG16Int: return 32;
		case InternalFormat::RG16Uint: return 32;
		case InternalFormat::RG32Int: return 64;
		case InternalFormat::RG32Uint: return 64;
		case InternalFormat::RGB16Int: return 48;
		case InternalFormat::RGB16Uint: return 48;
		case InternalFormat::RGB32Int: return 96;
		case InternalFormat::RGB32Uint: return 96;
		case InternalFormat::RGBA8Int: return 32;
		case InternalFormat::RGBA8Uint: return 32;
		case InternalFormat::RGBA16Int: return 64;
		case InternalFormat::RGBA16Uint: return 64;
		case InternalFormat::RGBA32Int: return 128;
		case InternalFormat::RGBA32Uint: return 128;
		case InternalFormat::BGRA8Unorm: return 32;
		case InternalFormat::BGRA8UnormSrgb: return 32;
		case InternalFormat::R5G6B5Unorm: return 16;
		case InternalFormat::D32Float: return 32;
		case InternalFormat::D16Unorm: return 16;
		case InternalFormat::D32FloatS8X24: return 64;
		case InternalFormat::D24UnormS8: return 32;
		case InternalFormat::BC1Unorm: return 64;
		case InternalFormat::BC1UnormSrgb: return 64; 
		case InternalFormat::BC2Unorm: return 128;
		case InternalFormat::BC2UnormSrgb: return 128;
		case InternalFormat::BC3Unorm: return 128;
		case InternalFormat::BC3UnormSrgb: return 128;
		case InternalFormat::BC4Unorm: return 64;
		case InternalFormat::BC4Snorm: return 64;
		case InternalFormat::BC5Unorm: return 128;
		case InternalFormat::BC5Snorm: return 128;
		case InternalFormat::BC6HS16: return 128;
		case InternalFormat::BC6HU16: return 128;
		case InternalFormat::BC7Unorm: return 128;
		case InternalFormat::BC7UnormSrgb: return 128;
		case InternalFormat::ETC2RGB8Unorm: return 64;
		default: return 0;
	}
}

//-----------------

std::string toString(InternalFormat format) {
	switch (format) {
		case InternalFormat::Unknown: return "Unknown";
		case InternalFormat::R8Unorm: return "R8Unorm";
		case InternalFormat::R8Snorm: return "R8Snorm";
		case InternalFormat::R16Unorm: return "R16Unorm";
		case InternalFormat::R16Snorm: return "R16Snorm";
		case InternalFormat::RG8Unorm: return "RG8Unorm";
		case InternalFormat::RG8Snorm: return "RG8Snorm";
		case InternalFormat::RG16Unorm: return "RG16Unorm";
		case InternalFormat::RG16Snorm: return "RG16Snorm";
		case InternalFormat::RGB16Unorm: return "RGB16Unorm";
		case InternalFormat::RGB16Snorm: return "RGB16Snorm";
		case InternalFormat::RGB5A1Unorm: return "RGB5A1Unorm";
		case InternalFormat::RGBA8Unorm: return "RGBA8Unorm";
		case InternalFormat::RGBA8Snorm: return "RGBA8Snorm";
		case InternalFormat::RGB10A2Unorm: return "RGB10A2Unorm";
		case InternalFormat::RGB10A2Uint: return "RGB10A2Uint";
		case InternalFormat::RGBA16Unorm: return "RGBA16Unorm";
		case InternalFormat::RGBA8UnormSrgb: return "RGBA8UnormSrgb";
		case InternalFormat::R16Float: return "R16Float";
		case InternalFormat::RG16Float: return "RG16Float";
		case InternalFormat::RGB16Float: return "RGB16Float";
		case InternalFormat::RGBA16Float: return "RGBA16Float";
		case InternalFormat::R32Float: return "R32Float";
		case InternalFormat::RG32Float: return "RG32Float";
		case InternalFormat::RGB32Float: return "RGB32Float";
		case InternalFormat::RGBA32Float: return "RGBA32Float";
		case InternalFormat::R11G11B10Float: return "R11G11B10Float";
		case InternalFormat::RGB9E5Float: return "RGB9E5Float";
		case InternalFormat::R8Int: return "R8Int";
		case InternalFormat::R8Uint: return "R8Uint";
		case InternalFormat::R16Int: return "R16Int";
		case InternalFormat::R16Uint: return "R16Uint";
		case InternalFormat::R32Int: return "R32Int";
		case InternalFormat::R32Uint: return "R32Uint";
		case InternalFormat::RG8Int: return "RG8Int";
		case InternalFormat::RG8Uint: return "RG8Uint";
		case InternalFormat::RG16Int: return "RG16Int";
		case InternalFormat::RG16Uint: return "RG16Uint";
		case InternalFormat::RG32Int: return "RG32Int";
		case InternalFormat::RG32Uint: return "RG32Uint";
		case InternalFormat::RGB16Int: return "RGB16Int";
		case InternalFormat::RGB16Uint: return "RGB16Uint";
		case InternalFormat::RGB32Int: return "RGB32Int";
		case InternalFormat::RGB32Uint: return "RGB32Uint";
		case InternalFormat::RGBA8Int: return "RGBA8Int";
		case InternalFormat::RGBA8Uint: return "RGBA8Uint";
		case InternalFormat::RGBA16Int: return "RGBA16Int";
		case InternalFormat::RGBA16Uint: return "RGBA16Uint";
		case InternalFormat::RGBA32Int: return "RGBA32Int";
		case InternalFormat::RGBA32Uint: return "RGBA32Uint";
		case InternalFormat::BGRA8Unorm: return "BGRA8Unorm";
		case InternalFormat::BGRA8UnormSrgb: return "BGRA8UnormSrgb";
		case InternalFormat::R5G6B5Unorm: return "R5G6B5Unorm";
		case InternalFormat::D32Float: return "D32Float";
		case InternalFormat::D16Unorm: return "D16Unorm";
		case InternalFormat::D32FloatS8X24: return "D32FloatS8X24";
		case InternalFormat::D24UnormS8: return "D24UnormS8";
		case InternalFormat::BC1Unorm: return "BC1Unorm";
		case InternalFormat::BC1UnormSrgb: return "BC1UnormSrgb"; 
		case InternalFormat::BC2Unorm: return "BC2Unorm";
		case InternalFormat::BC2UnormSrgb: return "BC2UnormSrgb";
		case InternalFormat::BC3Unorm: return "BC3Unorm";
		case InternalFormat::BC3UnormSrgb: return "BC3UnormSrgb";
		case InternalFormat::BC4Unorm: return "BC4Unorm";
		case InternalFormat::BC4Snorm: return "BC4Snorm";
		case InternalFormat::BC5Unorm: return "BC5Unorm";
		case InternalFormat::BC5Snorm: return "BC5Snorm";
		case InternalFormat::BC6HS16: return "BC6HS16";
		case InternalFormat::BC6HU16: return "BC6HU16";
		case InternalFormat::BC7Unorm: return "BC7Unorm";
		case InternalFormat::BC7UnormSrgb: return "BC7UnormSrgb";
		case InternalFormat::ETC2RGB8Unorm: return "ETC2RGB8Unorm";
	}
}

//-----------------

std::string toString(ResourceUsage usage) {
	switch(usage) {
		case ResourceUsage::Undefined: return "Undefined";
		case ResourceUsage::PreInitialized: return "PreInitialized";
		case ResourceUsage::General: return "General";
		case ResourceUsage::RenderTarget: return "RenderTarget";
		case ResourceUsage::DepthStencil: return "DepthStencil";
		case ResourceUsage::ShaderResource: return "ShaderResource";
		case ResourceUsage::CopySource: return "CopySource";
		case ResourceUsage::CopyDestination: return "CopyDestination";
		case ResourceUsage::Present: return "Present";
		case ResourceUsage::ShaderWrite: return "ShaderWrite";
		case ResourceUsage::IndexBuffer: return "IndexBuffer";
		case ResourceUsage::VertexBuffer: return "VertexBuffer";
		case ResourceUsage::IndirectBuffer: return "IndirectBuffer";
	}
	return "";
}

//-----------------

size_t getDataSize(const ImageFormat& format) {
	size_t baseSize = format.extent.getX() * format.extent.getY() * format.extent.getZ();
	size_t size = baseSize;
	for(uint32_t i=1; i<format.mipLevels; ++i)
		size += baseSize >> (2*i);
	return size * format.layers * format.samples * getDataSize(format.pixelFormat);
}

//-----------------

std::string toString(const ImageFormat& format) {
	std::stringstream ss;
	ss << "ImageFormat(" << " * " << format.extent.x() << "x" << format.extent.y() << "x" << format.extent.z()
			<< " " << toString(format.pixelFormat) << " layers " << format.layers << " mip " << format.mipLevels << " samples " << format.samples << ")";
	return ss.str();
}

//-----------------

} /* Rendering */