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
#ifndef RENDERING_MESH_MESH_H_
#define RENDERING_MESH_MESH_H_

#include "../State/PipelineState.h"
#include "MeshIndexData.h"
#include "MeshVertexData.h"
#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>
#include <Util/IO/FileName.h>
#include <cstddef>
#include <cstdint>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}

namespace Rendering{

class MeshDataStrategy;
class VertexDescription;
class RenderingContext;

//! @addtogroup rendering_resources
//! @{
//! @defgroup mesh Mesh
//! @}

/**
 * Class for polygonal meshes.
 * A mesh consisting of four components:
 * - MeshVertexData: The vertices of the mesh stored in local and/or graphics memory
 * - MeshIndexData: The indices of the used vertices stored in local and/or graphics memory
 * - DataStrategy: A strategy that determines where the data is stored and how the
 * 	mesh is rendered (e.g. as VBO or VertexArray)
 * - Filename: (optional) The filename from which the mesh was loaded.
 *
 * Create a new mesh:
 * \code
 * Mesh * mesh = new Mesh;                         // create a mesh
 *
 * MeshIndexData & id = mesh->openMeshIndexData();   // get access to index data
 * id.allocate(numberOfIndices);                     // allocate memory for indices (using triangles)
 * for(uint32_t i = 0; i < numberOfIndices; ++i) {
 *     id[i] = someIndex(...);                       // access the indices
 * }
 * id.updateIndexRange();                            // recalculate index range
 *
 * MeshVertexData & vd = mesh->openMeshVertexData(); // get access to vertex data
 * VertexDescription desc;                           // create a vertexDescription
 * vd.allocate(numberOfVertices);                    // allocate memory for vertices
 * uint8_t * binaryMeshVertexData = vd.data();       // access the vertices
 * createMeshVertexData(...);
 * vd.updateBoundingBox();                           // recalculate bounding box
 * \endcode
 * \note After an existing mesh has been changed, vd.markAsChanged() and id.markAsChanged() have
 * to be called so that the VBO can be updated. After allocate(...) this is not necessary.
 * @ingroup mesh
 */
class Mesh : public Util::ReferenceCounter<Mesh> {
	PROVIDES_TYPE_NAME_NV(Mesh)

	/*!	@name Main */
	// @{
	public:
		using Ref = Util::Reference<Mesh>;
		Mesh();
		Mesh(MeshIndexData meshIndexData, MeshVertexData meshVertexData);
		Mesh(const VertexDescription & desc,uint32_t vertexCount,uint32_t indexCount);
		Mesh(const Mesh &) = default;
		Mesh(Mesh &&) = default;
		~Mesh();

		Mesh* clone() const;

		void swap(Mesh & m);

		/**
		 * Return the amount of main memory currently occupied by this mesh.
		 *
		 * @note If the mesh data is currently not present in main memory, only a small number is returned (probably @c sizeof(Mesh)).
		 * @return Amount of memory in bytes
		 */
		size_t getMainMemoryUsage() const;

		/**
		 * Return the amount of graphics memory currently occupied by this mesh.
		 *
		 * @note If the mesh data is currently not uploaded to the graphics card, zero is returned.
		 * @return Amount of memory in bytes
		 */
		size_t getGraphicsMemoryUsage() const;

		/*! Returns true if no data is set. */
		bool empty() const { return useIndexData ? (vertexData.empty() || indexData.empty()) : vertexData.empty(); }

		/*! Display the mesh as VBO or VertexArray (determined by current data strategy).
			- If the mesh uses indices (isUsingIndexData()==true), @p firstElement and @p elementCount are the first 
			index and the number of indices to be drawn.
			- If the mesh does not us indices (isUsingIndexData()==false), @p firstElement and @p elementCount are the first 
			vertex and the number of vertices to be drawn.
			Calls:  
				--> dataStrategy->displayMesh(...) 
				--> dataStrategy::doDisplayMesh(...) 
				--> vertexData.bind() & indexData.drawElements(...) OR (if no indexData is present) vertexData.drawArray(...)
			\note **Attention** The function has to be called from within the GL-thread!	
			\note Except if you know what you are doing, use renderingContext.displayMesh(mesh) instead. */
		void _display(RenderingContext & context,uint32_t firstElement,uint32_t elementCount);

		/**
		 * Return the number of primitives stored in this mesh. The number
		 * depends on the number of indices, the number of vertices, and the
		 * draw mode. To retrieve the type of primitives, call getDrawMode().
		 * 
		 * @param numElements If zero, the number of indices or the number of
		 * vertices will be used. If non-zero, use the number of elements to
		 * do the calculation.
		 */
		uint32_t getPrimitiveCount(uint32_t numElements = 0) const;

	private:
		void operator=(const Mesh &) = delete;
		void operator=(Mesh &&) = delete;
	// @}

	/*!	@name MeshIndexData */
	// @{
	public:
		/*! Returns a reference to the indexData member.
			\note In most cases: openIndexData() is what you want (that why the _ is in the name...)*/
		MeshIndexData & _getIndexData() { return indexData; }
		const MeshIndexData & _getIndexData() const { return indexData; }

		/*! Returns a reference to the indexData member and assures that if the mesh contains
			index data, this data can be accessed via MeshIndexData.data()	*/
		MeshIndexData & openIndexData();

		uint32_t getIndexCount() const {
			return useIndexData ? indexData.getIndexCount() : 0;
		}

		//! If useIndexData is false, the mesh's indexData
		bool isUsingIndexData() const { return useIndexData; }
		void setUseIndexData(const bool b) { useIndexData  = b; }

	private:
		MeshIndexData indexData;
	// @}

	/*!	@name Filename */
	// @{
	public:
		const Util::FileName & getFileName() const { return fileName; }
		void setFileName(const Util::FileName & f) { fileName=f; }

	private:
		Util::FileName fileName;
	// @}

	/*!	@name MeshVertexData */
	// @{
	public:
		/*! Returns a reference to the vertexData member.
			\note In most cases: openVertexData() is what you want (that why the _ is in the name...)	*/
		MeshVertexData & _getVertexData() { return vertexData; }
		const MeshVertexData & _getVertexData() const { return vertexData; }

		/*! Returns a reference to the vertexData member and assures that if the mesh contains
			vertex data, this data can be accessed via MeshVertexData.data()	*/
		MeshVertexData & openVertexData();

		uint32_t getVertexCount() const { return vertexData.getVertexCount(); }
		const VertexDescription & getVertexDescription() const { return vertexData.getVertexDescription(); }
		const Geometry::Box & getBoundingBox() const { return vertexData.getBoundingBox(); }

	private:
		MeshVertexData vertexData;
	// @}

	/*!	@name DataStrategy */
	// @{
	public:
		//! Return the current data strategy.
		MeshDataStrategy * getDataStrategy() const {
			return dataStrategy;
		}

		//! Set a new data strategy.
		void setDataStrategy(MeshDataStrategy * newStrategy);

	private:
		MeshDataStrategy * dataStrategy;
	// @}



	/*!	@name Topology */
	// @{
	public:
		enum draw_mode_t : uint8_t {
			DRAW_POINTS,
			DRAW_LINE_STRIP,
			DRAW_LINE_LOOP,
			DRAW_LINES,
			DRAW_TRIANGLES
		};
	private:
		PrimitiveTopology topology;
	public:
		PrimitiveTopology getTopology() const { return topology; }
		void setTopology(PrimitiveTopology value) { topology = value; }

		[[deprecated]]
		draw_mode_t getDrawMode() const;
		[[deprecated]]
		void setDrawMode(draw_mode_t newMode);
		[[deprecated]]
		uint32_t getGLDrawMode() const { return 0; }
		[[deprecated]]
		void setGLDrawMode(uint32_t glDrawMode) {}
	// @}
		
	private:
		bool useIndexData; //! (located at this position to save memory due to padding)

};

}
#endif // RENDERING_MESH_MESH_H_
