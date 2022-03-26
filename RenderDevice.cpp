/*
	This file is part of the Platform for Algorithm Development and Rendering (PADrend).
	Web page: http://www.padrend.de/
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2022 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "RenderDevice.h"

#include <nvrhi/nvrhi.h>

#include <sstream>

namespace Rendering {
using namespace Util;

std::string toString(DeviceType value) {
	switch (value) {
		case DeviceType::Other: return "Other";
		case DeviceType::IntegratedGpu: return "IntegratedGpu";
		case DeviceType::DiscreteGpu: return "DiscreteGpu";
		case DeviceType::VirtualGpu: return "VirtualGpu";
		case DeviceType::Cpu: return "Cpu";
		default: return "invalid";
	}
}

std::string toString(QueueFamily value) {
	std::stringstream ss;
	if (isFlagSet(value, QueueFamily::Graphics)) ss << "Graphics | ";
	if (isFlagSet(value, QueueFamily::Compute)) ss << "Compute | ";
	if (isFlagSet(value, QueueFamily::Transfer)) ss << "Transfer | ";
	if (isFlagSet(value, QueueFamily::Present)) ss << "Present | ";
	std::string str = ss.str();
	return !str.empty() ? str.substr(0, str.size() - 3) : "";
}

nvrhi::Format convertFormat(Util::PixelFormat format) {
	switch (format) {
		case PixelFormat::R8UInt: return nvrhi::Format::R8_UINT;
		case PixelFormat::RG8UInt: return nvrhi::Format::RG8_UINT;
		case PixelFormat::RGBA8UInt: return nvrhi::Format::RGBA8_UINT;
		case PixelFormat::R8UNorm: return nvrhi::Format::R8_UNORM;
		case PixelFormat::RG8UNorm: return nvrhi::Format::RG8_UNORM;
		case PixelFormat::RGBA8UNorm: return nvrhi::Format::RGBA8_UNORM;
		case PixelFormat::sRGBA8UNorm: return nvrhi::Format::SRGBA8_UNORM;
		case PixelFormat::R8SInt: return nvrhi::Format::R8_SINT;
		case PixelFormat::RG8SInt: return nvrhi::Format::RG8_SINT;
		case PixelFormat::RGBA8SInt: return nvrhi::Format::RGBA8_SINT;
		case PixelFormat::R8SNorm: return nvrhi::Format::R8_SNORM;
		case PixelFormat::RG8SNorm: return nvrhi::Format::RG8_SNORM;
		case PixelFormat::RGBA8SNorm: return nvrhi::Format::RGBA8_SNORM;
		case PixelFormat::R16UInt: return nvrhi::Format::R16_UINT;
		case PixelFormat::RG16UInt: return nvrhi::Format::RG16_UINT;
		case PixelFormat::RGBA16UInt: return nvrhi::Format::RGBA16_UINT;
		case PixelFormat::R16UNorm: return nvrhi::Format::R16_UNORM;
		case PixelFormat::RG16UNorm: return nvrhi::Format::RG16_UNORM;
		case PixelFormat::RGBA16UNorm: return nvrhi::Format::RGBA16_UNORM;
		case PixelFormat::R16SInt: return nvrhi::Format::R16_SINT;
		case PixelFormat::RG16SInt: return nvrhi::Format::RG16_SINT;
		case PixelFormat::RGBA16SInt: return nvrhi::Format::RGBA16_SINT;
		case PixelFormat::R16SNorm: return nvrhi::Format::R16_SNORM;
		case PixelFormat::RG16SNorm: return nvrhi::Format::RG16_SNORM;
		case PixelFormat::RGBA16SNorm: return nvrhi::Format::RGBA16_SNORM;
		case PixelFormat::R32UInt: return nvrhi::Format::R32_UINT;
		case PixelFormat::RG32UInt: return nvrhi::Format::RG32_UINT;
		case PixelFormat::RGB32UInt: return nvrhi::Format::RGB32_UINT;
		case PixelFormat::RGBA32UInt: return nvrhi::Format::RGBA32_UINT;
		case PixelFormat::R32SInt: return nvrhi::Format::R32_SINT;
		case PixelFormat::RG32SInt: return nvrhi::Format::RG32_SINT;
		case PixelFormat::RGB32SInt: return nvrhi::Format::RGB32_SINT;
		case PixelFormat::RGBA32SInt: return nvrhi::Format::RGBA32_SINT;
		case PixelFormat::BGRA8UNorm: return nvrhi::Format::BGRA8_UNORM;
		case PixelFormat::sBGRA8UNorm: return nvrhi::Format::SBGRA8_UNORM;
		case PixelFormat::BGR5A1UNorm: return nvrhi::Format::B5G5R5A1_UNORM;
		case PixelFormat::B5G6R5UNorm: return nvrhi::Format::B5G6R5_UNORM;
		case PixelFormat::RGB10A2UNorm: return nvrhi::Format::R10G10B10A2_UNORM;
		case PixelFormat::R11G11B10SFloat: return nvrhi::Format::R11G11B10_FLOAT;
		case PixelFormat::D16UNorm: return nvrhi::Format::D16;
		case PixelFormat::D24S8UNorm: return nvrhi::Format::D24S8;
		case PixelFormat::D32SFloat: return nvrhi::Format::D32;
		case PixelFormat::D32S8SFloat: return nvrhi::Format::D32S8;
		case PixelFormat::RGB8UNormBC1: return nvrhi::Format::BC1_UNORM;
		case PixelFormat::sRGB8UNormBC1: return nvrhi::Format::BC1_UNORM_SRGB;
		case PixelFormat::RGB8A4UNormBC2: return nvrhi::Format::BC2_UNORM;
		case PixelFormat::sRGB8A4UNormBC2: return nvrhi::Format::BC2_UNORM_SRGB;
		case PixelFormat::RGBA8UNormBC3: return nvrhi::Format::BC3_UNORM;
		case PixelFormat::sRGBA8UNormBC3: return nvrhi::Format::BC3_UNORM_SRGB;
		case PixelFormat::R8UNormBC4: return nvrhi::Format::BC4_UNORM;
		case PixelFormat::R8SNormBC4: return nvrhi::Format::BC4_SNORM;
		case PixelFormat::RG8UNormBC5: return nvrhi::Format::BC5_UNORM;
		case PixelFormat::RG8SNormBC5: return nvrhi::Format::BC5_SNORM;
		case PixelFormat::RGB16SFloatBC6H: return nvrhi::Format::BC6H_SFLOAT;
		case PixelFormat::RGBA8UNormBC7: return nvrhi::Format::BC7_UNORM;
		case PixelFormat::sRGBA8UNormBC7: return nvrhi::Format::BC7_UNORM_SRGB;
		default: return nvrhi::Format::UNKNOWN;
	}
}

} // namespace Rendering