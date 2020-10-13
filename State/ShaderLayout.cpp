/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ShaderLayout.h"

#include <Util/StringUtils.h>

#include <sstream>

namespace Rendering {
//-------------

std::string toString(const ShaderLayout& layout) {
	std::stringstream ss;
	ss << "ShaderLayout:" << std::endl;
	for(auto& set : layout.getLayoutSets()) {
		ss << "  set " << set.first << ": " << std::endl;
		for(auto& binding : set.second.getLayouts() ) {
			ss << "    binding " << binding.first << ": ";
			ss << toString(binding.second.type);
			ss << "[" << binding.second.elementCount << "] ";
			if(binding.second.dynamic)
				ss << "(dynamic) ";
			ss << "{" << toString(binding.second.stages) << "}" << std::endl;
		}
	}
	if(layout.getPushConstantCount() > 0) {
		ss << "  push constant ranges: " << std::endl;
		for(auto& range : layout.getPushConstantRanges()) {
			ss << "    [" << range.offset << ", " << (range.offset+range.size) << "] " << toString(range.stages) << std::endl;
		}
	}
	return ss.str();
}

//-------------

std::string toString(ShaderStage stage) {
	std::vector<std::string> stages;
	if((stage & ShaderStage::Vertex) == ShaderStage::Vertex) stages.emplace_back("Vertex");
	if((stage & ShaderStage::TessellationControl) == ShaderStage::TessellationControl) stages.emplace_back("TessControl");
	if((stage & ShaderStage::TessellationEvaluation) == ShaderStage::TessellationEvaluation) stages.emplace_back("TessEvaluation");
	if((stage & ShaderStage::Geometry) == ShaderStage::Geometry) stages.emplace_back("Geometry");
	if((stage & ShaderStage::Fragment) == ShaderStage::Fragment) stages.emplace_back("Fragment");
	if((stage & ShaderStage::Compute) == ShaderStage::Compute) stages.emplace_back("Compute");
	return Util::StringUtils::implode(stages.begin(), stages.end(), "|");
}

//-------------

std::string toString(ShaderResourceType type) {
	switch(type) {
		case ShaderResourceType::Input: return "Input";
		case ShaderResourceType::InputAttachment: return "InputAttachment";
		case ShaderResourceType::Output: return "Output";
		case ShaderResourceType::Image: return "Image";
		case ShaderResourceType::ImageSampler: return "ImageSampler";
		case ShaderResourceType::ImageStorage: return "ImageStorage";
		case ShaderResourceType::Sampler: return "Sampler";
		case ShaderResourceType::BufferUniform: return "BufferUniform";
		case ShaderResourceType::BufferStorage: return "BufferStorage";
		case ShaderResourceType::PushConstant: return "PushConstant";
		case ShaderResourceType::SpecializationConstant: return "SpecializationConstant";
		default: return "";
	}
}

//-------------

std::string toString(const ShaderResource& resource, bool formatted) {
	std::stringstream ss;
	ss << "ShaderResource(" << resource.name << ", ";
	ss << "stage: " << toString(resource.layout.stages) << ", ";
	ss << "type: " << toString(resource.layout.type) << ", ";
	ss << "set: " << resource.set << ", ";
	ss << "binding: " << resource.binding << ", ";
	ss << "location: " << resource.location << ", ";
	ss << "inputAttachmentIndex: " << resource.inputAttachmentIndex << ", ";
	ss << "vecSize: " << resource.vecSize << ", ";
	ss << "columns: " << resource.columns << ", ";
	ss << "arraySize: " << resource.layout.elementCount << ", ";
	ss << "offset: " << resource.offset << ", ";
	ss << "size: " << resource.size << ", ";
	ss << "constantId: " << resource.constantId << ", ";
	ss << "dynamic: " << resource.layout.dynamic << ", ";
	ss << "format: " << resource.format.toString(formatted);
	ss << ")";
	return ss.str();
}

//-------------

} /* Rendering */