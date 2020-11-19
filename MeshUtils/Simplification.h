/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_MESHUTILS_SIMPLIFICATION_H
#define RENDERING_MESHUTILS_SIMPLIFICATION_H

#include <array>
#include <cstdint>

namespace Rendering {
class Mesh;
namespace MeshUtils {
namespace Simplification {

typedef std::array<float, 5> weights_t;
static const int VERTEX_OFFSET = 0;
static const int NORMAL_OFFSET = 1;
static const int COLOR_OFFSET = 2;
static const int TEX0_OFFSET = 3;
static const int BOUNDARY_OFFSET = 4;

/**
 * Simplify the given mesh to a total number of triangles given in
 * the parameters. This method will return a new mesh and leave the
 * original unchanged.
 * Hint: Vertex weight should contain normalization of vertex position (divide by BoundingBox.extendMax)
 *
 * @param mesh Mesh to be simplified
 * @param numberOfTriangles the number of polygons the returned mesh should have
 * @param threshold maximum distance of vertices not connected by an edge for pair selection
 * @param useOptimalPositioning enables/disables calculation of optimal positioning for vertices
 * @param maxAngle maximum angle a face may rotate per merge step (value is arccos of angle [-1, 1])
 * @param weights weights for all attributes using indices defined above
 * @return new simplified mesh, null if simplification failed
 * @author Jonas Knoll, Benjamin Eikel
 */
RENDERINGAPI Mesh * simplifyMesh(Mesh * mesh, 
					uint32_t numberOfTriangles, 
					float threshold, 
					bool useOptimalPositioning, 
					float maxAngle, 
					const weights_t & weights);

}
}
}

#endif // RENDERING_MESHUTILS_SIMPLIFICATION_H
