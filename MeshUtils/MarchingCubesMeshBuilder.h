/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MACRHING_CUBES_MESH_BUILDER_H_
#define MACRHING_CUBES_MESH_BUILDER_H_

#include <cstdint>
#include <vector>

namespace Util {
class PixelAccessor;
}
namespace Rendering {
class Mesh;
namespace MeshUtils {

/**
 * MarchingCubesMeshBuilder provides static functions for creating meshes from the specified
 * depth-texture, color-texture and normal-texture.
 *
 * Based on tables by Cory Gene Bloyd.
 * The source code is based on the examples on the site "Polygonising a scalar field" from Paul Bourke
 * \see http://paulbourke.net/geometry/polygonise/
 * @ingroup mesh_builder
 */
namespace MarchingCubesMeshBuilder {

struct DataSet{
	const uint32_t resolutionX,resolutionY,resolutionZ;
	const uint32_t layerXYSize;
	float isolevel;
	uint32_t rangeMinX,rangeMaxX,rangeMinY,rangeMaxY,rangeMinZ,rangeMaxZ;
	std::vector<float> density; //! resolutionX*resolutionY*resolutionZ many values.
	std::vector<float> occlusion;
	
	DataSet(const uint32_t rX,const uint32_t rY,const uint32_t rZ) : 
		resolutionX(rX),resolutionY(rY),resolutionZ(rZ),layerXYSize(rX*rY),
		isolevel(0.5),
		rangeMinX(0),rangeMaxX(rX),rangeMinY(0),rangeMaxY(rY),rangeMinZ(0),rangeMaxZ(rZ),
		density(rX*rY*rZ),occlusion(rX*rY*rZ)
		{}
	
};
	
Mesh * createMesh(DataSet & data);
Mesh * createMeshFromTiledImage(const Util::PixelAccessor & accessor, uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ);
}

}
}

#endif /* MACRHING_CUBES_MESH_BUILDER_H_ */
