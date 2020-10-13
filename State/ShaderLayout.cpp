/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ShaderLayout.h"

namespace Rendering {

//-------------

std::string toString(ShaderStage stage) {
	switch(stage) {
		case ShaderStage::Undefined: return "Undefined";
		case ShaderStage::Vertex: return "Vertex";
		case ShaderStage::TessellationControl: return "TessellationControl";
		case ShaderStage::TessellationEvaluation: return "TessellationEvaluation";
		case ShaderStage::Geometry: return "Geometry";
		case ShaderStage::Fragment: return "Fragment";
		case ShaderStage::Compute: return "Compute";
		case ShaderStage::All: return "All";
		default: return "";
	}
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

std::string toString(const ShaderResource& resource) {
	return "ShaderResource(name " + resource.name + ", " 
		+ "stage " + toString(resource.layout.stages) + ", "
		+ "type " + toString(resource.layout.type) + ", "
		+ "set " + std::to_string(resource.set) + ", "
		+ "binding " + std::to_string(resource.binding) + ", "
		+ "location " + std::to_string(resource.location) + ", "
		+ "inputAttachmentIndex " + std::to_string(resource.inputAttachmentIndex) + ", "
		+ "vecSize " + std::to_string(resource.vecSize) + ", "
		+ "columns " + std::to_string(resource.columns) + ", "
		+ "arraySize " + std::to_string(resource.layout.elementCount) + ", "
		+ "offset " + std::to_string(resource.offset) + ", "
		+ "size " + std::to_string(resource.size) + ", "
		+ "constantId " + std::to_string(resource.constantId) + ", "
		+ "dynamic " + std::to_string(resource.layout.dynamic) + ")";
}

//-------------

} /* Rendering */