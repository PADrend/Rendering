/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Simplification.h"
#include "MeshUtils.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttributeAccessors.h"
#include "../Mesh/VertexAttributeIds.h"
#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>
#include <Geometry/Sphere.h>
#include <Geometry/Triangle.h>
#include <Util/Macros.h>
#include <Util/Numeric.h>
#include <Util/ProgressIndicator.h>
#include <Util/UpdatableHeap.h>
#include <Util/Utils.h>
#include <Util/References.h>
#include <Util/Timer.h>
#include <algorithm>
#include <array>
#include <deque>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Rendering {
namespace MeshUtils {
namespace Simplification {

struct heapData{
	unsigned int vertex1;
	unsigned int vertex2;
	std::vector<float> optPos;

	heapData() :
		vertex1(std::numeric_limits<unsigned int>::max()), vertex2(std::numeric_limits<unsigned int>::max()), optPos() {
	}
	heapData(unsigned int v1, unsigned int v2) :
		vertex1(v1), vertex2(v2), optPos() {
	}
};

typedef Util::UpdatableHeap<float, heapData>::UpdatableHeapElement heapElement;
typedef std::set<heapElement*>::iterator heapElementSetIterator;
typedef std::set<unsigned int>::iterator uIntSetIterator;
const float DONT_MERGE_COST = std::numeric_limits<float>::max();

struct VertexPoint : public Geometry::Point<Geometry::Vec3f> {
	unsigned int data;

	VertexPoint(const Geometry::Vec3f & pos, unsigned int index) :
			Geometry::Point<Geometry::Vec3f>(pos), data(index) {
	}
};

/**
 * Storage of an upper triangular n-by-n square matrix.
 * Only the entries in the upper triangle and the diagonal are stored and no memory is used for the entries in the lower triangle.
 *
 * @author Benjamin Eikel
 * @date 2010-12-20
 */
template<typename _T>
class UpperTriangularMatrix {
	private:
		//! Dimension of the matrix
		size_t n;
		//! One-dimensional array containing the entries
		std::vector<_T> data;
		//! Return the number of entries in the @c data array.
		size_t arraySize() const {
			return n * (n + 1) / 2;
		}
		//! Calculate the index of an entry (i, j).
		size_t calcIndex(size_t i, size_t j) const {
			// index = j + n * i - ((i * (i + 1)) / 2) = 1/2 * i * (2 * n - i - 1)
			return j + i * (2 * n - i - 1) / 2;
		}
	public:
		/**
		 * Construct a new square matrix of order n.
		 * The elements are initialized to zero.
		 *
		 * @param size Matrix size n
		 */
		explicit UpperTriangularMatrix(size_t size) :
			n(size), data(arraySize(), static_cast<_T>(0)) {
		}

		//! Return the dimension of the matrix.
		size_t getSize() const {
			return n;
		}

		/**
		 * Return an entry of the matrix.
		 *
		 * @param i Row number from [0, n-1]
		 * @param j Column number from [0, n-1]
		 * @return Element (i, j) of the matrix if i <= j and 0 for i > j
		 */
		_T get(size_t i, size_t j) const {
			if (i > j) {
				return 0;
			} else {
				return data[calcIndex(i, j)];
			}
		}


		/**
		 * Set an entry of the matrix to the given value.
		 * Attempts to set a value in the lower triangle with i > j are ignored.
		 *
		 * @param i Row number from [0, n-1]
		 * @param j Column number from [0, n-1]
		 * @param value New value
		 */
		void set(size_t i, size_t j, _T value) {
			if (i <= j) {
				data[calcIndex(i, j)] = value;
			}
		}


		/**
		 * Add the given value an entry of the matrix.
		 * Attempts to add a value in the lower triangle with i > j are ignored.
		 *
		 * @param i Row number from [0, n-1]
		 * @param j Column number from [0, n-1]
		 * @param summand Summand to add
		 */
		void add(size_t i, size_t j, _T summand) {
			if (i <= j) {
				data[calcIndex(i, j)] += summand;
			}
		}

		/**
		 * Add a second matrix component-wise to this matrix.
		 *
		 * @param second Another upper triangular matrix.
		 */
		void operator+=(const UpperTriangularMatrix<_T> & second) {
			if (second.n != n) {
				throw std::invalid_argument("Second matrix has different size");
			}
			const size_t size = arraySize();
			for (size_t i = 0; i < size; ++i) {
				data[i] += second.data[i];
			}
		}
};

template<typename _T>
struct Quadric {
	UpperTriangularMatrix<_T> A;
	std::vector<_T> b;
	_T c;

	explicit Quadric(size_t n) : A(n), b(n, static_cast<_T>(0)), c(static_cast<_T>(0)) {}

	/**
	 * Add a second quadric to this quadric by adding the three components.
	 *
	 * @param second Another quadric.
	 */
	void operator+=(const Quadric<_T> & second) {
		A += second.A;
		const size_t size = b.size();
		for(size_t i = 0; i < size; ++i) {
			b[i] += second.b[i];
		}
		c += second.c;
	}

	/**
	 * Return the quadric value given by Q(v) = v^T A v + 2 b^T v + c.
	 *
	 * @param v Parameter vector
	 * @return Value of the quadratic form evaluated using v
	 */
	_T getCost(const std::vector<_T> & v) const {
		const size_t size = v.size();
		if (size != A.getSize()) {
			throw std::invalid_argument("Second vector has different size");
		}
		_T v_A_v = 0;
		_T b_v = 0;
		for(uint_fast8_t i = 0; i < size; ++i) {
			_T sum = 0;
			for(uint_fast8_t j = 0; j < size; ++j) {
				if(i > j) {
					// Switch row and column to simulate an access to a symmetric matrix.
					sum += v[j] * A.get(j, i);
				} else {
					sum += v[j] * A.get(i, j);
				}
			}
			v_A_v += sum * v[i];

			b_v += b[i] * v[i];
		}
		return v_A_v + 2 * b_v + c;
	}
};

struct vertex_t {
	Quadric<float> q;
	std::vector<float> data; // vertex data: VERTEX, NORMAL, COLOR, TEX0
	std::set<heapElement*> inHeap;
	std::deque<uint32_t> inIndex;
	std::set<unsigned int> neighbors;

	explicit vertex_t(size_t n) :
		q(n), data(), inHeap(), inIndex(), neighbors() {
		data.reserve(n);
	}
};

/**
 * Modify the given @a container and make sure, that it only contains unique elements.
 *
 * @param container Random-access container (e.g. of type std::deque or std::vector).
 */
template<typename ContainerType>
inline static void make_unique(ContainerType & container) {
	// First sort the container because std::unique only works like we expect it here on a sorted range.
	std::sort(container.begin(), container.end());
	// Re-order the container and move duplicate entries to the end. The iterator will point to the last element of the range of unique elements.
	auto it = std::unique(container.begin(), container.end());
	// Now really remove the duplicate elements at the end.
	container.resize(static_cast<std::size_t>(std::distance(container.begin(), it)));
}

/**
 * Normalize a vector
 * 
 * @param normalVector Vector as array
 * @return @c true if successful, @c false if the length is zero
 */
static bool normalize(std::vector<float> & normalVector) {
	float length = 0;
	for(const auto & n : normalVector) {
		length += n * n;
	}
	length = std::sqrt(length);

	if(length == 0) {
		return false;
	}

	length = 1 / length;
	for(auto & n : normalVector) {
		n *= length;
	}
	return true;
}

/**
 * Calculate the normal of the triangle that is induced by three vertices.
 * 
 * @param vertexA First vertex
 * @param vertexB Second vertex
 * @param vertexC Third vertex
 * @return Normal of the triangle. If the direction has zero length before
 * normalization, a zero vector is returned.
 */
static Geometry::Vec3f calcNormal(const float * vertexA, const float * vertexB, const float * vertexC) {
	const auto triangle = Geometry::Triangle_f(Geometry::Vec3f(vertexA), Geometry::Vec3f(vertexB), Geometry::Vec3f(vertexC));
	// Do not use calcNormal: Check for zero length before calling normalize.
	const auto direction = triangle.getEdgeAB().cross(triangle.getEdgeAC());
	const auto length = direction.length();
	if(length < 1.0e-6f) {
		return Geometry::Vec3f();
	}
	return direction / length;
}

/**
 * Adds quadric error metric from planes spanned by vertices p, q and r to
 * quadric q.
 */
static void getQuadric(const std::vector<float> & p, const std::vector<float> & q, const std::vector<float> & r, Quadric<float> & quadric){
	const size_t size = quadric.A.getSize();

	// e1 = (q - p) / ||q - p||
	std::vector<float> e1;
	e1.reserve(size);
	for(uint_fast8_t i=0; i<size; ++i){
		e1.push_back(q[i] - p[i]);
	}
	normalize(e1);

	// e2 = (r - p - (e1 * (r - p)) e1) / ||...||
	std::vector<float> e2;
	e2.reserve(size);
	float r_p_e1 = 0.0f;
	for(uint_fast8_t i=0; i<size; ++i){
		r_p_e1 += (r[i] - p[i]) * e1[i];
	}
	for(uint_fast8_t i=0; i<size; ++i){
		e2.push_back(r[i] - p[i] - r_p_e1 * e1[i]);
	}
	normalize(e2);

	float p_e1 = 0.0f;
	float p_e2 = 0.0f;
	float p_p = 0.0f;

	// A = I - e1 e1^T - e2 e2^T
	for(uint_fast8_t i=0; i<size; ++i){
		for(uint_fast8_t j=0; j<size; ++j){
			quadric.A.set(i, j, -e1[i]*e1[j]-e2[i]*e2[j]);
		}
		quadric.A.add(i, i, 1.0f);

		p_e1 += p[i]*e1[i];
		p_e2 += p[i]*e2[i];
		p_p += p[i]*p[i];
	}

	// b = (p * e1) e1 + (p * e2) e2 - p
	for(uint_fast8_t i=0; i<size; ++i){
		quadric.b[i] = p_e1*e1[i]+p_e2*e2[i]-p[i];
	}

	// c = p * p - (p * e1)^2 - (p * e2)^2
	quadric.c = p_p-p_e1*p_e1-p_e2*p_e2;
}

/**
 * Allocates memory for array and initializes matrices q
 * @return pointer to allocated array
 */
static std::size_t initVertexArray(Mesh * mesh, 
								   Util::ProgressIndicator & progress, 
								   const weights_t & weights,
								   std::vector<vertex_t> & vertices, 
								   Geometry::PointOctree<VertexPoint> * vOctree) {
	const uint32_t vertexCount = mesh->getVertexCount();
	MeshVertexData & vertexData = mesh->openVertexData();

	std::size_t numDataEntries = 0;

	Util::Reference<PositionAttributeAccessor> positionAccessor;
	if(weights[VERTEX_OFFSET] > 0) {
		try {
			positionAccessor = PositionAttributeAccessor::create(vertexData, VertexAttributeIds::POSITION);
			numDataEntries += 3;
		} catch(...) {
		}
	}
	Util::Reference<NormalAttributeAccessor> normalAccessor;
	if(weights[NORMAL_OFFSET] > 0) {
		try {
			normalAccessor = NormalAttributeAccessor::create(vertexData, VertexAttributeIds::NORMAL);
			numDataEntries += 3;
		} catch(...) {
		}
	}
	Util::Reference<ColorAttributeAccessor> colorAccessor;
	if(weights[COLOR_OFFSET] > 0) {
		try {
			colorAccessor = ColorAttributeAccessor::create(vertexData, VertexAttributeIds::COLOR);
			numDataEntries += 4;
		} catch(...) {
		}
	}
	Util::Reference<TexCoordAttributeAccessor> texCoordAccessor;
	if(weights[TEX0_OFFSET] > 0) {
		try {
			texCoordAccessor = TexCoordAttributeAccessor::create(vertexData, VertexAttributeIds::TEXCOORD0);
			numDataEntries += 2;
		} catch(...) {
		}
	}

	// allocate memory for array and values, get values for vertices and set matrix to 0
	vertices.reserve(vertexCount);
	for(uint_fast32_t v = 0; v < vertexCount; ++v) {
		vertex_t vertex(numDataEntries);

		if(positionAccessor.isNotNull()) {
			const auto position = positionAccessor->getPosition(v) * weights[VERTEX_OFFSET];
			vertex.data.push_back(position.getX());
			vertex.data.push_back(position.getY());
			vertex.data.push_back(position.getZ());

			// build vertex octree
			if(vOctree != nullptr) {
				vOctree->insert(VertexPoint(position, v));
			}
		}
		if(normalAccessor.isNotNull()) {
			const auto normal = normalAccessor->getNormal(v);
			vertex.data.push_back(normal.getX() * weights[NORMAL_OFFSET]);
			vertex.data.push_back(normal.getY() * weights[NORMAL_OFFSET]);
			vertex.data.push_back(normal.getZ() * weights[NORMAL_OFFSET]);
		}
		if(colorAccessor.isNotNull()) {
			const auto color = colorAccessor->getColor4f(v);
			vertex.data.push_back(color.getR() * weights[COLOR_OFFSET]);
			vertex.data.push_back(color.getG() * weights[COLOR_OFFSET]);
			vertex.data.push_back(color.getB() * weights[COLOR_OFFSET]);
			vertex.data.push_back(color.getA() * weights[COLOR_OFFSET]);
		}
		if(texCoordAccessor.isNotNull()) {
			const auto texCoord = texCoordAccessor->getCoordinate(v);
			vertex.data.push_back(texCoord.getX() * weights[TEX0_OFFSET]);
			vertex.data.push_back(texCoord.getY() * weights[TEX0_OFFSET]);
		}

		vertices.emplace_back(std::move(vertex));

		progress.increment();
	}

	// initialize quadrics q
	Quadric<float> tmpQ(numDataEntries);
	const MeshIndexData & iData = mesh->openIndexData();

	const uint32_t indexCount = iData.getIndexCount();

	for (uint_fast32_t i = 0; i < indexCount; i += 3){
		const uint32_t indexA = iData[i + 0];
		const uint32_t indexB = iData[i + 1];
		const uint32_t indexC = iData[i + 2];

		// calculate plane equation
		getQuadric(vertices[indexA].data, vertices[indexB].data, vertices[indexC].data, tmpQ);

		// add face to vertex attribute
		vertices[indexA].inIndex.push_back(i);
		vertices[indexB].inIndex.push_back(i);
		vertices[indexC].inIndex.push_back(i);

		// add quadric distance matrix to q of vertices spanning this plane
		vertices[indexA].q += tmpQ;
		vertices[indexB].q += tmpQ;
		vertices[indexC].q += tmpQ;
		progress.increment();
	}

	for(auto & vertex : vertices) {
		make_unique(vertex.inIndex);
	}

	return numDataEntries;
}

/**
 *  Adds neighbor vertices of vertex v to list n. Neighbors are all vertices
 *  connected via edges (and those with a distance smaller than the threshold).
 *  Attention: returns v as a neighbor of v and may return v as singleNeighbor
 *  @param mesh mesh to get index data from
 *  @param v vertex to get neighbors of
 *  @param threshold maximum distance between two non connected neighbors
 *  @param n returns set of neighbors in n
 *  @param singleNeighbors returns set of neighbors spanning exactly one face with v
 */
static void getNeighboursOfVertex(Mesh * mesh, const vertex_t & v, Geometry::PointOctree<VertexPoint> * vOctree, float threshold, std::set<unsigned int> & n, std::set<std::pair<unsigned int, unsigned int> > & singleNeighbors){
	const MeshIndexData & iData = mesh->openIndexData();
	const uint32_t * indices = iData.data();
	std::vector<bool> multiNeighbors(iData.getMaxIndex(), false);
	for(auto & elem : v.inIndex){
		if(!n.insert(indices[elem+0]).second){
			// has been inserted before => is a multiple neighbor of v
			multiNeighbors[indices[elem+0]] = true;
		}
		if(!n.insert(indices[elem+1]).second){
			// has been inserted before => is a multiple neighbor of v
			multiNeighbors[indices[elem+1]] = true;
		}
		if(!n.insert(indices[elem + 2]).second){
			// has been inserted before => is a multiple neighbor of v
			multiNeighbors[indices[elem+2]] = true;
		}
	}

	// build single neighbor set
	for(auto & elem : v.inIndex){
		if(!multiNeighbors[indices[elem+0]]){
			// iData[*it+0] is not a multiple neighbor => add to singleNeighbors
			singleNeighbors.insert(std::make_pair(indices[elem+0], elem));
		}
		if(!multiNeighbors[indices[elem+1]]){
			// iData[*it+1] is not a multiple neighbor => add to singleNeighbors
			singleNeighbors.insert(std::make_pair(indices[elem+1], elem));
		}
		if(!multiNeighbors[indices[elem+2]]){
			// iData[*it+2] is not a multiple neighbor => add to singleNeighbors
			singleNeighbors.insert(std::make_pair(indices[elem+2], elem));
		}
	}

	// add non connected vertices to neighbors
	if(threshold > 0.0f) {
		std::deque<VertexPoint> nonConnectedNeighbors;
		vOctree->collectPointsWithinSphere(Geometry::Sphere_f(Geometry::Vec3f(v.data[0], v.data[1], v.data[2]), threshold), nonConnectedNeighbors);
		for(auto & nonConnectedNeighbor : nonConnectedNeighbors) {
			n.insert(nonConnectedNeighbor.data);
		}
	}
}

/**
 * Calculated optimal position and cost for merging vertices v1 and v2
 * only compares v1, v2 and (v1+v2)/2 as new positions...
 * @return cost
 */
static float getOptimalPosition(const vertex_t & vertexA, const vertex_t & vertexB, std::vector<float> & optPos, std::size_t dataSize, bool useOptPos){
	bool optPosSuccess=false;
	float cost = 0;
	if(useOptPos){
		// allocate and initiate matrix for inversion
		const auto rowSize = 2 * dataSize;
		auto mInvert = new float[dataSize * rowSize];
		for (std::size_t row = 0; row < dataSize; ++row) {
			for (std::size_t col = 0; col < dataSize; ++col) {
				const auto rowOffset = row * rowSize;
				if (row <= col) {
					mInvert[rowOffset + col] = vertexA.q.A.get(row, col) + vertexB.q.A.get(row, col);
				} else {
					mInvert[rowOffset + col] = vertexA.q.A.get(col, row) + vertexB.q.A.get(col, row);
				}
			}
		}

		if(Util::Numeric::invertMatrix(mInvert, dataSize)){
			optPosSuccess = true;
			// matrix inversion successful
			// calculate & store optimal position
			// Optimal position vBar = - A^-1 b
			optPos.resize(dataSize);
			for (uint_fast8_t row = 0; row < dataSize; ++row) {
				const uint_fast8_t rowOffset = row * rowSize + dataSize;

				float sum = 0.0f;
				for(uint_fast8_t col = 0; col < dataSize; ++col) {
					sum += mInvert[rowOffset + col] * (vertexA.q.b[col] + vertexB.q.b[col]);
				}
				optPos[row] = -sum;
			}
			// Cost Q(vBar) = - b^T A^-1 b + c
			cost = vertexA.q.c + vertexB.q.c;
			for(uint_fast8_t col = 0; col < dataSize; ++col) {
				float sum = 0.0f;
				for (uint_fast8_t row = 0; row < dataSize; ++row) {
					const uint_fast8_t rowOffset = row * rowSize + dataSize;
					sum += (vertexA.q.b[row] + vertexB.q.b[row]) * mInvert[rowOffset + col];
				}
				cost -= sum * (vertexA.q.b[col] + vertexB.q.b[col]);
			}
		}

		// free matrix memory
		delete[] mInvert;
	}
	if(!optPosSuccess){
		// matrix is not invertible => get best position of v1, v2 and (v1+v2)/2
		Quadric<float> sumQ(vertexA.q);
		sumQ += vertexB.q;

		std::vector<float> sumData;
		sumData.reserve(dataSize);
		for (std::size_t i = 0; i < dataSize; ++i) {
			sumData.push_back(0.5f * (vertexA.data[i] + vertexB.data[i]));
		}
		float costV1 = sumQ.getCost(vertexA.data);
		float costV2 = sumQ.getCost(vertexB.data);
		float costV1V2div2 = sumQ.getCost(sumData);
		// use minimum of v1 and v2 as optimal position
		if(costV1<costV2 && costV1<costV1V2div2){
			cost = costV1;
			optPos = vertexA.data;
		}else if(costV2<costV1 && costV2<costV1V2div2){
			cost = costV2;
			optPos = vertexB.data;
		} else{
			cost = costV1V2div2;
			optPos = sumData;
		}
	}
	return cost;
}

Mesh * simplifyMesh(Mesh * mesh, uint32_t newNumberOfTriangles, float threshold, bool useOptimalPositioning, float maxAngle, const weights_t & weights) {
	if(mesh->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("Mesh simplification can only be done with triangle meshes.");
		return mesh;
	}
	if(mesh->getPrimitiveCount() <= newNumberOfTriangles) {
		WARN("Mesh already has less or equal as many triangles as requested.");
		return mesh;
	}
	Util::info<<"\nSimplifying mesh from "<<mesh->getPrimitiveCount()<<" to "<<newNumberOfTriangles<<" triangles; threshold: "<<threshold<<"; optPos: "<<useOptimalPositioning<<"\n";
	Util::info<<"Weights are: vertex="<<weights[0]<<" normal="<<weights[1]<<" color="<<weights[2]<<" tex0="<<weights[3]<<" boundary="<<weights[4]<<"\n";
	Util::ProgressIndicator progress("Simplify progress",(mesh->getPrimitiveCount()-newNumberOfTriangles)+mesh->getPrimitiveCount()+3*mesh->getVertexCount(), 2);
	Util::Timer timer;
	timer.reset();

	// initialize vertex array
	const Geometry::Box & meshBB = mesh->getBoundingBox();
	const float octreeBoxMax = std::max(std::max(meshBB.getMaxX(), meshBB.getMaxY()), meshBB.getMaxZ()) * weights[VERTEX_OFFSET];
	const float octreeBoxMin = std::min(std::min(meshBB.getMinX(), meshBB.getMinY()), meshBB.getMinZ()) * weights[VERTEX_OFFSET];
	const Geometry::Box octreeBox(octreeBoxMin, octreeBoxMax, octreeBoxMin, octreeBoxMax, octreeBoxMin, octreeBoxMax);
	threshold *= weights[VERTEX_OFFSET];
	Geometry::PointOctree<VertexPoint> * vertexOctree = nullptr;
	if(threshold > 0.0f) {
		vertexOctree = new Geometry::PointOctree<VertexPoint>(octreeBox, threshold, 100);
	}
	std::vector<vertex_t> vertices;
	const auto numDataEntries = initVertexArray(mesh, progress, weights, vertices, vertexOctree);
	if(numDataEntries == 0) {
		WARN("Vertex data does not contain readable information, or weights prevent the data usage.");
		return mesh;
	}

	// build heap
	Util::UpdatableHeap<float, heapData> heap;

	const uint32_t vertexCount = mesh->getVertexCount();
	MeshIndexData iData = mesh->openIndexData();

	// add boundary constraint planes and get neighbors
	{
		Quadric<float> tmpQ(numDataEntries);
		std::vector<float> v1(numDataEntries, 0.0f);
		std::vector<float> v2(numDataEntries, 0.0f);
		std::vector<float> v3(numDataEntries, 0.0f);
		for(unsigned int i=0; i<vertexCount; ++i){
			std::set<std::pair<unsigned int, unsigned int> > singleNeighbors;
			getNeighboursOfVertex(mesh, vertices[i], vertexOctree, threshold, vertices[i].neighbors, singleNeighbors);

			if(weights[BOUNDARY_OFFSET] && weights[VERTEX_OFFSET]){
				// make sure to add a boundary plan to only once to both vertices i and *singleNeighborIterator.first by checking first<i
				for(auto singleNeighborIterator=singleNeighbors.begin(); singleNeighborIterator->first<i && singleNeighborIterator!=singleNeighbors.end(); ++singleNeighborIterator){
					// plane normal has to be perpendicular to edge between single neighbors and to normal of face spanned by this edge
					// => plane has to lie in single neighbor vertices and one of those vertices+planeNormal
					const auto normal = calcNormal(vertices[iData[singleNeighborIterator->second + 0]].data.data(),
												   vertices[iData[singleNeighborIterator->second + 1]].data.data(),
												   vertices[iData[singleNeighborIterator->second + 2]].data.data());

					v1[0] = vertices[i].data[0];
					v1[1] = vertices[i].data[1];
					v1[2] = vertices[i].data[2];
					v2[0] = vertices[singleNeighborIterator->first].data[0];
					v2[1] = vertices[singleNeighborIterator->first].data[1];
					v2[2] = vertices[singleNeighborIterator->first].data[2];
					v3[0] = normal.getX() + vertices[i].data[0];
					v3[1] = normal.getY() + vertices[i].data[1];
					v3[2] = normal.getZ() + vertices[i].data[2];


					//getQuadric(&vertices[i].data[0], &vertices[singleNeighborIterator->first].data[0], v3, tmp_quadric, 4);
					getQuadric(v1, v2, v3, tmpQ);
					for(std::size_t j=3; j<numDataEntries; ++j)
						tmpQ.A.set(j, j, 0.0f);

					// add boundary constraint plane to both vertices that are single neighbors
					vertices[i].q += tmpQ;
					vertices[singleNeighborIterator->first].q += tmpQ;
				}
			}
			progress.increment();
			// singleNeighbors is destroyed here
		}
	}

	// build heap
	for(unsigned int i=0; i<vertexCount; ++i){
		// add unique neighbors to heap (by adding only pairs of (i,j) where i<j)
		for(auto it=vertices[i].neighbors.upper_bound(i); it!=vertices[i].neighbors.end(); ++it){
			heapData hd(i, *it);
			float cost = getOptimalPosition(vertices[i], vertices[*it], hd.optPos, numDataEntries, useOptimalPositioning);
			heapElement *h = heap.insert(cost, hd);
			vertices[i].inHeap.insert(h);
			vertices[*it].inHeap.insert(h);
		}
		progress.increment();
	}


	// merge vertices
	int flipcount = 0;
	std::unordered_set<unsigned int> indexTrash;
	std::vector<unsigned int> vertexTrash;
	// rough estimate of the number of vertices that will be removed
	vertexTrash.reserve((mesh->getPrimitiveCount() - newNumberOfTriangles) / 2);
	unsigned int newTriangleCount = mesh->getPrimitiveCount();

	{
		while(newTriangleCount>newNumberOfTriangles && heap.size()!=0 && heap.top()->getCost()!=DONT_MERGE_COST /*&& iteration<threshold*/){
			heapElement *heapHead = heap.top();
			heapData &topData = heapHead->data;
			std::set<heapElement*> heapTrash;

			if(maxAngle != -1) {
				// check if some normal flips
				bool normalFlip = false;
				// checking for vertex1.inIndex
				for(const auto & triIndex : vertices[topData.vertex1].inIndex) {
					if(!(iData[triIndex + 0] == topData.vertex2
						|| iData[triIndex + 1] == topData.vertex2
						|| iData[triIndex + 2] == topData.vertex2)) {
						// face will not be deleted

						// calculate normal before merging
						const auto normalBefore = calcNormal(vertices[iData[triIndex + 0]].data.data(), 
															 vertices[iData[triIndex + 1]].data.data(), 
															 vertices[iData[triIndex + 2]].data.data());
						if(normalBefore.isZero()) {
							normalFlip = true;
							break;
						}

						// calculate normal after merging
						Geometry::Vec3f normalAfter;
						if(iData[triIndex + 0] == topData.vertex1) {
							normalAfter = calcNormal(topData.optPos.data(), 
													 vertices[iData[triIndex + 1]].data.data(), 
													 vertices[iData[triIndex + 2]].data.data());
						} else if(iData[triIndex + 1] == topData.vertex1) {
							normalAfter = calcNormal(vertices[iData[triIndex + 0]].data.data(), 
													 topData.optPos.data(), 
													 vertices[iData[triIndex + 2]].data.data());
						} else {
							normalAfter = calcNormal(vertices[iData[triIndex + 0]].data.data(), 
													 vertices[iData[triIndex + 1]].data.data(), 
													 topData.optPos.data());
						}
						if(normalAfter.isZero()) {
							normalFlip = true;
							break;
						}

						// check if normal has flipped
						if(normalBefore.dot(normalAfter) < maxAngle) {
							normalFlip = true;
							break;
						}
					}
				}
				// checking for vertex2.inIndex
				for(const auto & triIndex : vertices[topData.vertex2].inIndex) {
					if(!(iData[triIndex + 0] == topData.vertex1
						|| iData[triIndex + 1] == topData.vertex1
						|| iData[triIndex + 2] == topData.vertex1)) {
						// face will not be deleted

						// calculate normal before merging
						const auto normalBefore = calcNormal(vertices[iData[triIndex + 0]].data.data(),
															 vertices[iData[triIndex + 1]].data.data(), 
															 vertices[iData[triIndex + 2]].data.data());
						if(normalBefore.isZero()) {
							normalFlip = true;
							break;
						}

						// calculate normal after merging
						Geometry::Vec3f normalAfter;
						if(iData[triIndex + 0] == topData.vertex2) {
							normalAfter = calcNormal(topData.optPos.data(), 
													 vertices[iData[triIndex + 1]].data.data(), 
													 vertices[iData[triIndex + 2]].data.data());
						} else if(iData[triIndex + 1] == topData.vertex2) {
							normalAfter = calcNormal(vertices[iData[triIndex + 0]].data.data(), 
													 topData.optPos.data(), 
													 vertices[iData[triIndex + 2]].data.data());
						} else {
							normalAfter = calcNormal(vertices[iData[triIndex + 0]].data.data(), 
													 vertices[iData[triIndex + 1]].data.data(), 
													 topData.optPos.data());
						}
						if(normalAfter.isZero()) {
							normalFlip = true;
							break;
						}

						// check if normal has flipped
						if(normalBefore.dot(normalAfter) < maxAngle) {
							normalFlip = true;
							break;
						}
					}
				}
				if(normalFlip){
					flipcount++;
					normalFlip = false;
					heap.update(heapHead, DONT_MERGE_COST);
					continue;
				}
			}

			// merge vertex1 and vertex2 into vertex1
			vertexTrash.push_back(topData.vertex2);

			// update data of vertex1 to data of merged vertex
			vertices[topData.vertex1].data = topData.optPos;

			// update indexData of vertex2 to vertex1 and inIndex of vertex1
			for(const auto & triIndex : vertices[topData.vertex2].inIndex) {
				std::array<uint32_t, 3> vertexIndices;
				vertexIndices[0] = iData[triIndex + 0];
				vertexIndices[1] = iData[triIndex + 1];
				vertexIndices[2] = iData[triIndex + 2];
				if(vertexIndices[0] == topData.vertex1 || vertexIndices[1] == topData.vertex1 || vertexIndices[2] == topData.vertex1) {
					// triangle uses vertex1 and vertex2 => no triangle after merging
					if(indexTrash.insert(triIndex).second) {
						// face was inserted => was not deleted before
						for(const auto & vertexIndex : vertexIndices) {
							if(vertexIndex != topData.vertex2) {
								vertices[vertexIndex].inIndex.erase(std::remove(vertices[vertexIndex].inIndex.begin(), vertices[vertexIndex].inIndex.end(), triIndex),
																	vertices[vertexIndex].inIndex.end());
							}
						}

						--newTriangleCount;
						progress.increment();
					}
				} else {
					// only vertex2 is used in this triangle => update vertex2 to vertex1
					if(vertexIndices[0] == topData.vertex2) {
						iData[triIndex + 0] = topData.vertex1;
					}
					if(vertexIndices[1] == topData.vertex2) {
						iData[triIndex + 1] = topData.vertex1;
					}
					if(vertexIndices[2] == topData.vertex2) {
						iData[triIndex + 2] = topData.vertex1;
					}

					vertices[topData.vertex1].inIndex.push_back(triIndex);

					// Make inIndex unique again.
					make_unique(vertices[topData.vertex1].inIndex);
				}
			}

			// update heap by replacing vertex2 by vertex1
			for(auto & elem : vertices[topData.vertex2].inHeap) {
				// update heapHead->vertex2 in heapElement to heapHead->vertex1 and make sure that heapElements are unique
				if(elem->data.vertex1==topData.vertex2) {
					// update vertex1 of heapElement
					if(vertices[topData.vertex1].neighbors.insert(elem->data.vertex2).second) {
						// new neighbor has been added => update heap normally
						vertices[elem->data.vertex2].neighbors.insert(topData.vertex1);
						vertices[topData.vertex1].inHeap.insert(elem);
						elem->data.vertex1 = topData.vertex1;
					} else {
						// neighbors already existed => delete this heapElement so preserve uniqueness
						heapTrash.insert(elem);
					}
				}else{
					// update vertex2 of heapElement
					if(vertices[topData.vertex1].neighbors.insert(elem->data.vertex1).second) {
						// new neighbor has been added => update heap normally
						vertices[elem->data.vertex1].neighbors.insert(topData.vertex1);
						vertices[topData.vertex1].inHeap.insert(elem);
						elem->data.vertex2 = topData.vertex1;
					} else {
						// neighbors already existed => delete this heapElement so preserve uniqueness
						heapTrash.insert(elem);
					}
				}
			}

			// update matrix q=(vertex1.q+vertex2.q) of vertex1
			vertices[topData.vertex1].q += vertices[topData.vertex2].q;

			// update cost of merge with other neighbors
			for(auto & elem : vertices[topData.vertex1].inHeap) {
				// only update heapElement if it will not be deleted
				if(heapTrash.count(elem) == 0) {
					float cost = getOptimalPosition(vertices[elem->data.vertex1], vertices[elem->data.vertex2], elem->data.optPos, numDataEntries, useOptimalPositioning);
					heap.update(elem, cost);
				}
			}

			// delete heapElements in heapTrash (this is including heapHead)
			for(auto & elem : heapTrash) {
				vertices[elem->data.vertex1].inHeap.erase(elem);
				vertices[elem->data.vertex2].inHeap.erase(elem);
				heap.erase(elem);
			}
		}
	}

	if(heap.size()!=0 && heap.top()->getCost()==DONT_MERGE_COST){
		WARN("Could not merge any more due to constraints.");
	}

	// write vertex data
	MeshVertexData vertexData = mesh->openVertexData();
	{
		// there should be no duplicates, but the function sorts the array
		// sorted array is needed for binary_search below
		make_unique(vertexTrash);

		Util::Reference<PositionAttributeAccessor> positionAccessor;
		if(weights[VERTEX_OFFSET] > 0) {
			try {
				positionAccessor = PositionAttributeAccessor::create(vertexData, VertexAttributeIds::POSITION);
			} catch(...) {
			}
		}
		Util::Reference<NormalAttributeAccessor> normalAccessor;
		if(weights[NORMAL_OFFSET] > 0) {
			try {
				normalAccessor = NormalAttributeAccessor::create(vertexData, VertexAttributeIds::NORMAL);
			} catch(...) {
			}
		}
		Util::Reference<ColorAttributeAccessor> colorAccessor;
		if(weights[COLOR_OFFSET] > 0) {
			try {
				colorAccessor = ColorAttributeAccessor::create(vertexData, VertexAttributeIds::COLOR);
			} catch(...) {
			}
		}
		Util::Reference<TexCoordAttributeAccessor> texCoordAccessor;
		if(weights[TEX0_OFFSET] > 0) {
			try {
				texCoordAccessor = TexCoordAttributeAccessor::create(vertexData, VertexAttributeIds::TEXCOORD0);
			} catch(...) {
			}
		}

		for(unsigned int v = 0; v < vertexCount; ++v) {
			const bool vertexDeleted = std::binary_search(vertexTrash.begin(), vertexTrash.end(), v);
			if(vertexDeleted) {
				// skip vertex
				continue;
			}
			auto dataIt = vertices[v].data.cbegin();

			if(positionAccessor.isNotNull()) {
				Geometry::Vec3f position;
				position.setX(*dataIt++);
				position.setY(*dataIt++);
				position.setZ(*dataIt++);
				positionAccessor->setPosition(v, position / weights[VERTEX_OFFSET]);
			}
			if(normalAccessor.isNotNull()) {
				Geometry::Vec3f normal;
				normal.setX(*dataIt++);
				normal.setY(*dataIt++);
				normal.setZ(*dataIt++);
				const auto length = normal.length();
				if(length > 1.0e-6f) {
					normal /= length;
				}
				normalAccessor->setNormal(v, normal);
			}
			if(colorAccessor.isNotNull()) {
				Util::Color4f color;
				color.setR(*dataIt++);
				color.setG(*dataIt++);
				color.setB(*dataIt++);
				color.setA(*dataIt++);
				colorAccessor->setColor(v, color / weights[COLOR_OFFSET]);
			}
			if(texCoordAccessor.isNotNull()) {
				Geometry::Vec2f coordinate;
				coordinate.setX(*dataIt++);
				coordinate.setY(*dataIt++);
				texCoordAccessor->setCoordinate(v, coordinate / weights[TEX0_OFFSET]);
			}
		}
	}

	// copy indices to newMesh deleting/skipping indexTrash
	MeshIndexData indexData;
	{
		uint32_t newIndexCount = mesh->getIndexCount() - indexTrash.size() * 3;
		indexData.allocate(newIndexCount);

		uint32_t * indexPointer = indexData.data();
		for(unsigned int i = 0; i < iData.getIndexCount(); i += 3) {
			const bool indexDeleted = (indexTrash.count(i) > 0);
			if(!indexDeleted) {
				// copy index
				std::copy(iData.data() + i, iData.data() + i + 3, indexPointer);
				indexPointer += 3;
			}
		}
	}

	Util::Reference<Mesh> newMesh = new Mesh(std::move(indexData), std::move(vertexData));

	// copy vertices to newMesh deleting unused vertices
	Mesh * returnMesh = MeshUtils::eliminateUnusedVertices(newMesh.get());

	newMesh = nullptr;

	timer.stop();
	std::cout<<"time needed[ms]: "<< timer.getMilliseconds() <<"; "<<flipcount<<" flips\n";

	return returnMesh;
}

}
}
}
