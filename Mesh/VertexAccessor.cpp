/*
	This file is part of the Rendering library.
	Copyright (C) 2018-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VertexAccessor.h"
#include "Mesh.h"
#include "../Helper.h"

#include <Util/Macros.h>

#include <iostream>

namespace Rendering {

VertexAccessor::VertexAccessor(MeshVertexData& _vData, uint8_t* ptr) : 
	Util::ResourceAccessor(ptr, 
		_vData.getVertexCount() * _vData.getVertexDescription().getVertexSize(), 
		_vData.getVertexDescription()
	), vData(_vData) { }

VertexAccessor::~VertexAccessor() {
	if(vData.isUploaded())
		vData._getBufferObject().unmap();
}

Util::Reference<VertexAccessor> VertexAccessor::create(MeshVertexData& vData) {
	uint8_t* ptr = vData.isUploaded() ? vData._getBufferObject().map() : vData.data();
	if(!ptr) {
		WARN("VertexAccessor: could not map vertex data.");
		return nullptr;
	}
	return new VertexAccessor(vData, ptr);
}

Util::Reference<VertexAccessor> VertexAccessor::create(Mesh* mesh) {
	return create(mesh->_getVertexData());
}

} /* Rendering */