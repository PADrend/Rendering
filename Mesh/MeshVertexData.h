/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MeshVertexData_H
#define MeshVertexData_H

#include "../BufferObject.h"
#include <Geometry/Box.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Rendering {
class RenderingContext;
class VertexDescription;

/*! VertexData-Class.
	Part of the Mesh implementation containing all vertex specific data of a mesh:
	- VertexDescription: Data format of the vertices.
	- The local storage for the vertex data (If the data is uploaded to
		the graphics card, the local copy may be freed.)
	- The vertex buffer id, if the data has been uploaded to graphics memory.
	- A bounding box enclosing all vertices.
	@ingroup mesh
*/
class MeshVertexData {
	std::vector<uint8_t> binaryData;
	const VertexDescription * vertexDescription;
	uint32_t vertexCount;
	BufferObject::Ref bufferObject;

	Geometry::Box bb;
	bool dataChanged;

	/*! (internal) To save memory, the vertexDescription is stored in a static set
		so that each MeshVertexData-Object having the same vertex description references the same
		VertexDescription object. */
	RENDERINGAPI void setVertexDescription(const VertexDescription & vd);
public:

	// main
	RENDERINGAPI MeshVertexData();
	/*! Copy all data from @p other.
		\note If the other data is only available in the graphics card memory, this may
			only be called from within the gl-thread.	*/
	RENDERINGAPI MeshVertexData(const MeshVertexData & other);
	RENDERINGAPI MeshVertexData(MeshVertexData &&);

	RENDERINGAPI ~MeshVertexData();

	MeshVertexData & operator=(const MeshVertexData &) = delete;
	MeshVertexData & operator=(MeshVertexData &&) = default;

	const VertexDescription & getVertexDescription() const { return *vertexDescription; }
	uint32_t getVertexCount() const { return vertexCount; }
	bool empty() const { return vertexCount==0; }
	RENDERINGAPI void swap(MeshVertexData & other);

	// data
	/*! Set the local vertex data. The old data is freed.
		\note Sets dataChanged. */
	RENDERINGAPI void allocate(uint32_t count, const VertexDescription & vd);
	RENDERINGAPI void releaseLocalData();
	void markAsChanged() { dataChanged=true; }
	bool hasChanged() const { return dataChanged; }
	bool hasLocalData() const { return !binaryData.empty(); }
	const uint8_t * data() const { return binaryData.data(); }
	uint8_t * data() { return binaryData.data(); }
	size_t dataSize() const { return binaryData.size(); }
	RENDERINGAPI const uint8_t * operator[](uint32_t index) const;
	RENDERINGAPI uint8_t * operator[](uint32_t index);

	// bounding box
	RENDERINGAPI void updateBoundingBox();
	const Geometry::Box & getBoundingBox() const { return bb; }

	//! @name Internal
	//! @{

	/**
	 * Set a new bounding box.
	 *
	 * @note This function should not be used normally. It is needed in special situations when there is no vertex data but the bounding box is known.
	 * @param box New bounding box.
	 */
	void _setBoundingBox(const Geometry::Box & box) { bb = box; }


	// vbo
	inline bool isUploaded() const { return bufferObject && bufferObject->isValid(); }

	//! Call @a upload() with default usage hint.
	bool upload() { return upload(MemoryUsage::GpuOnly); }
	//! (internal) Create or update a VBO if hasChanged is set to true. hasChanged is set to false.
	RENDERINGAPI bool upload(MemoryUsage usage);
	
	RENDERINGAPI bool download();
	RENDERINGAPI void downloadTo(std::vector<uint8_t> & destination) const;

	RENDERINGAPI void bind(RenderingContext & context);
	RENDERINGAPI void draw(RenderingContext & context, uint32_t startIndex, uint32_t numberOfElements);
	void release() { bufferObject->destroy(); }

	const BufferObject::Ref& getBuffer() { return bufferObject; }
	RENDERINGAPI void setDebugName(const std::string& name);
	//! @}

	//! @name Deprecated
	//! @{
	[[deprecated]]
	void bind(RenderingContext & context, bool useVBO) {
		bind(context);
	}
	[[deprecated]]
	void unbind(RenderingContext & context, bool useVBO) {}
	[[deprecated]]
	bool upload(uint32_t usageHint) { return upload(); }
	[[deprecated]]
	void removeGlBuffer() { release(); }
	[[deprecated]]
	void drawArray(RenderingContext & context,bool useVBO,uint32_t drawMode,uint32_t startIndex,uint32_t numberOfElements) {
		draw(context, startIndex, numberOfElements);
	}
	[[deprecated]]
	void _swapBufferObject(BufferObject & other) { bufferObject->swap(other); }
	[[deprecated]]
	BufferObject& _getBufferObject() { return *bufferObject.get(); }
	//! @}
};


}
#endif // MeshVertexData_H
