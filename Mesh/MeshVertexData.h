/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
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
		BufferObject bufferObject;

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
		MeshVertexData(MeshVertexData &&) = default;

		~MeshVertexData() = default;

		MeshVertexData & operator=(const MeshVertexData &) = delete;
		MeshVertexData & operator=(MeshVertexData &&) = default;

		const VertexDescription & getVertexDescription()const 	{	return *vertexDescription;	}
		uint32_t getVertexCount()const							{	return vertexCount;	}
		bool empty()const										{	return vertexCount==0;	}
		RENDERINGAPI void swap(MeshVertexData & other);

		// data
		/*! Set the local vertex data. The old data is freed.
			\note Sets dataChanged. */
		RENDERINGAPI void allocate(uint32_t count, const VertexDescription & vd);
		RENDERINGAPI void releaseLocalData();
		void markAsChanged()								{  	dataChanged=true;	}
		bool hasChanged()const								{  	return dataChanged;	}
		bool hasLocalData()const							{  	return !binaryData.empty();	}
		const uint8_t * data()const							{	return binaryData.data();	}
		uint8_t * data()									{	return binaryData.data();	}
		size_t dataSize()const								{	return binaryData.size();	}
		RENDERINGAPI const uint8_t * operator[](uint32_t index) const;
		RENDERINGAPI uint8_t * operator[](uint32_t index);

		// bounding box
		RENDERINGAPI void updateBoundingBox();
		const Geometry::Box & getBoundingBox() const		{	return bb;	}
		/**
		 * Set a new bounding box.
		 *
		 * @note This function should not be used normally. It is needed in special situations when there is no vertex data but the bounding box is known.
		 * @param box New bounding box.
		 */
		void _setBoundingBox(const Geometry::Box & box)		{ bb = box; }


		// vbo
		inline bool isUploaded()const						{   return bufferObject.isValid();    }

		/*! (internal) */
		RENDERINGAPI void bind(RenderingContext & context, bool useVBO);
		/*! (internal) */
		RENDERINGAPI void unbind(RenderingContext & context, bool useVBO);

		//! Call @a upload() with default usage hint.
		RENDERINGAPI bool upload();
		/*! (internal) Create or update a VBO if hasChanged is set to true.
			hasChanged is set to false.	*/
		RENDERINGAPI bool upload(uint32_t usageHint);
		/*! (internal) */
		RENDERINGAPI bool download();
		RENDERINGAPI void downloadTo(std::vector<uint8_t> & destination)const;
		/*! (internal) */
		RENDERINGAPI void removeGlBuffer();
		
		/*! (internal) Draw the vertices using the VBO or a VertexArray.
			Used by MeshDataStrategy::doDisplay(..) if the mesh does not use indices. */
		RENDERINGAPI void drawArray(RenderingContext & context,bool useVBO,uint32_t drawMode,uint32_t startIndex,uint32_t numberOfElements);

		/*! Swap the internal BufferObject. 
			\note The local data is not changed!
			\note the size of the new buffer must be equal to that of the old one.
			\note Use only if you know what you are doing!	*/		
		void _swapBufferObject(BufferObject & other)	{	bufferObject.swap(other);	}
		
		/*! get the internal BufferObject.
		\note Use only if you know what you are doing!	*/		
		BufferObject& _getBufferObject() { return bufferObject; }
};


}
#endif // MeshVertexData_H
