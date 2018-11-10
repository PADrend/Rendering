/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MESHUTILS_H
#define MESHUTILS_H

#include <Geometry/Matrix4x4.h>

#include <cstdint>
#include <deque>
#include <utility>
#include <vector>
#include <set>
#include <limits>

namespace Geometry {
template<typename _T> class _Plane;
typedef _Plane<float> Plane;
template<typename _T> class _Sphere;
typedef _Sphere<float> Sphere_f;
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3;
template<typename _T> class _Ray;
typedef _Ray<Vec3> Ray3;
}

namespace Util {
class Color4f;
class StringIdentifier;
class PixelAccessor;
}

namespace Rendering {

class Mesh;
class MeshVertexData;
class VertexDescription;



/**
 * @brief Operations on meshes
 *
 * Functions and classes for the creation (e.g. MeshUtils::MeshBuilder) or modification (e.g. MeshUtils::calculateNormals) of meshes.
 *
 * @author Claudius Jaehn
 * @author Ralf Petring
 * @author Benjamin Eikel
 * @author Stefan Arens
 * @author Paul Justus
 */
namespace MeshUtils {

//! Compute a tight bounding sphere for the vertex positions of the given mesh.
Geometry::Sphere_f calculateBoundingSphere(Mesh * mesh);

/**
 * Compute a tight bounding sphere for the vertex positions of the given meshes
 * after applying the corresponding transformations to the positions.
 */
Geometry::Sphere_f calculateBoundingSphere(const std::vector<std::pair<Mesh *, Geometry::Matrix4x4>> & meshesAndTransformations);

//! Calculate a hash value for the given mesh.
uint32_t calculateHash(Mesh * mesh);

//! Calculate a hash value for the given vertex description.
uint32_t calculateHash(const VertexDescription & vd);

/**
 * calulates vertex normals for a given mesh calculation is done by
 * - first calculating face normals
 * - second calculating the unweighted average of the adjacent face normals for all vertices
 * @note if the mesh has already normals these are ignored and recalculated
 * @param m the mesh to be modified
 * @author Ralf Petring
 */
void calculateNormals(Mesh * m);

/**
 * Calculate and add tangent space vectors from the normals and uv-coordinates of the given mesh.
 * \note based on: Lengyel, Eric. "Computing Tangent Space Basis Vectors for an Arbitrary Mesh".
 * Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
 * The bitangent can be calculated in the shader by:
 * float3 bitangent = cross(normal, tangent.xyz) * tangent.w;
 */
void calculateTangentVectors(	Mesh * mesh, const Util::StringIdentifier uvName,
const Util::StringIdentifier tangentVecName);


//! Create texture coordinates by projecting the vertices with the given projection matrix.
void calculateTextureCoordinates_projection( Mesh * mesh, Util::StringIdentifier attribName, const Geometry::Matrix4x4 & projection);

/**
 * Combine several meshes into a single mesh.
 *
 * @note All meshes must have the same VertexDescription.
 * @author Claudius Jaehn
 * @author Stefan Arens
 * @author Paul Justus
 */
Mesh * combineMeshes(const std::deque<Mesh *> & meshArray);
Mesh * combineMeshes(const std::deque<Mesh *> & meshArray, const std::deque<Geometry::Matrix4x4> & transformations);

/**
 * Splits the vertex data of a given mesh into multiple blocks of vertex data each containing @a chunkSize many vertices.
 *
 * @note The last block only contains MeshVertexCount % chunkSize many vertices.
 * @author Sascha Brauer
 */
std::deque<MeshVertexData> splitVertexData(Mesh * mesh, uint32_t chunkSize);

/**
 * Extracts a range of vertices from the given mesh.
 *
 * @param mesh The mesh to extract the vertex data from
 * @param begin Start of the range of extracted vertices
 * @param length The number of vertices to extract
 *
 * @author Sascha Brauer
 */
MeshVertexData * extractVertexData(Mesh * mesh, uint32_t begin, uint32_t length);

//! Return @c true iff the given two meshes contain the same data - only the glIds and the filenames are not compared.
bool compareMeshes( Mesh * mesh1,Mesh * mesh2 );

/**
 * allocates the memory for storing old vertices in new format and copies the old values to the correct position in the new memory
 * @note missing values are initialized with 0
 * @note values which do not fit into the new format get lost
 * @author Ralf Petring
 */
MeshVertexData * convertVertices(	const MeshVertexData & vertices,
const VertexDescription & newVertexDescription);

 //! Copy data from one vertex attribute to another. Create, or modify the target attribute.
void copyVertexAttribute(Mesh * mesh, Util::StringIdentifier from, Util::StringIdentifier to);


/**
 * Remove vertices which are equal to each other from the mesh and
 * store them only once. The indices to the vertices are adjusted.
 * This function has runtime O(n * log(n)) where n is the
 * number of vertices in @a mesh.
 *
 * @param mesh Mesh to do the elimination on.
 *
 * @author Benjamin Eikel
 */
void eliminateDuplicateVertices(Mesh * mesh);

/**
 * Clone the given mesh but remove all vertices which are
 * never referenced.
 */
Mesh * eliminateUnusedVertices(Mesh * mesh);

/**
 * Deletes long triangles (whose ratio between the longest side and
 * the corresponding height is > ratio).
 * \note Calls eliminateUnusedVertices to remove unused vertices.
 */
Mesh * eliminateLongTriangles(Mesh * mesh, float ratio);

/**
 * Delete triangles that have at least one vertex lying behind the given plane.
 *
 * @param mesh Source mesh. The mesh is not changed.
 * @param plane Plane that is used for cutting off vertices.
 * @return New mesh
 */
Mesh * eliminateTrianglesBehindPlane(Mesh * mesh, const Geometry::Plane & plane);

/**
 * Delete triangles that have (nearly) zero area.
 *
 * @param mesh Source mesh. The mesh is not changed.
 * @return New mesh
 */
Mesh * eliminateZeroAreaTriangles(Mesh * m);

/**
 *Estimate the max. side length of the polygon in the mesh m
*/

float getLongestSideLength(Mesh * m);

/**
 * Take the given mesh and optimize the indices stored there for
 * vertex cache optimality.
 * The implementation is based on the algorithm described by Sander,
 * Nehab and Barczak.
 * This function has runtime O(n) where n is the number of indices
 * in @a mesh.
 *
 * @param mesh Mesh whose indices will be optimized.
 * @param cacheSize Post-transform vertex cache size to optimize
 * for. This parameter is called @c k in the article.
 * @see http://doi.acm.org/10.1145/1276377.1276489
 * @author Benjamin Eikel
 */
void optimizeIndices(Mesh * mesh, const uint_fast8_t cacheSize =	24);

/**
 * removes the color information from a mesh
 * @param m the mesh to be modified
 * @author Ralf Petring
 */
void removeColorData(Mesh * m);

/**
 * Identify triangles that span large depth ranges by calculating their normals.
 * Remove those triangles and move the adjacent vertices in the background a little bit to cover the arising hole.
 *
 * @note The calculation assumes that z values represent depth, x values represent horizontal direction, and y values represent vertical direction.
 * This holds for meshes generated from depth textures.
 * @param mesh Source mesh. The mesh is not changed.
 * @param maxNormalZ Maximum absolute z coordinate of the triangle's normal so that the triangle is removed.
 * @param coveringMovement Ratio of the depth range of the removed triangle that the background vertices are moved in normal direction of this triangle.
 * @return New mesh
 * @note maxNormalZ = 0.6f and coveringMovement = 0.1f are good starting points.
 */
Mesh * removeSkinsWithHoleCovering(Mesh * mesh, float maxNormalZ, float coveringMovement);

/**
 * Change the order of the vertices of each triangle.
 *
 * @param mesh Input and output mesh (the given mesh is changed).
 * @note This function only works for meshes with a triangle list. If the mesh
 * uses another mode, then the mesh is not changed.
 * @author Benjamin Eikel
 */
void reverseWinding(Mesh * mesh);

/**
 * splits all triangles in the mesh which have at least one side longer than specified
 * @param m the mesh to deal with
 * @param maxSideLength the maximum length of a triangles edge, if this value is exceeded the triangle will be split up
 * @return the modified mesh
 * @author Ralf Petring
 */
void splitLargeTriangles(Mesh * m, float maxSideLength);

/**
 * Apply color information to each vertex of the mesh
 * @param mesh the mesh to be midified
 * @param color the color to be set
 * @author Ralf Petring
 */
void setColor(Mesh * mesh, const Util::Color4f & color);

/**
 * Apply material information to each vertex of the mesh
 * @param mesh the mesh to be midified
 * @param ambient the ambient color to be set
 * @param diffuse the diffuse color to be set
 * @param specular the specular color to be set
 * @param shininess the shininess value to be set
 * @author Ralf Petring
 */
void setMaterial(Mesh * mesh, const Util::Color4f & ambient, const Util::Color4f & diffuse, const Util::Color4f & specular, float shininess);


/**
 * converts normals from 3 * GL_FLOAT to 4 * GL_BYTE if present
 * converts colors from (3 or 4) * GL_FLOAT to 4 * GL_UNSIGNED_BYTE if present
 * optionally converts position from (3 or 4) * GL_FLOAT to 4 * GL_HALF_FLOAT
 * @param m the mesh to be shrinked
 * @param
 * @author Ralf Petring
 */
void shrinkMesh(Mesh * m, bool shrinkPosition=false);


/**
 * transforms the position and the normals of the vertices of the vertex data by the given matrix
 * @param mesh the vertex data to be modified
 * @param transMat the matrix to be used for transformation
 * @author Claudius Jaehn
 */
void transform(MeshVertexData & vd, const Geometry::Matrix4x4 & transMat);

//!	Transforms one specific vertexAttribute of the vertexData according to the given matrix.
void transformCoordinates(MeshVertexData & vd, Util::StringIdentifier attrName , const Geometry::Matrix4x4 & transMat,uint32_t begin,uint32_t numVerts);
void transformNormals(MeshVertexData & vd, Util::StringIdentifier attrName , const Geometry::Matrix4x4 & transMat,uint32_t begin,uint32_t numVerts);

/**
 * Return a new VertexDescription that contains the union of all VertexAttributes of the given VertexDescriptions.
 *
 * @param vertexDescs Container with VertexDescriptions that will be analysed.
 * @return New VertexDescription that is able to hold all VertexAttributes.
 * @author Benjamin Eikel
 */
VertexDescription uniteVertexDescriptions(const std::deque<VertexDescription> & vertexDescs);

/**
 * Cuts the given mesh along the given plane.
 *
 * @param m the mesh to be cut
 * @param plane the cutting plane
 * @param tIndices list of triangle indices to cut. If empty, the whole mesh is cut.
 * @param tolerance if a vertex lies on the plane with the given tolerance, no new vertex is created
 * @author Sascha Brandt
 */
void cutMesh(Mesh* m, const Geometry::Plane& plane, const std::set<uint32_t> tIndices={}, float tolerance=std::numeric_limits<float>::epsilon());

/**
 * Extrudes the specified triangles of the given mesh.
 *
 * @param m the mesh
 * @param dir extrusion direction
 * @param tIndices set of triangle indices to extrude
 * @author Sascha Brandt
 */
void extrudeTriangles(Mesh* m, const Geometry::Vec3& dir, const std::set<uint32_t> tIndices);

/**
 * Slow method for finding the first triangle in a mesh that intersects the given ray.
 * @param m the mesh
 * @param ray the ray
 * @return -1 if no intersecting triangle was found, the triangle index otherwise.
 * @author Sascha Brandt
 */
int32_t getFirstTriangleIntersectingRay(Mesh* m, const Geometry::Ray3& ray);

/**
 * Remove vertices which are close to each other from the mesh and
 * store them only once. The indices to the vertices are adjusted.
 * This function has runtime O(n * log(n)) where n is the
 * number of vertices in @a mesh.
 *
 * @param mesh Mesh to do the elimination on.
 * @return number of merged vertices
 * @author Sascha Brandt
 */
uint32_t mergeCloseVertices(Mesh * mesh, float tolerance=std::numeric_limits<float>::epsilon());

/**
 * Splits a mesh into its connected components.
 *
 * @param mesh Mesh to split into connected components
 * @param relDistance relative distance (w.r.t. mesh's bounding box) between vertices that are considered as connected.
 * @return connected components of the mesh
 * @author Sascha Brandt
 */
std::deque<Mesh*> splitIntoConnectedComponents(Mesh* mesh, float relDistance=0.001);

/**
 * Moves every vertex along their normal according to the given texture (using its u,v coordinates).
 *
 * @param mesh The mesh
 * @param displaceAcc pixel accessor of the displacement map
 * @param scale scale factor multiplied with the value of the texture
 * @param clampToEdge clamp to texture borders (true) or wrap around (false)
 * @author Sascha Brandt
 */
void applyDisplacementMap(Mesh* mesh, Util::PixelAccessor* displaceAcc, float scale=1.0, bool clampToEdge=false);


/**
 * Moves every vertex along their normal using the value of a 3D perlin noise function.
 *
 * @param mesh The mesh
 * @param seed The seed for the noise generator
 * @param noiseScale scale factor multiplied with the noise value
 * @param transform transformation matrix applied on each position
 * @author Sascha Brandt
 */
void applyNoise(Mesh* mesh, float noiseScale=1.0, const Geometry::Matrix4x4& transform={}, uint32_t seed=0);

/**
 * Sets the y-coordinates of all vertices in a radius around a given 3d position to it's y-coordinate (with cubic bezier falloff) 
 *
 * @param mesh The mesh
 * @param pos the 3d position
 * @param radius radius around the 3d position to flatten vertices
 * @param falloff blend falloff for vertices beyond the radius
 * @author Sascha Brandt
 */
void flattenMesh(Mesh* mesh, const Geometry::Vec3& pos, float radius, float falloff);

/**
 * Computes the combined surface area of all triangles in a mesh 
 *
 * @param mesh The mesh
 * @return the surface area
 * @author Sascha Brandt
 */
float computeSurfaceArea(Mesh* mesh);

/**
 * Extracts the vertices of a mesh with corresponding indices and moves them to a new mesh 
 *
 * @param mesh Mesh to extract vertices from
 * @param indices array of indices of the vertices to be extracted
 * @return new mesh
 * @author Sascha Brandt
 */
Rendering::MeshVertexData* extractVertices(Rendering::Mesh* mesh, const std::vector<uint32_t>& indices);

/**
 * Copies vertices from one mesh to another. 
 * If both meshes are uploaded, it directly copies using the buffers.
 *
 * @param source Mesh to copy vertices from
 * @param target Mesh to copy vertices to
 * @param sourceOffset vertex offset of the source mesh
 * @param targetOffset vertex offset of the target mesh
 * @param count number of vertices to copy
 * @author Sascha Brandt
 */
void copyVertices(Rendering::Mesh* source, Rendering::Mesh* target, uint32_t sourceOffset, uint32_t targetOffset, uint32_t count);

}
}

#endif // MESHUTILS_H
