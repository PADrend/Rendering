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
class Device;
using DeviceRef = Util::Reference<Device>;

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
	const DeviceRef device;
	std::vector<uint8_t> binaryData;
	const VertexDescription * vertexDescription;
	uint32_t vertexCount;
	BufferObject::Ref bufferObject;

	Geometry::Box bb;
	bool dataChanged;

	/*! (internal) To save memory, the vertexDescription is stored in a static set
		so that each MeshVertexData-Object having the same vertex description references the same
		VertexDescription object. */
	void setVertexDescription(const VertexDescription & vd);
public:

	// main
	MeshVertexData();
	MeshVertexData(const DeviceRef& device);
	/*! Copy all data from @p other.
		\note If the other data is only available in the graphics card memory, this may
			only be called from within the gl-thread.	*/
	MeshVertexData(const MeshVertexData & other);
	MeshVertexData(MeshVertexData &&);

	~MeshVertexData();

	MeshVertexData & operator=(const MeshVertexData &) = delete;
	MeshVertexData & operator=(MeshVertexData &&) = default;

	const VertexDescription & getVertexDescription() const { return *vertexDescription; }
	uint32_t getVertexCount() const { return vertexCount; }
	bool empty() const { return vertexCount==0; }
	void swap(MeshVertexData & other);

	// data
	/*! Set the local vertex data. The old data is freed.
		\note Sets dataChanged. */
	void allocate(uint32_t count, const VertexDescription & vd);
	void releaseLocalData();
	void markAsChanged() { dataChanged=true; }
	bool hasChanged() const { return dataChanged; }
	bool hasLocalData() const { return !binaryData.empty(); }
	const uint8_t * data() const { return binaryData.data(); }
	uint8_t * data() { return binaryData.data(); }
	size_t dataSize() const { return binaryData.size(); }
	const uint8_t * operator[](uint32_t index) const;
	uint8_t * operator[](uint32_t index);

	// bounding box
	void updateBoundingBox();
	const Geometry::Box & getBoundingBox() const { return bb; }

	/**
	 * Set a new bounding box.
	 *
	 * @note This function should not be used normally. It is needed in special situations when there is no vertex data but the bounding box is known.
	 * @param box New bounding box.
	 */
	void _setBoundingBox(const Geometry::Box & box) { bb = box; }


	// vbo
	inline bool isUploaded() const { return bufferObject->isValid(); }

	//! Call @a upload() with default usage hint.
	bool upload() { return upload(MemoryUsage::GpuOnly); }
	/*! (internal) Create or update a VBO if hasChanged is set to true.
		hasChanged is set to false.	*/
	bool upload(MemoryUsage usage);
	
	/*! (internal) */
	bool download();
	void downloadTo(std::vector<uint8_t> & destination) const;

	//! @name Deprecated
	//! @{
	[[deprecated]]
	void bind(RenderingContext & context, bool useVBO);
	[[deprecated]]
	void unbind(RenderingContext & context, bool useVBO);
	[[deprecated]]
	bool upload(uint32_t usageHint) { return upload(); }
	[[deprecated]]
	void removeGlBuffer() { bufferObject->destroy(); }
	[[deprecated]]
	void drawArray(RenderingContext & context,bool useVBO,uint32_t drawMode,uint32_t startIndex,uint32_t numberOfElements);
	[[deprecated]]
	void _swapBufferObject(BufferObject & other) { bufferObject->swap(other); }
	[[deprecated]]
	BufferObject& _getBufferObject() { return *bufferObject.get(); }
	//! @}
};


}
#endif // MeshVertexData_H
