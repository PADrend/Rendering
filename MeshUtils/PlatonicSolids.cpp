/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
  Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "PlatonicSolids.h"
#include "MeshBuilder.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/MeshIndexData.h"
#include "../Mesh/MeshVertexData.h"
#include "../Mesh/VertexDescription.h"
#include <Geometry/Vec3.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <utility>

namespace Rendering {
namespace MeshUtils {
namespace PlatonicSolids {
using namespace Geometry;

// Vertex positions and face connectivity are taken from http://geometrictools.com/Documentation/PlatonicSolids.pdf.

Mesh* createTetrahedron(const VertexDescription& vd) {
  MeshBuilder mb(vd);
  addTetrahedron(mb);
  return mb.buildMesh();
}

void addTetrahedron(MeshBuilder& mb) {
  uint32_t idx = mb.getNextIndex();
	// sqrt(2) / 3
	const float sqrtTwoOT = 0.47140452079103168293f;
	// sqrt(6) / 3
	const float sqrtSixOT = 0.81649658092772603273f;
	const float oneThird = 0.33333333333333333333f;

	// Because we have unit lengths here, normals equal positions.
	mb.position(Vec3{0.0f, 0.0f, 1.0f});
	mb.normal(Vec3{0.0f, 0.0f, 1.0f});
	mb.addVertex();
	mb.position(Vec3{2.0f * sqrtTwoOT, 0.0f, -oneThird});
	mb.normal(Vec3{2.0f * sqrtTwoOT, 0.0f, -oneThird});
	mb.addVertex();
	mb.position(Vec3{-sqrtTwoOT, sqrtSixOT, -oneThird});
	mb.normal(Vec3{-sqrtTwoOT, sqrtSixOT, -oneThird});
	mb.addVertex();
	mb.position(Vec3{-sqrtTwoOT, -sqrtSixOT, -oneThird});
	mb.normal(Vec3{-sqrtTwoOT, -sqrtSixOT, -oneThird});
	mb.addVertex();

	mb.addTriangle(idx + 0, idx + 1, idx + 2);
	mb.addTriangle(idx + 0, idx + 2, idx + 3);
	mb.addTriangle(idx + 0, idx + 3, idx + 1);
	mb.addTriangle(idx + 1, idx + 3, idx + 2);
}


Mesh* createCube(const VertexDescription& vd) {
  MeshBuilder mb(vd);
  addCube(mb);
  return mb.buildMesh();
}

void addCube(MeshBuilder& mb) {
  uint32_t idx = mb.getNextIndex();
	// 1 / sqrt(3)
	const float oneOverSqrtThree = 0.57735026918962576450f;

	// Because we have unit lengths here, normals equal positions.
	mb.position(Vec3{-oneOverSqrtThree, -oneOverSqrtThree, -oneOverSqrtThree});
	mb.normal(Vec3{-oneOverSqrtThree, -oneOverSqrtThree, -oneOverSqrtThree});
	mb.addVertex();

	mb.position(Vec3{ oneOverSqrtThree, -oneOverSqrtThree, -oneOverSqrtThree});
	mb.normal(Vec3{ oneOverSqrtThree, -oneOverSqrtThree, -oneOverSqrtThree});
	mb.addVertex();

	mb.position(Vec3{ oneOverSqrtThree,  oneOverSqrtThree, -oneOverSqrtThree});
	mb.normal(Vec3{ oneOverSqrtThree,  oneOverSqrtThree, -oneOverSqrtThree});
	mb.addVertex();

	mb.position(Vec3{-oneOverSqrtThree,  oneOverSqrtThree, -oneOverSqrtThree});
	mb.normal(Vec3{-oneOverSqrtThree,  oneOverSqrtThree, -oneOverSqrtThree});
	mb.addVertex();

	mb.position(Vec3{-oneOverSqrtThree, -oneOverSqrtThree,  oneOverSqrtThree});
	mb.normal(Vec3{-oneOverSqrtThree, -oneOverSqrtThree,  oneOverSqrtThree});
	mb.addVertex();

	mb.position(Vec3{ oneOverSqrtThree, -oneOverSqrtThree,  oneOverSqrtThree});
	mb.normal(Vec3{ oneOverSqrtThree, -oneOverSqrtThree,  oneOverSqrtThree});
	mb.addVertex();

	mb.position(Vec3{ oneOverSqrtThree,  oneOverSqrtThree,  oneOverSqrtThree});
	mb.normal(Vec3{ oneOverSqrtThree,  oneOverSqrtThree,  oneOverSqrtThree});
	mb.addVertex();

	mb.position(Vec3{-oneOverSqrtThree,  oneOverSqrtThree,  oneOverSqrtThree});
	mb.normal(Vec3{-oneOverSqrtThree,  oneOverSqrtThree,  oneOverSqrtThree});
	mb.addVertex();

	mb.addTriangle(idx + 0, idx + 3, idx + 1);
	mb.addTriangle(idx + 3, idx + 2, idx + 1);
	mb.addTriangle(idx + 0, idx + 1, idx + 4);
	mb.addTriangle(idx + 1, idx + 5, idx + 4);
	mb.addTriangle(idx + 0, idx + 4, idx + 3);
	mb.addTriangle(idx + 4, idx + 7, idx + 3);
	mb.addTriangle(idx + 6, idx + 5, idx + 2);
	mb.addTriangle(idx + 5, idx + 1, idx + 2);
	mb.addTriangle(idx + 6, idx + 2, idx + 7);
	mb.addTriangle(idx + 2, idx + 3, idx + 7);
	mb.addTriangle(idx + 6, idx + 7, idx + 5);
	mb.addTriangle(idx + 7, idx + 4, idx + 5);
}

Mesh* createOctahedron(const VertexDescription& vd) {
  MeshBuilder mb(vd);
  addOctahedron(mb);
  return mb.buildMesh();
}

void addOctahedron(MeshBuilder& mb) {
  uint32_t idx = mb.getNextIndex();
	// Because we have unit lengths here, normals equal positions.
	mb.position(Vec3{1.0f, 0.0f, 0.0f});
	mb.normal(Vec3{1.0f, 0.0f, 0.0f});
	mb.addVertex();
	mb.position(Vec3{-1.0f, 0.0f, 0.0f});
	mb.normal(Vec3{-1.0f, 0.0f, 0.0f});
	mb.addVertex();
	mb.position(Vec3{0.0f, 1.0f, 0.0f});
	mb.normal(Vec3{0.0f, 1.0f, 0.0f});
	mb.addVertex();
	mb.position(Vec3{0.0f, -1.0f, 0.0f});
	mb.normal(Vec3{0.0f, -1.0f, 0.0f});
	mb.addVertex();
	mb.position(Vec3{0.0f, 0.0f, 1.0f});
	mb.normal(Vec3{0.0f, 0.0f, 1.0f});
	mb.addVertex();
	mb.position(Vec3{0.0f, 0.0f, -1.0f});
	mb.normal(Vec3{0.0f, 0.0f, -1.0f});
	mb.addVertex();
	
	mb.addTriangle(idx + 4, idx + 0, idx + 2);
	mb.addTriangle(idx + 4, idx + 2, idx + 1);
	mb.addTriangle(idx + 4, idx + 1, idx + 3);
	mb.addTriangle(idx + 4, idx + 3, idx + 0);
	mb.addTriangle(idx + 5, idx + 2, idx + 0);
	mb.addTriangle(idx + 5, idx + 1, idx + 2);
	mb.addTriangle(idx + 5, idx + 3, idx + 1);
	mb.addTriangle(idx + 5, idx + 0, idx + 3);
}

Mesh* createDodecahedron(const VertexDescription& vd) {
  MeshBuilder mb(vd);
  addDodecahedron(mb);
  return mb.buildMesh();
}

void addDodecahedron(MeshBuilder& mb) {
  uint32_t idx = mb.getNextIndex();
	// 1 / sqrt(3)
	const float a = 0.57735026918962576450f;
	// sqrt((3 - sqrt(5)) / 6)
	const float b = 0.35682208977308993194f;
	// sqrt((3 + sqrt(5)) / 6)
	const float c = 0.93417235896271569645f;

	// Because we have unit lengths here, normals equal positions.
	mb.position(Vec3{ a,  a,  a});
	mb.normal(Vec3{ a,  a,  a});
	mb.addVertex();
	mb.position(Vec3{ a,  a, -a});
	mb.normal(Vec3{ a,  a, -a});
	mb.addVertex();
	mb.position(Vec3{ a, -a,  a});
	mb.normal(Vec3{ a, -a,  a});
	mb.addVertex();
	mb.position(Vec3{ a, -a, -a});
	mb.normal(Vec3{ a, -a, -a});
	mb.addVertex();

	mb.position(Vec3{-a,  a,  a});
	mb.normal(Vec3{-a,  a,  a});
	mb.addVertex();
	mb.position(Vec3{-a,  a, -a});
	mb.normal(Vec3{-a,  a, -a});
	mb.addVertex();
	mb.position(Vec3{-a, -a,  a});
	mb.normal(Vec3{-a, -a,  a});
	mb.addVertex();
	mb.position(Vec3{-a, -a, -a});
	mb.normal(Vec3{-a, -a, -a});
	mb.addVertex();

	mb.position(Vec3{ b,  c,  0});
	mb.normal(Vec3{ b,  c,  0});
	mb.addVertex();
	mb.position(Vec3{-b,  c,  0});
	mb.normal(Vec3{-b,  c,  0});
	mb.addVertex();
	mb.position(Vec3{ b, -c,  0});
	mb.normal(Vec3{ b, -c,  0});
	mb.addVertex();
	mb.position(Vec3{-b, -c,  0});
	mb.normal(Vec3{-b, -c,  0});
	mb.addVertex();

	mb.position(Vec3{ c,  0,  b});
	mb.normal(Vec3{ c,  0,  b});
	mb.addVertex();
	mb.position(Vec3{ c,  0, -b});
	mb.normal(Vec3{ c,  0, -b});
	mb.addVertex();
	mb.position(Vec3{-c,  0,  b});
	mb.normal(Vec3{-c,  0,  b});
	mb.addVertex();
	mb.position(Vec3{-c,  0, -b});
	mb.normal(Vec3{-c,  0, -b});
	mb.addVertex();

	mb.position(Vec3{ 0,  b,  c});
	mb.normal(Vec3{ 0,  b,  c});
	mb.addVertex();
	mb.position(Vec3{ 0, -b,  c});
	mb.normal(Vec3{ 0, -b,  c});
	mb.addVertex();
	mb.position(Vec3{ 0,  b, -c});
	mb.normal(Vec3{ 0,  b, -c});
	mb.addVertex();
	mb.position(Vec3{ 0, -b, -c});
	mb.normal(Vec3{ 0, -b, -c});
	mb.addVertex();
	
	mb.addTriangle(idx +  0, idx +  8, idx +  9);
	mb.addTriangle(idx +  0, idx +  9, idx +  4);
	mb.addTriangle(idx +  0, idx +  4, idx + 16);
	mb.addTriangle(idx +  0, idx + 12, idx + 13);
	mb.addTriangle(idx +  0, idx + 13, idx +  1);
	mb.addTriangle(idx +  0, idx +  1, idx +  8);
	mb.addTriangle(idx +  0, idx + 16, idx + 17);
	mb.addTriangle(idx +  0, idx + 17, idx +  2);
	mb.addTriangle(idx +  0, idx +  2, idx + 12);
	mb.addTriangle(idx +  8, idx +  1, idx + 18);
	mb.addTriangle(idx +  8, idx + 18, idx +  5);
	mb.addTriangle(idx +  8, idx +  5, idx +  9);
	mb.addTriangle(idx + 12, idx +  2, idx + 10);
	mb.addTriangle(idx + 12, idx + 10, idx +  3);
	mb.addTriangle(idx + 12, idx +  3, idx + 13);
	mb.addTriangle(idx + 16, idx +  4, idx + 14);
	mb.addTriangle(idx + 16, idx + 14, idx +  6);
	mb.addTriangle(idx + 16, idx +  6, idx + 17);
	mb.addTriangle(idx +  9, idx +  5, idx + 15);
	mb.addTriangle(idx +  9, idx + 15, idx + 14);
	mb.addTriangle(idx +  9, idx + 14, idx +  4);
	mb.addTriangle(idx +  6, idx + 11, idx + 10);
	mb.addTriangle(idx +  6, idx + 10, idx +  2);
	mb.addTriangle(idx +  6, idx +  2, idx + 17);
	mb.addTriangle(idx +  3, idx + 19, idx + 18);
	mb.addTriangle(idx +  3, idx + 18, idx +  1);
	mb.addTriangle(idx +  3, idx +  1, idx + 13);
	mb.addTriangle(idx +  7, idx + 15, idx +  5);
	mb.addTriangle(idx +  7, idx +  5, idx + 18);
	mb.addTriangle(idx +  7, idx + 18, idx + 19);
	mb.addTriangle(idx +  7, idx + 11, idx +  6);
	mb.addTriangle(idx +  7, idx +  6, idx + 14);
	mb.addTriangle(idx +  7, idx + 14, idx + 15);
	mb.addTriangle(idx +  7, idx + 19, idx +  3);
	mb.addTriangle(idx +  7, idx +  3, idx + 10);
	mb.addTriangle(idx +  7, idx + 10, idx + 11);
}

Mesh* createIcosahedron(const VertexDescription& vd) {
  MeshBuilder mb(vd);
  addIcosahedron(mb);
  return mb.buildMesh();
}

void addIcosahedron(MeshBuilder& mb) {
  uint32_t idx = mb.getNextIndex();
	// Golden Ratio (1 + sqrt(5)) / 2
	//const float gr = 1.61803398874989484820f;
	// Length of the vectors sqrt(1 + gr^2)
	//const float length = 1.90211303259030714423f;
	// Normalized Golden Ratio (gr / length)
	const float grN = 0.85065080835203993218f;
	// Normalized One (1 / length)
	const float oneN = 0.52573111211913360602f;

	// Because we have unit lengths here, normals equal positions.
	mb.position(Vec3{grN, oneN, 0.0f});
	mb.normal(Vec3{grN, oneN, 0.0f});
	mb.addVertex();
	mb.position(Vec3{-grN, oneN, 0.0f});
	mb.normal(Vec3{-grN, oneN, 0.0f});
	mb.addVertex();
	mb.position(Vec3{grN, -oneN, 0.0f});
	mb.normal(Vec3{grN, -oneN, 0.0f});
	mb.addVertex();
	mb.position(Vec3{-grN, -oneN, 0.0f});
	mb.normal(Vec3{-grN, -oneN, 0.0f});
	mb.addVertex();

	mb.position(Vec3{oneN, 0.0f, grN});
	mb.normal(Vec3{oneN, 0.0f, grN});
	mb.addVertex();
	mb.position(Vec3{oneN, 0.0f, -grN});
	mb.normal(Vec3{oneN, 0.0f, -grN});
	mb.addVertex();
	mb.position(Vec3{-oneN, 0.0f, grN});
	mb.normal(Vec3{-oneN, 0.0f, grN});
	mb.addVertex();
	mb.position(Vec3{-oneN, 0.0f, -grN});
	mb.normal(Vec3{-oneN, 0.0f, -grN});
	mb.addVertex();

	mb.position(Vec3{0.0f, grN, oneN});
	mb.normal(Vec3{0.0f, grN, oneN});
	mb.addVertex();
	mb.position(Vec3{0.0f, -grN, oneN});
	mb.normal(Vec3{0.0f, -grN, oneN});
	mb.addVertex();
	mb.position(Vec3{0.0f, grN, -oneN});
	mb.normal(Vec3{0.0f, grN, -oneN});
	mb.addVertex();
	mb.position(Vec3{0.0f, -grN, -oneN});
	mb.normal(Vec3{0.0f, -grN, -oneN});
	mb.addVertex();
	
	mb.addTriangle(idx + 0, idx + 8, idx + 4);
	mb.addTriangle(idx + 1, idx + 10,idx + 7);
	mb.addTriangle(idx + 2, idx + 9, idx + 11);
	mb.addTriangle(idx + 7, idx + 3, idx + 1);
  
	mb.addTriangle(idx + 0, idx + 5, idx + 10);
	mb.addTriangle(idx + 3, idx + 9, idx + 6);
	mb.addTriangle(idx + 3, idx + 11,idx + 9);
	mb.addTriangle(idx + 8, idx + 6, idx + 4);
  
	mb.addTriangle(idx + 2, idx + 4, idx + 9);
	mb.addTriangle(idx + 3, idx + 7, idx + 11);
	mb.addTriangle(idx + 4, idx + 2, idx + 0);
	mb.addTriangle(idx + 9, idx + 4, idx + 6);
  
	mb.addTriangle(idx + 2, idx + 11,idx + 5);
	mb.addTriangle(idx + 0, idx + 10,idx + 8);
	mb.addTriangle(idx + 5, idx + 0, idx + 2);
	mb.addTriangle(idx + 10,idx + 5, idx + 7);
  
	mb.addTriangle(idx + 1, idx + 6, idx + 8);
	mb.addTriangle(idx + 1, idx + 8, idx + 10);
	mb.addTriangle(idx + 6, idx + 1, idx + 3);
	mb.addTriangle(idx + 11,idx + 7 ,idx + 5);
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
