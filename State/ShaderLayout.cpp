/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ShaderLayout.h"

#include <sstream>

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
		default: return "Unknown";
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