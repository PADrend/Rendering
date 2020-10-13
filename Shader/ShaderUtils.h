/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADERUTILS_H_
#define RENDERING_SHADERUTILS_H_

#include <Util/References.h>

#include <cstdint>
#include <string>
#include <vector>

namespace Rendering {
class Shader;

//! @addtogroup shader
//! @{

//-------------

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

//-------------

enum class ShaderResourceType {
	Input,
	InputAttachment,
	Output,
	Image,
	ImageSampler,
	ImageStorage,
	Sampler,
	BufferUniform,
	BufferStorage,
	PushConstant,
	SpecializationConstant
};

//-------------

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

	bool operator==(const ShaderResource& o) {
		return name == o.name && type == o.type && set == o.size && binding == o.binding && location == o.location && input_attachment_index == o.input_attachment_index
			&& vec_size == o.vec_size && columns == o.columns && array_size == o.array_size && offset == o.offset && size == o.size && constant_id == o.constant_id && dynamic == o.dynamic;
	}
	bool operator!=(const ShaderResource& o) { return !(*this == o); }
};
using ShaderResourceList = std::vector<ShaderResource>;

//-------------

namespace ShaderUtils {

std::string toString(ShaderStage stage);
std::string toString(ShaderResourceType type);
std::string toString(const ShaderResource& resource);

/**
 * Reflects the shader resources from a compiled shader. 
 * @return List of shader resources.
 */
ShaderResourceList reflect(ShaderStage stage, const std::vector<uint32_t>& code);


//! Create a shader that writes the pixel normal into the color buffer.
Util::Reference<Shader> createNormalToColorShader();

//! Create a simple shader without any effects.
Util::Reference<Shader> createDefaultShader();

}

//! @}
}

#endif /* RENDERING_SHADERUTILS_H_ */
