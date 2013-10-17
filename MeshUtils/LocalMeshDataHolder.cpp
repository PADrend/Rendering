/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "LocalMeshDataHolder.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/MeshDataStrategy.h"
#include <Util/Macros.h>

namespace Rendering {
namespace MeshUtils {

LocalMeshDataHolder::LocalMeshDataHolder(Mesh * meshToHold) :
		mesh(meshToHold),
		originalDataStrategy(mesh->getDataStrategy()) {
	originalDataStrategy->assureLocalIndexData(mesh.get());
	originalDataStrategy->assureLocalVertexData(mesh.get());
	mesh->setDataStrategy(SimpleMeshDataStrategy::getStaticDrawPreserveLocalStrategy());
}

LocalMeshDataHolder::~LocalMeshDataHolder() {
	if(mesh->getDataStrategy() != SimpleMeshDataStrategy::getStaticDrawPreserveLocalStrategy()) {
		WARN("The data strategy of the mesh has changed while the mesh was hold. "
			 "The original strategy will not be restored.");
	} else {
		mesh->setDataStrategy(originalDataStrategy);
	}
}

}
}
