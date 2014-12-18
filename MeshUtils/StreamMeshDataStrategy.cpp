/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "StreamMeshDataStrategy.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/BufferObject.h>
#include <Rendering/GLHeader.h>
#include <Rendering/Helper.h>

#include <Util/Macros.h>

namespace Rendering {

StreamMeshDataStrategy::StreamMeshDataStrategy() : vertexStreamEnd(0), vertexStreamStart(0) { }

StreamMeshDataStrategy::~StreamMeshDataStrategy() = default;

//! ---|> MeshDataStrategy
void StreamMeshDataStrategy::assureLocalVertexData(Mesh* m) {
	MeshVertexData & vd=m->_getVertexData();

	if( vd.dataSize()==0 && vd.isUploaded())
		vd.download();
}

//! ---|> MeshDataStrategy
void StreamMeshDataStrategy::assureLocalIndexData(Mesh* m) {
	MeshIndexData & id=m->_getIndexData();

	if( id.dataSize()==0 && id.isUploaded())
		id.download();
}

//! ---|> MeshDataStrategy
void StreamMeshDataStrategy::prepare(Mesh* m) {

	if(m->isUsingIndexData()) {
		WARN("Streaming of meshes with index data is not supported yet.");
	}

	MeshVertexData & vd=m->_getVertexData();
	if( vd.empty() && vd.isUploaded() ){ // "old" VBO present, although data has been removed
		vd.removeGlBuffer();
	} else if( !vd.empty()) {
		if(!vd.isUploaded()){ // data has changed or is new
			BufferObject bo;
			bo.allocateData<uint8_t>(GL_ARRAY_BUFFER, vd.dataSize(), GL_STREAM_DRAW);
			vd._swapBufferObject(bo);
		}
		if(vertexStreamEnd > vertexStreamStart ){ // new data is available
			upload(m);
		}
	}
}

//! ---|> MeshDataStrategy
void StreamMeshDataStrategy::displayMesh(RenderingContext& context, Mesh* m, uint32_t startIndex, uint32_t indexCount) {
	if( !m->empty() && vertexStreamStart > 0 && startIndex < vertexStreamStart)
		MeshDataStrategy::doDisplayMesh(context,m,startIndex,std::min(indexCount, vertexStreamStart-startIndex-1));
}

void StreamMeshDataStrategy::upload(Mesh * m) {
	MeshVertexData & vd=m->_getVertexData();
	size_t vertexSize = vd.getVertexDescription().getVertexSize();

#if defined(LIB_GL)
	if(GL_ARB_map_buffer_range) {
		BufferObject bo;
		vd._swapBufferObject(bo);
		bo.bind(GL_ARRAY_BUFFER);

		uint8_t* data = reinterpret_cast<uint8_t*>(glMapBufferRange(
				GL_ARRAY_BUFFER,
				vertexStreamStart*vertexSize,
				(vertexStreamEnd-vertexStreamStart)*vertexSize,
				GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT));

		if(data == nullptr) {
			WARN("Failed to map buffer");
			FAIL();
		}

		const uint8_t* begin = vd.data() + vertexStreamStart*vertexSize;
		const uint8_t* end = vd.data() + std::min(vertexStreamEnd*vertexSize, vd.dataSize());
		std::copy(begin, end, data);

		glUnmapBuffer(GL_ARRAY_BUFFER);

		vertexStreamStart = vertexStreamEnd;

		bo.unbind(GL_ARRAY_BUFFER);
		vd._swapBufferObject(bo);
	} else {
		WARN("glMapBufferRange is not supported.");
	}
#else
	WARN("glMapBufferRange is not supported.");
#endif
}

} /* namespace Rendering */
