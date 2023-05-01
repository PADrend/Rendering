/*
	This file is part of the Platform for Algorithm Development and Rendering (PADrend).
	Web page: http://www.padrend.de/
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2023 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADERREFLECTION_H_
#define RENDERING_SHADERREFLECTION_H_

#include <Util/References.h>
#include <Util/Utils.h>
#include <Util/Resources/ResourceFormat.h>

#include <nvrhi/nvrhi.h>

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace Rendering {

//! @addtogroup shader
//! @{

//! Layout of a single shader resource
struct ShaderResourceLayout {
	//uint32_t binding; //! The binding number of this shader resource
	nvrhi::ResourceType type; //! The type of shader resource.
	nvrhi::ShaderType stages = nvrhi::ShaderType::All; //! The shader stages the resource can be accessed from.
	uint32_t elementCount = 1; //! The number of elements in an array of resources.
	bool dynamic = false; //! Controls if the shader resource is dynamic.

	bool operator==(const ShaderResourceLayout& o) const {
		return elementCount == o.elementCount && type == o.type && dynamic == o.dynamic;// && stages == o.stages;
	}
};

struct ShaderResource {
	std::string name;
	uint32_t set;
	uint32_t binding;
	ShaderResourceLayout layout;
	uint32_t location;
	uint32_t inputAttachmentIndex;
	uint32_t vecSize;
	uint32_t columns;
	uint32_t constantId;
	uint32_t offset;
	uint32_t size;
	Util::ResourceFormat format;

	bool operator==(const ShaderResource& o) const {
		return name == o.name && layout == o.layout && set == o.set && binding == o.binding && location == o.location && inputAttachmentIndex == o.inputAttachmentIndex
			&& vecSize == o.vecSize && columns == o.columns && offset == o.offset && size == o.size && constantId == o.constantId && format == o.format;
	}
	bool operator!=(const ShaderResource& o) const { return !(*this == o); }

	operator bool() const { return !name.empty(); }
};

struct ShaderReflection {
	std::vector<ShaderResource> resources;
};


/**
 * Reflects the shader resources from a compiled shader. 
 * @return List of shader resources.
 */
RENDERINGAPI ShaderReflection reflect(nvrhi::ShaderType stage, const std::vector<uint32_t>& code);

//! @}
}

#endif /* RENDERING_SHADERREFLECTION_H_ */
