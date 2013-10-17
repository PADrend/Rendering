/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "PlatonicSolids.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/MeshIndexData.h"
#include "../Mesh/MeshVertexData.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include "../GLHeader.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <map>
#include <utility>

namespace Rendering {
namespace MeshUtils {
namespace PlatonicSolids {

// Vertex positions and face connectivity are taken from http://geometrictools.com/Documentation/PlatonicSolids.pdf.

Mesh * createTetrahedron() {
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, 4, 3 * 4);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *>(vd.data());

	// sqrt(2) / 3
	const float sqrtTwoOT = 0.47140452079103168293f;
	// sqrt(6) / 3
	const float sqrtSixOT = 0.81649658092772603273f;
	const float oneThird = 0.33333333333333333333f;

	// Because we have unit lengths here, normals equal positions.
	*v++ = 0.0f; *v++ = 0.0f; *v++ = 1.0f;
	*v++ = 0.0f; *v++ = 0.0f; *v++ = 1.0f;
	*v++ = 2.0f * sqrtTwoOT; *v++ = 0.0f; *v++ = -oneThird;
	*v++ = 2.0f * sqrtTwoOT; *v++ = 0.0f; *v++ = -oneThird;
	*v++ = -sqrtTwoOT; *v++ = sqrtSixOT; *v++ = -oneThird;
	*v++ = -sqrtTwoOT; *v++ = sqrtSixOT; *v++ = -oneThird;
	*v++ = -sqrtTwoOT; *v++ = -sqrtSixOT; *v++ = -oneThird;
	*v++ = -sqrtTwoOT; *v++ = -sqrtSixOT; *v++ = -oneThird;

	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();

	*i++ = 0; *i++ = 1; *i++ = 2;
	*i++ = 0; *i++ = 2; *i++ = 3;
	*i++ = 0; *i++ = 3; *i++ = 1;
	*i++ = 1; *i++ = 3; *i++ = 2;

	id.updateIndexRange();
	return mesh;
}


Mesh * createCube() {
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, 8, 3 * 12);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *>(vd.data());

	// 1 / sqrt(3)
	const float oneOverSqrtThree = 0.57735026918962576450f;

	// Because we have unit lengths here, normals equal positions.
	*v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree;
	*v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree;

	*v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree;
	*v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree;

	*v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree;
	*v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree;

	*v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree;
	*v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree;

	*v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree;
	*v++ = -oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree;

	*v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree;
	*v++ =  oneOverSqrtThree; *v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree;

	*v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree;
	*v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree;

	*v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree;
	*v++ = -oneOverSqrtThree; *v++ =  oneOverSqrtThree; *v++ =  oneOverSqrtThree;

	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();

	*i++ = 0; *i++ = 3; *i++ = 1;
	*i++ = 3; *i++ = 2; *i++ = 1;

	*i++ = 0; *i++ = 1; *i++ = 4;
	*i++ = 1; *i++ = 5; *i++ = 4;

	*i++ = 0; *i++ = 4; *i++ = 3;
	*i++ = 4; *i++ = 7; *i++ = 3;

	*i++ = 6; *i++ = 5; *i++ = 2;
	*i++ = 5; *i++ = 1; *i++ = 2;

	*i++ = 6; *i++ = 2; *i++ = 7;
	*i++ = 2; *i++ = 3; *i++ = 7;

	*i++ = 6; *i++ = 7; *i++ = 5;
	*i++ = 7; *i++ = 4; *i++ = 5;

	id.updateIndexRange();
	return mesh;
}

Mesh * createOctahedron() {
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, 6, 3 * 8);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *>(vd.data());

	// Because we have unit lengths here, normals equal positions.
	*v++ = 1.0f; *v++ = 0.0f; *v++ = 0.0f;
	*v++ = 1.0f; *v++ = 0.0f; *v++ = 0.0f;
	*v++ = -1.0f; *v++ = 0.0f; *v++ = 0.0f;
	*v++ = -1.0f; *v++ = 0.0f; *v++ = 0.0f;
	*v++ = 0.0f; *v++ = 1.0f; *v++ = 0.0f;
	*v++ = 0.0f; *v++ = 1.0f; *v++ = 0.0f;
	*v++ = 0.0f; *v++ = -1.0f; *v++ = 0.0f;
	*v++ = 0.0f; *v++ = -1.0f; *v++ = 0.0f;
	*v++ = 0.0f; *v++ = 0.0f; *v++ = 1.0f;
	*v++ = 0.0f; *v++ = 0.0f; *v++ = 1.0f;
	*v++ = 0.0f; *v++ = 0.0f; *v++ = -1.0f;
	*v++ = 0.0f; *v++ = 0.0f; *v++ = -1.0f;

	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();

	*i++ = 4; *i++ = 0; *i++ = 2;
	*i++ = 4; *i++ = 2; *i++ = 1;
	*i++ = 4; *i++ = 1; *i++ = 3;
	*i++ = 4; *i++ = 3; *i++ = 0;
	*i++ = 5; *i++ = 2; *i++ = 0;
	*i++ = 5; *i++ = 1; *i++ = 2;
	*i++ = 5; *i++ = 3; *i++ = 1;
	*i++ = 5; *i++ = 0; *i++ = 3;

	id.updateIndexRange();
	return mesh;
}

Mesh * createDodecahedron() {
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, 20, 3 * 36);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *>(vd.data());

	// 1 / sqrt(3)
	const float a = 0.57735026918962576450f;
	// sqrt((3 - sqrt(5)) / 6)
	const float b = 0.35682208977308993194f;
	// sqrt((3 + sqrt(5)) / 6)
	const float c = 0.93417235896271569645f;

	// Because we have unit lengths here, normals equal positions.
	*v++ =  a; *v++ =  a; *v++ =  a;
	*v++ =  a; *v++ =  a; *v++ =  a;
	*v++ =  a; *v++ =  a; *v++ = -a;
	*v++ =  a; *v++ =  a; *v++ = -a;
	*v++ =  a; *v++ = -a; *v++ =  a;
	*v++ =  a; *v++ = -a; *v++ =  a;
	*v++ =  a; *v++ = -a; *v++ = -a;
	*v++ =  a; *v++ = -a; *v++ = -a;

	*v++ = -a; *v++ =  a; *v++ =  a;
	*v++ = -a; *v++ =  a; *v++ =  a;
	*v++ = -a; *v++ =  a; *v++ = -a;
	*v++ = -a; *v++ =  a; *v++ = -a;
	*v++ = -a; *v++ = -a; *v++ =  a;
	*v++ = -a; *v++ = -a; *v++ =  a;
	*v++ = -a; *v++ = -a; *v++ = -a;
	*v++ = -a; *v++ = -a; *v++ = -a;

	*v++ =  b; *v++ =  c; *v++ =  0;
	*v++ =  b; *v++ =  c; *v++ =  0;
	*v++ = -b; *v++ =  c; *v++ =  0;
	*v++ = -b; *v++ =  c; *v++ =  0;
	*v++ =  b; *v++ = -c; *v++ =  0;
	*v++ =  b; *v++ = -c; *v++ =  0;
	*v++ = -b; *v++ = -c; *v++ =  0;
	*v++ = -b; *v++ = -c; *v++ =  0;

	*v++ =  c; *v++ =  0; *v++ =  b;
	*v++ =  c; *v++ =  0; *v++ =  b;
	*v++ =  c; *v++ =  0; *v++ = -b;
	*v++ =  c; *v++ =  0; *v++ = -b;
	*v++ = -c; *v++ =  0; *v++ =  b;
	*v++ = -c; *v++ =  0; *v++ =  b;
	*v++ = -c; *v++ =  0; *v++ = -b;
	*v++ = -c; *v++ =  0; *v++ = -b;

	*v++ =  0; *v++ =  b; *v++ =  c;
	*v++ =  0; *v++ =  b; *v++ =  c;
	*v++ =  0; *v++ = -b; *v++ =  c;
	*v++ =  0; *v++ = -b; *v++ =  c;
	*v++ =  0; *v++ =  b; *v++ = -c;
	*v++ =  0; *v++ =  b; *v++ = -c;
	*v++ =  0; *v++ = -b; *v++ = -c;
	*v++ =  0; *v++ = -b; *v++ = -c;

	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();

	*i++ =  0; *i++ =  8; *i++ =  9;
	*i++ =  0; *i++ =  9; *i++ =  4;
	*i++ =  0; *i++ =  4; *i++ = 16;
	*i++ =  0; *i++ = 12; *i++ = 13;
	*i++ =  0; *i++ = 13; *i++ =  1;
	*i++ =  0; *i++ =  1; *i++ =  8;
	*i++ =  0; *i++ = 16; *i++ = 17;
	*i++ =  0; *i++ = 17; *i++ =  2;
	*i++ =  0; *i++ =  2; *i++ = 12;
	*i++ =  8; *i++ =  1; *i++ = 18;
	*i++ =  8; *i++ = 18; *i++ =  5;
	*i++ =  8; *i++ =  5; *i++ =  9;
	*i++ = 12; *i++ =  2; *i++ = 10;
	*i++ = 12; *i++ = 10; *i++ =  3;
	*i++ = 12; *i++ =  3; *i++ = 13;
	*i++ = 16; *i++ =  4; *i++ = 14;
	*i++ = 16; *i++ = 14; *i++ =  6;
	*i++ = 16; *i++ =  6; *i++ = 17;
	*i++ =  9; *i++ =  5; *i++ = 15;
	*i++ =  9; *i++ = 15; *i++ = 14;
	*i++ =  9; *i++ = 14; *i++ =  4;
	*i++ =  6; *i++ = 11; *i++ = 10;
	*i++ =  6; *i++ = 10; *i++ =  2;
	*i++ =  6; *i++ =  2; *i++ = 17;
	*i++ =  3; *i++ = 19; *i++ = 18;
	*i++ =  3; *i++ = 18; *i++ =  1;
	*i++ =  3; *i++ =  1; *i++ = 13;
	*i++ =  7; *i++ = 15; *i++ =  5;
	*i++ =  7; *i++ =  5; *i++ = 18;
	*i++ =  7; *i++ = 18; *i++ = 19;
	*i++ =  7; *i++ = 11; *i++ =  6;
	*i++ =  7; *i++ =  6; *i++ = 14;
	*i++ =  7; *i++ = 14; *i++ = 15;
	*i++ =  7; *i++ = 19; *i++ =  3;
	*i++ =  7; *i++ =  3; *i++ = 10;
	*i++ =  7; *i++ = 10; *i++ = 11;

	id.updateIndexRange();
	return mesh;
}

Mesh * createIcosahedron() {
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, 12, 3 * 20);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *> (vd.data());

	// Golden Ratio (1 + sqrt(5)) / 2
	//const float gr = 1.61803398874989484820f;
	// Length of the vectors sqrt(1 + gr^2)
	//const float length = 1.90211303259030714423f;
	// Normalized Golden Ratio (gr / length)
	const float grN = 0.85065080835203993218f;
	// Normalized One (1 / length)
	const float oneN = 0.52573111211913360602f;

	// Because we have unit lengths here, normals equal positions.
	*v++ = grN; *v++ = oneN; *v++ = 0.0f;
	*v++ = grN; *v++ = oneN; *v++ = 0.0f;
	*v++ = -grN; *v++ = oneN; *v++ = 0.0f;
	*v++ = -grN; *v++ = oneN; *v++ = 0.0f;
	*v++ = grN; *v++ = -oneN; *v++ = 0.0f;
	*v++ = grN; *v++ = -oneN; *v++ = 0.0f;
	*v++ = -grN; *v++ = -oneN; *v++ = 0.0f;
	*v++ = -grN; *v++ = -oneN; *v++ = 0.0f;

	*v++ = oneN; *v++ = 0.0f; *v++ = grN;
	*v++ = oneN; *v++ = 0.0f; *v++ = grN;
	*v++ = oneN; *v++ = 0.0f; *v++ = -grN;
	*v++ = oneN; *v++ = 0.0f; *v++ = -grN;
	*v++ = -oneN; *v++ = 0.0f; *v++ = grN;
	*v++ = -oneN; *v++ = 0.0f; *v++ = grN;
	*v++ = -oneN; *v++ = 0.0f; *v++ = -grN;
	*v++ = -oneN; *v++ = 0.0f; *v++ = -grN;

	*v++ = 0.0f; *v++ = grN; *v++ = oneN;
	*v++ = 0.0f; *v++ = grN; *v++ = oneN;
	*v++ = 0.0f; *v++ = -grN; *v++ = oneN;
	*v++ = 0.0f; *v++ = -grN; *v++ = oneN;
	*v++ = 0.0f; *v++ = grN; *v++ = -oneN;
	*v++ = 0.0f; *v++ = grN; *v++ = -oneN;
	*v++ = 0.0f; *v++ = -grN; *v++ = -oneN;
	*v++ = 0.0f; *v++ = -grN; *v++ = -oneN;

	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();

	*i++ = 0; *i++ = 8; *i++ = 4;
	*i++ = 1; *i++ = 10; *i++ = 7;
	*i++ = 2; *i++ = 9; *i++ = 11;
	*i++ = 7; *i++ = 3; *i++ = 1;

	*i++ = 0; *i++ = 5; *i++ = 10;
	*i++ = 3; *i++ = 9; *i++ = 6;
	*i++ = 3; *i++ = 11; *i++ = 9;
	*i++ = 8; *i++ = 6; *i++ = 4;

	*i++ = 2; *i++ = 4; *i++ = 9;
	*i++ = 3; *i++ = 7; *i++ = 11;
	*i++ = 4; *i++ = 2; *i++ = 0;
	*i++ = 9; *i++ = 4; *i++ = 6;

	*i++ = 2; *i++ = 11; *i++ = 5;
	*i++ = 0; *i++ = 10; *i++ = 8;
	*i++ = 5; *i++ = 0; *i++ = 2;
	*i++ = 10; *i++ = 5; *i++ = 7;

	*i++ = 1; *i++ = 6; *i++ = 8;
	*i++ = 1; *i++ = 8; *i++ = 10;
	*i++ = 6; *i++ = 1; *i++ = 3;
	*i++ = 11; *i++ = 7; *i++ = 5;

	id.updateIndexRange();
	return mesh;
}

Mesh * createEdgeSubdivisionSphere(Mesh * platonicSolid, uint8_t subdivisions) {
	if(platonicSolid == nullptr || platonicSolid->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		return nullptr;
	}
	auto mesh = new Mesh(*platonicSolid);
	for (uint_fast8_t s = 0; s < subdivisions; ++s) {
		const MeshVertexData & oldVd = mesh->openVertexData();
		const MeshIndexData & oldId = mesh->openIndexData();
		// Calculate the number of new vertices and faces.
		const uint32_t numVertices = oldVd.getVertexCount();
		const uint32_t numIndices = oldId.getIndexCount();
		const uint32_t numEdges = numIndices / 2; // numFaces * 1.5
		const uint32_t numNewVertices = numVertices + numEdges;
		const uint32_t numNewIndices = 4 * numIndices;

		Mesh subdividedMesh(mesh->getVertexDescription(), numNewVertices, numNewIndices);
		MeshVertexData & vd = subdividedMesh.openVertexData();
		MeshIndexData & id = subdividedMesh.openIndexData();

		float * vertices = reinterpret_cast<float *> (vd.data());
		uint32_t * indices = id.data();
		// Copy vertex data from old mesh into the new mesh.
		const float * const oldVertices = reinterpret_cast<const float * const> (oldVd.data());
		vertices = std::copy(oldVertices, oldVertices + 6 * numVertices, vertices);

		const uint32_t * const oldIndices = oldId.data();
		uint32_t nextIndex = oldId.getMaxIndex() + 1;
		// Mapping from an edge (key.first < key.second) to the new vertex on that edge.
		typedef std::pair<uint32_t, uint32_t> edge_t;
		typedef std::map<edge_t, uint32_t> edge_vertex_map_t;
		edge_vertex_map_t newVertexMap;
		for(uint_fast32_t i = 0; i < numIndices; i += 3) {
			/*
			 *        o[2]
			 *         X
			 *        / \
			 *       /   \
			 * n[2] x.....x n[1]
			 *     / `   ´ \
			 *    /   ` ´   \
			 *   X-----x-----X
			 *  o[0]  n[0]    o[1]
			 *
			 *  Triangle (o[0], o[1], o[2]) is subdivided into (o[0], n[0], n[2]), (n[0], o[1], n[1]), (n[0], n[1], n[2]), and (n[2], n[1], o[2]).
			 */
			// Subdivide one triangle consisting of the vertices with indices o[0], o[1], o[2].
			uint32_t o[3];
			o[0] = oldIndices[i + 0];
			o[1] = oldIndices[i + 1];
			o[2] = oldIndices[i + 2];

			uint32_t n[3];
			for(uint_fast8_t v = 0; v < 3; ++v) {
				const uint32_t a = o[v];
				const uint32_t b = o[(v + 1) % 3];
				const auto extremes = std::minmax(a, b);
				edge_t edge(extremes.first, extremes.second);
				auto lb = newVertexMap.lower_bound(edge);
				if(lb != newVertexMap.end() && !(newVertexMap.key_comp()(edge, lb->first))) {
					// Edge already in map.
					n[v] = lb->second;
				} else {
					// Create new vertex with index d.
					n[v] = nextIndex++;
					newVertexMap.insert(lb, edge_vertex_map_t::value_type(edge, n[v]));

					const float * const vertexA = oldVertices + 6 * a;
					const float * const vertexB = oldVertices + 6 * b;
					// Position
					vertices[0] = 0.5f * (vertexA[0] + vertexB[0]);
					vertices[1] = 0.5f * (vertexA[1] + vertexB[1]);
					vertices[2] = 0.5f * (vertexA[2] + vertexB[2]);
					const float length = std::sqrt(vertices[0] * vertices[0] + vertices[1] * vertices[1] + vertices[2] * vertices[2]);
					vertices[0] /= length;
					vertices[1] /= length;
					vertices[2] /= length;
					// Normal
					vertices[3] = vertices[0];
					vertices[4] = vertices[1];
					vertices[5] = vertices[2];
					vertices += 6;
				}
			}

			*indices++ = o[0]; *indices++ = n[0]; *indices++ = n[2];
			*indices++ = n[0]; *indices++ = o[1]; *indices++ = n[1];
			*indices++ = n[0]; *indices++ = n[1]; *indices++ = n[2];
			*indices++ = n[2]; *indices++ = n[1]; *indices++ = o[2];
		}

		// Update the data and copy it to the original mesh.
		vd.updateBoundingBox();
		id.updateIndexRange();
		mesh->swap(subdividedMesh);
	}
	return mesh;
}

}
}
}
