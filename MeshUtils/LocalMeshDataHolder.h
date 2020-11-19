/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_MESHUTILS_LOCALMESHDATAHOLDER_H
#define RENDERING_MESHUTILS_LOCALMESHDATAHOLDER_H

#include <Util/References.h>

namespace Rendering {
class Mesh;
class MeshDataStrategy;
namespace MeshUtils {

/**
 * @brief Class to ensure that the data of a mesh stays in local memory
 * 
 * An object of this class watches over a mesh. When an object of this class is
 * created, it asks the mesh's data strategy to store the mesh data in main
 * memory (e.g. download it from graphics memory). Then, it changes the data
 * strategy to ensure that the data is not removed later on. When the object is
 * destroyed, it restores the original data strategy on the mesh.
 * 
 * @author Benjamin Eikel
 * @date 2013-03-01
 */
class LocalMeshDataHolder {
	private:
		Util::Reference<Mesh> mesh;
		MeshDataStrategy * originalDataStrategy;

	public:
		/**
		 * Ensure that the data of a mesh is and stays in local memory.
		 * 
		 * @param meshToHold Mesh that is taken care of
		 */
		RENDERINGAPI LocalMeshDataHolder(Mesh * meshToHold);

		//! Restore the original data strategy on the associated mesh.
		RENDERINGAPI ~LocalMeshDataHolder();
};

}
}

#endif /* RENDERING_MESHUTILS_LOCALMESHDATAHOLDER_H */
