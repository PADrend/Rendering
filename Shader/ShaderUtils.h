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

#include "../Core/Common.h"

#include <Util/References.h>
#include <Util/Utils.h>

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace Rendering {
class Shader;

//! @addtogroup shader
//! @{


//==================================================================
// ShaderLayout

//! Layout of a single shader resource
struct ShaderResourceLayout {
	//uint32_t binding; //! The binding number of this shader resource
	ShaderResourceType type; //! The type of shader resource.
	ShaderStage stages = ShaderStage::All; //! The shader stages the resource can be accessed from.
	uint32_t elementCount = 1; //! The number of elements in an array of resources.
	bool dynamic = false; //! Controls if the shader resource is dynamic.

	bool operator==(const ShaderResourceLayout& o) const {
		return elementCount == o.elementCount && type == o.type && stages == o.stages && dynamic == o.dynamic;
	}
};

//-------------

//! Layout for a set of shader resources
class ShaderResourceLayoutSet {
public:
	//! sets the shader resource layout for a shader resource binding.
	ShaderResourceLayoutSet& setLayout(uint32_t binding, const ShaderResourceLayout& value) { layouts[binding] = value; return *this; }
	//! sets the shader resource layout for all shader resource bindings.
	ShaderResourceLayoutSet& setLayouts(const std::map<uint32_t, ShaderResourceLayout>& values) { layouts = values; return *this; }

	//! @see{setLayout()}
	const ShaderResourceLayout& getLayout(uint32_t binding) const { return layouts.at(binding); }
	//! @see{setLayouts()}
	const std::map<uint32_t, ShaderResourceLayout>& getLayouts() const { return layouts; }
	//! @see{setLayout()}
	bool hasLayout(uint32_t binding) const { return layouts.find(binding) != layouts.end(); }
private:
	std::map<uint32_t, ShaderResourceLayout> layouts;
};

//-------------

struct PushConstantRange {
	size_t offset; //! The start offset of the range.
	size_t size; //! The size consumed by the range.
	ShaderStage stages; //! The shader stages the push constant can be accessed from.
	
	bool operator==(const PushConstantRange& o) const {
		return offset == o.offset && size == o.size && stages == o.stages;
	}
};

//-------------

//! Layout for all resources in a shader
class ShaderLayout {
public:
	//! sets the shader resource layouts for a shader resource set.
	ShaderLayout& setLayoutSet(uint32_t set, const ShaderResourceLayoutSet& value) { layoutSets[set] = value; return *this; }
	//! sets the shader resource layouts for all shader resource sets.
	ShaderLayout& setLayoutSets(const std::map<uint32_t, ShaderResourceLayoutSet>& values) { layoutSets = values; return *this; }
	//! sets the range for a push constant.
	ShaderLayout& setPushConstantRange(uint32_t index, const PushConstantRange& value) { ranges[index] = value; return *this; }
	//! sets the ranges for all push constants.
	ShaderLayout& setPushConstantRanges(const std::vector<PushConstantRange>& values) { ranges = values; return *this; }
	//! sets the number of push constant ranges.
	ShaderLayout& setPushConstantCount(uint32_t value) { ranges.resize(value); return *this; }

	//! @see{setLayoutSet()}
	const ShaderResourceLayoutSet& getLayoutSet(uint32_t set) const { return layoutSets.at(set); }
	//! @see{setLayoutSets()}
	const std::map<uint32_t, ShaderResourceLayoutSet>& getLayoutSets() const { return layoutSets; }
	//! @see{setLayoutSet()}
	bool hasLayoutSet(uint32_t set) const { return layoutSets.find(set) != layoutSets.end(); }
	//! @see{setPushConstantRange()}
	const PushConstantRange& getPushConstantRange(uint32_t index) const { return ranges[index]; }
	//! @see{setPushConstantRanges()}
	const std::vector<PushConstantRange>& getPushConstantRanges() const { return ranges; }
	//! @see{setPushConstantRange()}
	uint32_t getPushConstantCount() const { return ranges.size(); }
private:
	std::map<uint32_t, ShaderResourceLayoutSet> layoutSets;
	std::vector<PushConstantRange> ranges;
};

//-------------

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

	bool operator==(const ShaderResource& o) const {
		return name == o.name && layout == o.layout && set == o.set && binding == o.binding && location == o.location && inputAttachmentIndex == o.inputAttachmentIndex
			&& vecSize == o.vecSize && columns == o.columns && offset == o.offset && size == o.size && constantId == o.constantId;
	}
	bool operator!=(const ShaderResource& o) const { return !(*this == o); }
};
using ShaderResourceList = std::vector<ShaderResource>;

//-------------

std::string toString(ShaderStage stage);
std::string toString(ShaderResourceType type);
std::string toString(const ShaderResource& resource);

//==================================================================

namespace ShaderUtils {

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

//==================================================================

namespace std {

template <> struct hash<Rendering::ShaderResourceLayout> {
	std::size_t operator()(const Rendering::ShaderResourceLayout& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.elementCount);
		Util::hash_combine(result, state.stages);
		Util::hash_combine(result, state.type);
		return result;
	}
};

//-------------

template <> struct hash<Rendering::PushConstantRange> {
	std::size_t operator()(const Rendering::PushConstantRange& state) const {
		std::size_t result = 0;
		Util::hash_combine(result, state.offset);
		Util::hash_combine(result, state.size);
		Util::hash_combine(result, state.stages);
		return result;
	}
};

//-------------

template <> struct hash<Rendering::ShaderResourceLayoutSet> {
	std::size_t operator()(const Rendering::ShaderResourceLayoutSet& state) const {
		std::size_t result = 0;
		for(auto& e : state.getLayouts()) {
			Util::hash_combine(result, e.first);
			Util::hash_combine(result, e.second);
		}
		return result;
	}
};

//-------------

template <> struct hash<Rendering::ShaderLayout> {
	std::size_t operator()(const Rendering::ShaderLayout& state) const {
		std::size_t result = 0;
		for(auto& e : state.getLayoutSets()) {
			Util::hash_combine(result, e.first);
			Util::hash_combine(result, e.second);
		}
		for(auto& r : state.getPushConstantRanges()) {
			Util::hash_combine(result, r);
		}
		return result;
	}
};

} /* std */

#endif /* RENDERING_SHADERUTILS_H_ */
