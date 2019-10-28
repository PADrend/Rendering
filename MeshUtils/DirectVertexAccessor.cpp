/*
	This file is part of the Rendering library.
	Copyright (C) 2018-2019 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "DirectVertexAccessor.h"
#include "../Mesh/Mesh.h"
#include "../Helper.h"

#include <Util/Macros.h>

#include <iostream>

namespace Rendering {
namespace MeshUtils {

static Util::ResourceFormat convert(const VertexDescription& vd) {
	Util::ResourceFormat format;
	for(const auto& attr : vd.getAttributes()) {
		format.appendAttribute(attr.getNameId(), getAttributeType(attr.getDataType()), attr.getNumValues(), attr.getNormalize());
	}
	return format;
}

DirectVertexAccessor::DirectVertexAccessor(MeshVertexData& _vData, uint8_t* ptr) : Util::ResourceAccessor(ptr, _vData.dataSize(), convert(_vData.getVertexDescription())), vData(_vData) { }

DirectVertexAccessor::~DirectVertexAccessor() {
	if(vData.isUploaded())
		vData._getBufferObject().unmap();
}

Util::Reference<DirectVertexAccessor> DirectVertexAccessor::create(MeshVertexData& vData) {
	uint8_t* ptr = vData.isUploaded() ? vData._getBufferObject().map() : vData.data();
	if(!ptr) {
		WARN("DirectVertexAccessor: could not map vertex data.");
		return nullptr;
	}
	return new DirectVertexAccessor(vData, ptr);
}

Util::Reference<DirectVertexAccessor> DirectVertexAccessor::create(Mesh* mesh) {
	return create(mesh->_getVertexData());
}

	
} /* MeshUtils */
} /* Rendering */