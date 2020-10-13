/*
  This file is part of the Rendering library.
  Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
  
  This library is subject to the terms of the Mozilla Public License, v. 2.0.
  You should have received a copy of the MPL along with this library; see the
  file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_COMMON_H_
#define RENDERING_CORE_COMMON_H_

#include "ApiHandles.h"
#include <Geometry/Vec3.h>
#include <Util/Utils.h>

namespace Geometry {
using Vec3ui = _Vec3<uint32_t>;
} /* Geometry */

namespace Rendering {

//---------------------------

enum class MemoryUsage {
	Unknown, //! No intended memory usage specified.
	CpuOnly, //! Memory will be mappable on host.
	GpuOnly, //! Memory will be used on device only.
	CpuToGpu, //! Memory that is both mappable on host and preferably fast to access by GPU.
	GpuToCpu //! Memory mappable on host and cached.
};

//---------------------------

enum class QueueFamily : uint8_t {
	None = 0,
	Transfer = 1 << 0,
	Compute = 1 << 1,
	Graphics = 1 << 2,
	Present = 1 << 3,
};

//---------------------------

enum class PipelineType {
	Graphics = 0,
	Compute,
};

//---------------------------

enum class InternalFormat : std::uint8_t {
	Unknown,
	R8Unorm,
	R8Snorm,
	R16Unorm,
	R16Snorm,
	RG8Unorm,
	RG8Snorm,
	RG16Unorm,
	RG16Snorm,
	RGB16Unorm,
	RGB16Snorm,
	RGB5A1Unorm,
	RGBA8Unorm,
	RGBA8Snorm,
	RGB10A2Unorm,
	RGB10A2Uint,
	RGBA16Unorm,
	RGBA8UnormSrgb,
	R16Float,
	RG16Float,
	RGB16Float,
	RGBA16Float,
	R32Float,
	RG32Float,
	RGB32Float,
	RGBA32Float,
	R11G11B10Float,
	RGB9E5Float,
	R8Int,
	R8Uint,
	R16Int,
	R16Uint,
	R32Int,
	R32Uint,
	RG8Int,
	RG8Uint,
	RG16Int,
	RG16Uint,
	RG32Int,
	RG32Uint,
	RGB16Int,
	RGB16Uint,
	RGB32Int,
	RGB32Uint,
	RGBA8Int,
	RGBA8Uint,
	RGBA16Int,
	RGBA16Uint,
	RGBA32Int,
	RGBA32Uint,

	BGRA8Unorm,
	BGRA8UnormSrgb,
	
	R5G6B5Unorm,

	// Depth-stencil
	D32Float,
	D16Unorm,
	D32FloatS8X24,
	D24UnormS8,

	// Compressed formats
	BC1Unorm,   // DXT1
	BC1UnormSrgb, 
	BC2Unorm,   // DXT3
	BC2UnormSrgb,
	BC3Unorm,   // DXT5
	BC3UnormSrgb,
	BC4Unorm,   // RGTC Unsigned Red
	BC4Snorm,   // RGTC Signed Red
	BC5Unorm,   // RGTC Unsigned RG
	BC5Snorm,   // RGTC Signed RG
	BC6HS16,
	BC6HU16,
	BC7Unorm,
	BC7UnormSrgb,
};


//---------------------------

//! Comparison functions
enum class ComparisonFunc {
	Disabled, //! specifies that comparison is disabled.
	Never, //! specifies that the test never passes.
	Less, //! specifies that the test passes when R < S.
	Equal, //! specifies that the test passes when R = S.
	LessOrEqual, //! specifies that the test passes when R ≤ S.
	Greater, //! specifies that the test passes when R > S.
	NotEqual, //! specifies that the test passes when R ≠ S.
	GreaterOrEqual, //! specifies that the test passes when R ≥ S.
	Always, //! specifies that the test always passes.
};

//---------------------------

//! Resource usage. Keeps track of how a resource was last used
enum class ResourceUsage {
	Undefined = 0,
	PreInitialized,
	General,
	RenderTarget,
	DepthStencil,
	ShaderResource,
	CopySource,
	CopyDestination,
	Present,
	ShaderWrite,
	IndexBuffer,
	VertexBuffer,
	IndirectBuffer,
};

//---------------------------

enum ImageFilter {
	Nearest,
	Linear,		
};

//---------------------------

enum ImageAddressMode {
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder
};

//---------------------------

struct ImageFormat {
	Geometry::Vec3ui extent;
	InternalFormat pixelFormat = InternalFormat::RGBA8Unorm;
	uint32_t mipLevels = 1;
	uint32_t layers = 1;
	uint32_t samples = 1;
};

//---------------------------

enum class ShaderStage : uint8_t {
	Undefined = 0,
	Vertex = 1 << 0,
	TessellationControl = 1 << 1,
	TessellationEvaluation = 1 << 2,
	Geometry = 1 << 3,
	Fragment = 1 << 4,
	Compute = 1 << 5,
};

//-------------

inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
	return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

//-------------

inline ShaderStage operator&(ShaderStage a, ShaderStage b) {
	return static_cast<ShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

//---------------------------

enum class ShaderResourceType {
	Input = 0,
	InputAttachment,
	Output,
	Image,
	ImageSampler,
	ImageStorage,
	Sampler,
	BufferUniform,
	BufferStorage,
	PushConstant,
	SpecializationConstant,
	ResourceTypeCount
};

//---------------------------

struct ShaderResource {
	std::string name;
	ShaderStage stages;
	ShaderResourceType type;
	uint32_t set;
	uint32_t binding;
	uint32_t location;
	uint32_t input_attachment_index;
	uint32_t vec_size;
	uint32_t columns;
	uint32_t array_size;
	uint32_t offset;
	uint32_t size;
	uint32_t constant_id;
	bool dynamic;

	bool operator==(const ShaderResource& o) const {
		return name == o.name && type == o.type && set == o.size && binding == o.binding && location == o.location && input_attachment_index == o.input_attachment_index
			&& vec_size == o.vec_size && columns == o.columns && array_size == o.array_size && offset == o.offset && size == o.size && constant_id == o.constant_id && dynamic == o.dynamic;
	}
	bool operator!=(const ShaderResource& o) const { return !(*this == o); }
};
using ShaderResourceList = std::vector<ShaderResource>;

//---------------------------

std::string toString(ShaderStage stage);

//---------------------------

std::string toString(ShaderResourceType type);

//---------------------------

std::string toString(const ShaderResource& resource);

//---------------------------

} /* Rendering */

//---------------------------
// Hashing

//---------------------------

template <> struct std::hash<Rendering::ImageFormat> {
	std::size_t operator()(const Rendering::ImageFormat &format) const {
		std::size_t result = 0;
		Util::hash_combine(result, format.extent.getX());
		Util::hash_combine(result, format.extent.getY());
		Util::hash_combine(result, format.extent.getZ());
		Util::hash_combine(result, format.pixelFormat);
		Util::hash_combine(result, format.mipLevels);
		Util::hash_combine(result, format.layers);
		Util::hash_combine(result, format.samples);
		return result;
	}
};

//---------------------------

template <> struct std::hash<Rendering::ShaderResource> {
	std::size_t operator()(const Rendering::ShaderResource& resource) const {
		std::size_t result = 0;
		Util::hash_combine(result, resource.name);
		Util::hash_combine(result, resource.stages);
		Util::hash_combine(result, resource.type);
		Util::hash_combine(result, resource.set);
		Util::hash_combine(result, resource.binding);
		Util::hash_combine(result, resource.location);
		Util::hash_combine(result, resource.input_attachment_index);
		Util::hash_combine(result, resource.vec_size);
		Util::hash_combine(result, resource.columns);
		Util::hash_combine(result, resource.array_size);
		Util::hash_combine(result, resource.offset);
		Util::hash_combine(result, resource.size);
		Util::hash_combine(result, resource.constant_id);
		Util::hash_combine(result, resource.dynamic);
		return result;
	}
};

//---------------------------

#endif /* end of include guard: RENDERING_CORE_COMMON_H_ */
