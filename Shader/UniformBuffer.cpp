/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "UniformBuffer.h"

#include "../Core/Device.h"
#include "../BufferObject.h"

#include <Util/Resources/ResourceFormat.h>
#include <Util/Resources/ResourceAccessor.h>

namespace Rendering {

//---------------

UniformBuffer::Ref UniformBuffer::create(const DeviceRef& device, const Util::ResourceFormat& format) {
	Ref obj = new UniformBuffer;
	if(!obj->init(device, format)) {
		return nullptr;
	}
	return obj;
}

//---------------

UniformBuffer::UniformBuffer() = default;

//---------------

UniformBuffer::~UniformBuffer() = default;

//---------------

const Util::ResourceFormat& UniformBuffer::getFormat() const {
	return accessor->getFormat();
}

//---------------

bool UniformBuffer::init(const DeviceRef& device, const Util::ResourceFormat& format) {
	buffer = BufferObject::create(device);

	return true;
}

//---------------

} /* Rendering */