/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "UniformBuffer.h"

#include "Uniform.h"
#include "../Core/Device.h"
#include "../Core/CommandBuffer.h"
#include "../BufferObject.h"
#include "../State/ShaderLayout.h"

#include <Util/Macros.h>
#include <Util/Resources/ResourceFormat.h>
#include <Util/Resources/ResourceAccessor.h>
#include <Util/Graphics/Color.h>

#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>
#include <Geometry/Matrix4x4.h>

namespace Rendering {

//---------------

UniformBuffer::Ref UniformBuffer::create(const DeviceRef& device, const Util::ResourceFormat& format, uint32_t arraySize, bool pushConstant) {
	Ref obj = new UniformBuffer(pushConstant);
	if(!obj->init(device, format, arraySize)) {
		return nullptr;
	}
	return obj;
}

//---------------

UniformBuffer::Ref UniformBuffer::createFromShaderResource(const DeviceRef& device, const ShaderResource& resource) {
	bool pushConstant = resource.layout.type == ShaderResourceType::PushConstant;
	WARN_AND_RETURN_IF(!pushConstant && resource.layout.type != ShaderResourceType::BufferUniform, "UniformBuffer can only created from resource type 'PushConstant' or 'BufferUniform", nullptr);
	
	Ref obj = new UniformBuffer(pushConstant);
	if(!obj->init(device, resource.format, resource.layout.elementCount)) {
		return nullptr;
	}
	return obj;
}

//---------------

UniformBuffer::UniformBuffer(bool pushConstant) : pushConstant(pushConstant), dataHasChanged(true) {}

//---------------

UniformBuffer::~UniformBuffer() = default;

//---------------

void UniformBuffer::applyUniform(const Uniform& uniform, uint32_t index) {
	// TODO: check uniform format
	writeData(uniform.getNameId(), uniform.getData(), uniform.getDataSize(), index);
}

//---------------

void UniformBuffer::writeData(const Util::StringIdentifier& name, const uint8_t* data, size_t size, uint32_t index) {
	const auto& format = accessor->getFormat();
	uint32_t location = format.getAttributeLocation(name);
	if(location < format.getSize() && index < arraySize) {
		accessor->writeRawValue(index, location, data, size);
		dataHasChanged = true;
	}
}

//---------------

void UniformBuffer::flush(const CommandBufferRef& cmd, bool force) {
	WARN_AND_RETURN_IF(!cmd, "Uniform::flush: Invalud command buffer.",);
	if(!force && !dataHasChanged)
		return;
	
	if(pushConstant) {
		cmd->pushConstants(cache);
	} else {
		cmd->updateBuffer(buffer->getBuffer(), cache.data(), cache.size());
	}
	dataHasChanged = false;
}

//---------------

void UniformBuffer::bind(const CommandBufferRef& cmd, uint32_t binding, uint32_t set) {
	WARN_AND_RETURN_IF(!cmd, "Uniform::flush: Invalud command buffer.",);
	flush(cmd);
	if(!pushConstant)
		cmd->bindBuffer(buffer, set, binding);
}

//---------------

bool UniformBuffer::init(const DeviceRef& device, const Util::ResourceFormat& format, uint32_t arraySize) {
	// Create and allocate GPU buffer unless we use push constants
	if(!pushConstant) {
		// TODO: Allocate buffer from buffer pool?
		buffer = BufferObject::create(device);
		buffer->allocate(format.getSize() * arraySize, ResourceUsage::ShaderResource, MemoryUsage::GpuOnly);
		WARN_AND_RETURN_IF(!buffer->isValid(), "Failed to allocate uniform buffer.", false);
	}

	// Allocate cache
	cache.resize(format.getSize() * arraySize);

	// Create accessor
	accessor = Util::ResourceAccessor::create(cache.data(), cache.size(), format);

	return true;
}

//---------------

} /* Rendering */