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
#ifndef MESHDATASTRATEGY_H
#define MESHDATASTRATEGY_H

#include <cstdint>

namespace Rendering{

class Mesh;
class RenderingContext;

/*! Determines the strategy how the index and vertex data of a mesh is
	handled (uploaded, downloaded, deletet, ...).
	Different simple behaviors can be realized with this class directly, other strategies
	(e.g. with a more sophisticated memory management) can use this class as a base.

	\note All instances of this class should be created only once and re-used (they are probably never deleted)
	\note If a mesh does not have a data-strategy, the default strategy is used.
	\note If an implementation does make use of gl-calls, be carefull if the
		mesh is accesed from a non-gl-thread. 
	@ingroup mesh
*/
class MeshDataStrategy {
		static MeshDataStrategy * defaultStrategy;
	public:
		/*! Returns an instance of the default strategy as singleton. */
		static MeshDataStrategy * getDefaultStrategy();
		static void setDefaultStrategy(MeshDataStrategy * newDefault);

		// --------------------------------------

	public:
		MeshDataStrategy(){}
		virtual ~MeshDataStrategy(){}

		/*! If the Mesh has vertex data, assure that it can be accessed locally
			(e.g. by downloading it from graphics memory)
			---o */
		virtual void assureLocalVertexData(Mesh * m)=0;

		/*! If the Mesh has index data, assure that it can be accessed locally
			(e.g. by downloading it from graphics memory)
			---o */
		virtual void assureLocalIndexData(Mesh * m)=0;

		/*! Prepare the Mesh for display (VBO creation, etc.)
			---o	*/
		virtual void prepare(Mesh * m)=0;

		/*! Display the mesh as VBO or VertexArray.
			---o	*/
		virtual void displayMesh(RenderingContext & context, Mesh * m,uint32_t firstElement,uint32_t elementCount)=0;
		
	protected:
		//! (internal) Actually bind the buffers and render the mesh.
		static void doDisplayMesh(RenderingContext & context, Mesh * m,uint32_t firstElement,uint32_t elementCount);

};

// -------------------------------------------------------------------

/*!	SimpleMeshDataStrategy ---|> MeshDataStrategy
	@ingroup mesh
*/
class SimpleMeshDataStrategy : public MeshDataStrategy {
	public:
		/*!	Return an instance of the SimpleMeshDataStrategy:
			Create a VBO (with static usage) when first	displayed and release the local memory.
			\note This is the initial default strategy. */
		static SimpleMeshDataStrategy * getStaticDrawReleaseLocalStrategy();

		/*!	Return an instance of the SimpleMeshDataStrategy:
			Create a VBO (with static usage) when first	displayed and release the local memory.
			\note Each action results in an output message. */
		static SimpleMeshDataStrategy * getDebugStrategy();

		/*!	Return an instance of the SimpleMeshDataStrategy:
			Create a VBO (with static usage) when first	displayed and to preserve a copy in local memory. */
		static SimpleMeshDataStrategy * getStaticDrawPreserveLocalStrategy();

		/*!	Return an instance of the SimpleMeshDataStrategy:
			Create a VBO (with dynamic usage) when first displayed and to preserve a copy in local memory. */
		static SimpleMeshDataStrategy * getDynamicVertexStrategy();

		/*!	Return an instance of the SimpleMeshDataStrategy:
			Use VertexArrays and render from local memory. */
		static SimpleMeshDataStrategy * getPureLocalStrategy();

		// --------------------

		static const uint8_t USE_VBOS = 1<<0;
		static const uint8_t PRESERVE_LOCAL_DATA = 1<<1;
		static const uint8_t DYNAMIC_VERTICES = 1<<2;
		static const uint8_t DEBUG_OUTPUT = 1<<3;

		const uint8_t flags;
		inline bool getFlag(const uint8_t f)const	{	return flags&f;	}

		SimpleMeshDataStrategy( const uint8_t flags );
		virtual ~SimpleMeshDataStrategy();

		void assureLocalVertexData(Mesh * m) override;
		void assureLocalIndexData(Mesh * m) override;
		void prepare(Mesh * m) override;
		void displayMesh(RenderingContext & context, Mesh * m,uint32_t startIndex,uint32_t indexCount) override;
};

}

#endif // MESHDATASTRATEGY_H
