/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "QuadtreeMeshBuilder.h"
#include "MeshBuilder.h"

#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>

#include <Util/Graphics/PixelAccessor.h>

#include <limits>
#include <map>

#ifndef NDEBUG
#define NDEBUG
#endif /* NDEBUG */

namespace Rendering {
namespace MeshUtils {

using namespace Util;
using namespace std;

QuadtreeMeshBuilder::QuadTree::QuadTree(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height) :
	children(),
	neighbors(),
	parent(nullptr),
	x(_x),
	y(_y),
	width(_width),
	height(_height) {

	neighbors.WEST = nullptr;
	neighbors.NORTH = nullptr;
	neighbors.EAST = nullptr;
	neighbors.SOUTH = nullptr;
}

QuadtreeMeshBuilder::QuadTree::QuadTree(QuadtreeMeshBuilder::QuadTree* _parent, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height) :
	children(),
	neighbors(),
	parent(_parent),
	x(_x),
	y(_y),
	width(_width),
	height(_height) {

	neighbors.WEST = nullptr;
	neighbors.NORTH = nullptr;
	neighbors.EAST = nullptr;
	neighbors.SOUTH = nullptr;
}

QuadtreeMeshBuilder::QuadTree::~QuadTree() = default;

bool QuadtreeMeshBuilder::QuadTree::split() {
	if (!isLeaf()) {
		cerr << " (inner) BAD !!! \n";
		return false; // current node has been already split
	}

	if (width == 1 && height == 1) {
		return false; // no need for further split (representing a single pixel)
	}

	const uint16_t width1 = width - width / 2u;
	const uint16_t height1 = height - height / 2u;

	// create (maximum) four children (sometimes a quad-tree node can only contain the children NW and NE, or NW and SW)
	children.NW.reset(new QuadTree(this, x, y, width1, height1));
	if (width > 1) {
		children.NE.reset(new QuadTree(this, x+width1, y, width-width1, height1));
	}
	if (height > 1) {
		children.SW.reset(new QuadTree(this, x, y+height1, width1, height-height1));
	}
	if (width > 1 && height > 1) {
		children.SE.reset(new QuadTree(this, x+width1, y+height1, width-width1, height-height1));
	}

	// rearrange the neighbors and do balancing where necessary
	arrangeNeighbors();

	return true;
}

void QuadtreeMeshBuilder::QuadTree::arrangeNeighbors() {
	QuadTree * west = neighbors.WEST;
	QuadTree * north = neighbors.NORTH;
	QuadTree * east = neighbors.EAST;
	QuadTree * south = neighbors.SOUTH;

	QuadTree * nw = children.NW.get();
	QuadTree * ne = children.NE.get();
	QuadTree * sw = children.SW.get();
	QuadTree * se = children.SE.get();


	// west side
	if (west && west->getHeight() > height) { // west must be split
		if (west->isLeaf()) {
			west->split();
		}
		QuadTree * neighbor = (parent != nullptr && parent->children.NW.get() == this) ? west->children.NE.get() : west->children.SE.get();
		nw->neighbors.WEST = neighbor;
		if (sw != nullptr)
			sw->neighbors.WEST = neighbor;
	} else if (west && !west->isLeaf()) {
		makeHorizontalNeighbors(west->children.NE.get(), nw);
		if (sw != nullptr) {
			makeHorizontalNeighbors(west->children.SE.get(), sw);
		}
	} else {
		nw->neighbors.WEST = west;
		if (sw != nullptr) {
			sw->neighbors.WEST = west;
		}
	}

	// north side
	if (north && north->getWidth() > width) { // north must be split
		if (north->isLeaf()) {
			north->split();
		}
		QuadTree * neighbor = (parent != nullptr && parent->children.NW.get() == this) ? north->children.SW.get() : north->children.SE.get();
		nw->neighbors.NORTH = neighbor;
		if (ne != nullptr)
			ne->neighbors.NORTH = neighbor;
	} else if (north && !north->isLeaf()) {
		makeVerticalNeighbors(north->children.SW.get(), nw);
		if (ne != nullptr) {
			makeVerticalNeighbors(north->children.SE.get(), ne);
		}
	} else {
		nw->neighbors.NORTH = north;
		if (ne != nullptr) {
			ne->neighbors.NORTH = north;
		}
	}

	// east side
	if (east && east->getHeight() > height) { // east must be split
		if (east->isLeaf()) {
			east->split();
		}
		QuadTree * neighbor = (parent != nullptr && parent->children.NE.get() == this) ? east->children.NW.get() : east->children.SW.get();
		if (ne == nullptr) {
			nw->neighbors.EAST = neighbor;
			sw->neighbors.EAST = neighbor;
		} else {
			ne->neighbors.EAST = neighbor;
			if (se != nullptr) {
				se->neighbors.EAST = neighbor;
			}
		}
	} else if (east && !east->isLeaf()) {
		if (ne == nullptr) {
			makeHorizontalNeighbors(nw, east->children.NW.get());
			makeHorizontalNeighbors(sw, east->children.SW.get());
		} else {
			makeHorizontalNeighbors(ne, east->children.NW.get());
			if (se != nullptr) {
				makeHorizontalNeighbors(se, east->children.SW.get());
			}
		}
	} else {
		if (ne == nullptr) {
			nw->neighbors.EAST = east;
			sw->neighbors.EAST = east;
		} else {
			ne->neighbors.EAST = east;
			if (se != nullptr) {
				se->neighbors.EAST = east;
			}
		}
	}

	// south side
	if (south && south->getWidth() > width) { // south must be split
		if (south->isLeaf()) {
			south->split();
		}
		QuadTree * neighbor = (parent != nullptr && parent->children.SE.get() == this) ? south->children.NE.get() : south->children.NW.get();
		if (sw == nullptr) {
			nw->neighbors.SOUTH = neighbor;
			ne->neighbors.SOUTH = neighbor;
		} else {
			sw->neighbors.SOUTH = neighbor;
			if (se != nullptr)
				se->neighbors.SOUTH = neighbor;
		}
	} else if (south && !south->isLeaf()) {
		if (sw == nullptr) {
			makeVerticalNeighbors(nw, south->children.NW.get());
			makeVerticalNeighbors(ne, south->children.NE.get());
		} else {
			makeVerticalNeighbors(sw, south->children.NW.get());
			if (se != nullptr) {
				makeVerticalNeighbors(se, south->children.NE.get());
			}
		}
	} else {
		if (sw == nullptr) {
			nw->neighbors.SOUTH = south;
			ne->neighbors.SOUTH = south;
		} else {
			sw->neighbors.SOUTH = south;
			if (se != nullptr) {
				se->neighbors.SOUTH = south;
			}
		}
	}


	// arrange relation between the direct children
	if (ne == nullptr) {
		makeVerticalNeighbors(nw, sw);
	} else if (sw == nullptr) {
		makeHorizontalNeighbors(nw, ne);
	} else {
		makeHorizontalNeighbors(nw, ne);
		makeHorizontalNeighbors(sw, se);

		makeVerticalNeighbors(nw, sw);
		makeVerticalNeighbors(ne, se);
	}
}

void QuadtreeMeshBuilder::QuadTree::collectLeaves(deque<QuadTree *> & leaves) {
	if (isLeaf()) {
		leaves.push_back(this);
	} else {
		if (children.NW) {
			children.NW->collectLeaves(leaves);
		}
		if (children.NE) {
			children.NE->collectLeaves(leaves);
		}
		if (children.SW) {
			children.SW->collectLeaves(leaves);
		}
		if (children.SE) {
			children.SE->collectLeaves(leaves);
		}
	}
}

uint8_t QuadtreeMeshBuilder::QuadTree::collectVertices(std::vector<QuadtreeMeshBuilder::vertex_t> & vertices) const {
	uint8_t pattern = 0x00;

	const uint16_t widthHalf = width - width / 2u;
	const uint16_t heightHalf = height - height / 2u;
	const uint16_t xHalf = x + widthHalf;
	const uint16_t yHalf = y + heightHalf;
	const uint16_t xFull = x + width;
	const uint16_t yFull = y + height;
	// South-West corner
	vertices.emplace_back(x, yFull);
	{ // West side
		QuadTree * neighbor = neighbors.WEST;
		if (neighbor != nullptr && !neighbor->isLeaf()) {
			vertices.emplace_back(x, yHalf);
			pattern |= 0x01;
		}
	}
	// North-west corner
	vertices.emplace_back(x, y);
	{ // North side
		QuadTree *neighbor = neighbors.NORTH;
		if (neighbor && !neighbor->isLeaf()) {
			vertices.emplace_back(xHalf, y);
			pattern |= 0x02;
		}
	}
	// North-east corner
	vertices.emplace_back(xFull, y);
	{ // East side
		QuadTree * neighbor = neighbors.EAST;
		if (neighbor && !neighbor->isLeaf()) {
			vertices.emplace_back(xFull, yHalf);
			pattern |= 0x04;
		}
	}
	// South-East corner
	vertices.emplace_back(xFull, yFull);
	{ // South side
		QuadTree * neighbor = neighbors.SOUTH;
		if (neighbor && !neighbor->isLeaf()) {
			vertices.emplace_back(xHalf, yFull);
			pattern |= 0x08;
		}
	}
	return pattern;
}

// ############################################# SplitFunction #########################################################

QuadtreeMeshBuilder::DepthSplitFunction::DepthSplitFunction(Util::Reference<Util::PixelAccessor> depthAccessor, float depthDisruption) :
		depth(std::move(depthAccessor)),
		minDepth(std::numeric_limits<float>::max()),
		maxDepth(std::numeric_limits<float>::lowest()),
		disruptionFactor(depthDisruption) {
	if(depth.isNull()) {
		throw std::invalid_argument("No access to depth values.");
	}
	const uint16_t texWidth = depth->getWidth();
	const uint16_t texHeight = depth->getHeight();
	for (uint_fast16_t x = 0; x < texWidth; ++x) {
		for (uint_fast16_t y = 0; y < texHeight; ++y) {
			const float current = depth->readSingleValueFloat(x, y);
			if (current < minDepth) {
				minDepth = current;
			}
			if (current > maxDepth) {
				maxDepth = current;
			}
		}
	}
}

bool QuadtreeMeshBuilder::DepthSplitFunction::operator()(QuadtreeMeshBuilder::QuadTree * node) {
	const uint16_t xMin = node->getX();
	const uint16_t yMin = node->getY();
	const uint16_t xMax = node->getWidth() + xMin;
	const uint16_t yMax = node->getHeight() + yMin;

	const float minDisruption = disruptionFactor * (maxDepth - minDepth);
	// If there is a continuous change of depth values, then do not split.
	// If there is a large disruption of depth values, then split.
	for (uint_fast16_t y = yMin; y < yMax; ++y) {
		float rowDeltaMax = 0.0f;
		for (uint_fast16_t x = xMin + 1u; x < xMax; ++x) {
			const float current = depth->readSingleValueFloat(x, y);
			const float before = depth->readSingleValueFloat(x - 1, y);
			const float delta = std::abs(before - current);
			if (delta > rowDeltaMax) {
				rowDeltaMax = delta;
			}
		}
		// Check if there is a large disruption in this row.
		if (rowDeltaMax > minDisruption) {
			return true;
		}
	}
	for (uint_fast16_t x = xMin; x < xMax; ++x) {
		float columnDeltaMax = 0.0f;
		for (uint_fast16_t y = yMin + 1u; y < yMax; ++y) {
			const float current = depth->readSingleValueFloat(x, y);
			const float before = depth->readSingleValueFloat(x, y - 1);
			const float delta = std::abs(before - current);
			if (delta > columnDeltaMax) {
				columnDeltaMax = delta;
			}
		}
		// Check if there is a large disruption in this column.
		if (columnDeltaMax > minDisruption) {
			return true;
		}
	}

	return false;
}

QuadtreeMeshBuilder::ColorSplitFunction::ColorSplitFunction(Util::Reference<Util::PixelAccessor> colorAccessor) :
		color(std::move(colorAccessor)) {
	if(color.isNull()) {
		throw std::invalid_argument("No access to color values.");
	}
}

bool QuadtreeMeshBuilder::ColorSplitFunction::operator()(QuadtreeMeshBuilder::QuadTree * node) {
	const uint16_t xMin = node->getX();
	const uint16_t yMin = node->getY();
	const uint16_t xMax = node->getWidth() + xMin;
	const uint16_t yMax = node->getHeight() + yMin;

	const uint16_t minDisruption = 255;
	// If there is a continuous change of color values, then do not split.
	// If there is a large disruption of color values, then split.
	for (uint_fast16_t y = yMin; y < yMax; ++y) {
		uint16_t rowDeltaMax = 0;
		for (uint_fast16_t x = xMin + 1u; x < xMax; ++x) {
			const Util::Color4ub current = color->readColor4ub(x, y);

			const Util::Color4ub before = color->readColor4ub(x - 1, y);
			const Util::Color4ub diffColor = Util::Color4ub::createDifferenceColor(before, current);
			const uint16_t delta = diffColor.getR() + diffColor.getG() + diffColor.getB() + diffColor.getA();
			if (delta > rowDeltaMax) {
				rowDeltaMax = delta;
			}
		}
		// Check if there is a large disruption in this row.
		if (rowDeltaMax > minDisruption) {
			return true;
		}
	}
	for (uint_fast16_t x = xMin; x < xMax; ++x) {
		float columnDeltaMax = 0.0f;
		for (uint_fast16_t y = yMin + 1u; y < yMax; ++y) {
			const Util::Color4ub current = color->readColor4ub(x, y);

			const Util::Color4ub before = color->readColor4ub(x, y - 1);
			const Util::Color4ub diffColor = Util::Color4ub::createDifferenceColor(before, current);
			const uint16_t delta = diffColor.getR() + diffColor.getG() + diffColor.getB() + diffColor.getA();
			if (delta > columnDeltaMax) {
				columnDeltaMax = delta;
			}
		}
		// Check if there is a large disruption in this column.
		if (columnDeltaMax > minDisruption) {
			return true;
		}
	}
	return false;
}

QuadtreeMeshBuilder::StencilSplitFunction::StencilSplitFunction(Util::Reference<Util::PixelAccessor> stencilAccessor) :
		stencil(std::move(stencilAccessor)) {
	if(stencil.isNull()) {
		throw std::invalid_argument("No access to stencil values.");
	}
}

bool QuadtreeMeshBuilder::StencilSplitFunction::operator()(QuadtreeMeshBuilder::QuadTree * node) {
	const uint16_t xMin = node->getX();
	const uint16_t yMin = node->getY();
	const uint16_t xMax = node->getWidth() + xMin;
	const uint16_t yMax = node->getHeight() + yMin;

	// If there is a disruption of stencil values, then split.
	for (uint_fast16_t y = yMin; y < yMax; ++y) {
		for (uint_fast16_t x = xMin + 1u; x < xMax; ++x) {
			const uint8_t current = stencil->readSingleValueByte(x, y);
			const uint8_t before = stencil->readSingleValueByte(x - 1, y);
			if (current != before) {
				return true;
			}
		}
	}
	for (uint_fast16_t x = xMin; x < xMax; ++x) {
		for (uint_fast16_t y = yMin + 1u; y < yMax; ++y) {
			const uint8_t current = stencil->readSingleValueByte(x, y);
			const uint8_t before = stencil->readSingleValueByte(x, y - 1);
			if (current != before) {
				return true;
			}
		}
	}
	return false;
}

// ############################################## QuadtreeMeshBuilder ###################################################

static const uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();
static void addTriangle(MeshBuilder & builder, uint32_t a, uint32_t b, uint32_t c) {
	if(a != INVALID_INDEX && b != INVALID_INDEX && c != INVALID_INDEX) {
		builder.addTriangle(a, b, c);
	}
}

static void buildFaceTypeA(MeshBuilder & builder, const vector<uint32_t> & indices) {
	addTriangle(builder, indices[0], indices[1], indices[3]);
	addTriangle(builder, indices[1], indices[2], indices[3]);
}

static void buildFaceTypeB(MeshBuilder & builder, const vector<uint32_t> & indices, uint32_t basis) {
	addTriangle(builder, indices[basis], indices[(basis+1)%5], indices[(basis+4)%5]);
	addTriangle(builder, indices[(basis+1)%5], indices[(basis+2)%5], indices[(basis+3)%5]);
	addTriangle(builder, indices[(basis+3)%5], indices[(basis+4)%5], indices[(basis+1)%5]);
}

static void buildFaceTypeC(MeshBuilder & builder, const vector<uint32_t> & indices, uint32_t basis) {
	addTriangle(builder, indices[basis], indices[(basis+1)%6], indices[(basis+5)%6]);
	addTriangle(builder, indices[(basis+1)%6], indices[(basis+2)%6], indices[(basis+3)%6]);
	addTriangle(builder, indices[(basis+3)%6], indices[(basis+4)%6], indices[(basis+5)%6]);
	addTriangle(builder, indices[(basis+1)%6], indices[(basis+3)%6], indices[(basis+5)%6]);
}

static void buildFaceTypeD(MeshBuilder & builder, const vector<uint32_t> & indices, uint32_t basis) {
	addTriangle(builder, indices[basis], indices[(basis+1)%6], indices[(basis+5)%6]);
	addTriangle(builder, indices[(basis+1)%6], indices[(basis+2)%6], indices[(basis+4)%6]);
	addTriangle(builder, indices[(basis+2)%6], indices[(basis+3)%6], indices[(basis+4)%6]);
	addTriangle(builder, indices[(basis+1)%6], indices[(basis+4)%6], indices[(basis+5)%6]);
}

static void buildFaceTypeE(MeshBuilder & builder, const vector<uint32_t> & indices, uint32_t basis) {
	addTriangle(builder, indices[basis], indices[(basis+1)%7], indices[(basis+6)%7]);
	addTriangle(builder, indices[(basis+1)%7], indices[(basis+2)%7], indices[(basis+6)%7]);
	addTriangle(builder, indices[(basis+2)%7], indices[(basis+3)%7], indices[(basis+4)%7]);
	addTriangle(builder, indices[(basis+4)%7], indices[(basis+5)%7], indices[(basis+6)%7]);
	addTriangle(builder, indices[(basis+2)%7], indices[(basis+4)%7], indices[(basis+6)%7]);
}

static void buildFaceTypeF(MeshBuilder & builder, const vector<uint32_t> & indices) {
	addTriangle(builder, indices[0], indices[1], indices[7]);
	addTriangle(builder, indices[1], indices[2], indices[3]);
	addTriangle(builder, indices[3], indices[4], indices[5]);
	addTriangle(builder, indices[5], indices[6], indices[7]);
	addTriangle(builder, indices[3], indices[5], indices[7]);
	addTriangle(builder, indices[1], indices[3], indices[7]);
}

Mesh * QuadtreeMeshBuilder::createMesh(const VertexDescription& vd,
										Util::Reference<PixelAccessor> depthReader,
										Util::Reference<PixelAccessor> colorReader,
										Util::Reference<PixelAccessor> normalReader,
										Util::Reference<PixelAccessor> stencilReader,
										QuadtreeMeshBuilder::split_function_t function) {
	// 0A: create the pixel-accessors for the textures
	if( depthReader.isNull()){
		WARN("No depth reader given.");
		return nullptr;
	}

	// 0B: get the width and height
	const uint16_t width  = static_cast<uint16_t>(depthReader->getWidth()) - 1;
	const uint16_t height = static_cast<uint16_t>(depthReader->getHeight()) - 1;

	// 1: create queue used to build up a quad-tree (it usually contains the leaves, but could also contain some inner nodes)
	deque<QuadTree*> quadtrees;

	// 2: create the root quad-tree and add it to the queue
	QuadTree root(0, 0, width, height);
	quadtrees.push_back(&root);

	// 3: as long as there are further leaf-nodes
	while (!quadtrees.empty()) {
		QuadTree* quadtree = quadtrees.front();
		quadtrees.pop_front();

		if (!quadtree->isLeaf()) { // node has been already split during balancing
			quadtree->collectLeaves(quadtrees);
			continue;
		}

		// split the quadtree if necessary
		if (function(quadtree)) {
			if (quadtree->split()) {
				quadtree->collectLeaves(quadtrees);
			}
		}
	}


	// 4: balancing is implicitly done during every splitting-step

	// 5: create the mesh
	MeshBuilder builder(vd);
	const float xScale = 2.0f / static_cast<float>(width);
	const float yScale = 2.0f / static_cast<float>(height);
	const float uScale = 1.0f / static_cast<float>(width);
	const float vScale = 1.0f / static_cast<float>(height);

	// 5-A: collect the quadtree-leaves
	deque<QuadTree*> leaves;
	root.collectLeaves(leaves);

#ifndef NDEBUG
	createDebugOutput(leaves, depthReader.get(), colorReader.get());
#endif

	// 5-B: as long as there are leaves
	map<vertex_t, uint32_t> indexMap; // index map containing indices to already created vertices
	vector<uint32_t> indices;
	vector<vertex_t> vertices;
	indices.reserve(8);
	vertices.reserve(8);


	while (!leaves.empty()) {
		QuadTree * quadtree = leaves.front();
		leaves.pop_front();

		indices.clear();
		vertices.clear();

		uint8_t pattern = 0x00; // pattern coding the side containing additional vertex with "1"

		// determine the pattern and collect indices
		pattern = quadtree->collectVertices(vertices);
		for(const auto & vertex : vertices) {
			const uint16_t x = vertex.first;
			const uint16_t y = vertex.second;
			auto lb = indexMap.lower_bound(vertex);
			if(lb != indexMap.end() && !(indexMap.key_comp()(vertex, lb->first))) {
				indices.push_back(lb->second); // try to get an index of the specified vertex
			} else if(stencilReader.isNotNull() && stencilReader->readSingleValueByte(x, y) == 0) {
				// Generate a dummy vertex only, because the pixel belongs to the background
				indexMap.insert(lb, std::make_pair(vertex, INVALID_INDEX));
				indices.push_back(INVALID_INDEX);
			} else {
				// create new position
				const float depthValue = depthReader->readSingleValueFloat(x, y);

				builder.position(Geometry::Vec3(xScale * x - 1.0f, yScale * y - 1.0f, 2.0f * depthValue - 1.0f));

				if(colorReader.isNotNull()) {
					builder.color(colorReader->readColor4f(x, y));
				}

				if(normalReader.isNotNull()) {
					const Util::Color4ub normalColor = normalReader->readColor4ub(x, y);
					const Geometry::Vec3b normal(normalColor.getR() - 128, normalColor.getG() - 128, normalColor.getB() - 128);
					builder.normal(normal);
				}

				builder.texCoord0(Geometry::Vec2(x * uScale, y * vScale));

				const uint32_t index = builder.addVertex();
				indexMap.insert(lb, std::make_pair(vertex, index)); // insert into the indexMap
				indices.push_back(index);
			}
		}

		switch (pattern) {
			case 0: // 0000
				buildFaceTypeA(builder, indices);
				break;
			case 1: // 0001
				buildFaceTypeB(builder, indices, 0);
				break;
			case 2: // 0010
				buildFaceTypeB(builder, indices, 1);
				break;
			case 4: // 0100
				buildFaceTypeB(builder, indices, 2);
				break;
			case 8: // 1000
				buildFaceTypeB(builder, indices, 3);
				break;
			case 3: // 0011
				buildFaceTypeC(builder, indices, 0);
				break;
			case 6: // 0110
				buildFaceTypeC(builder, indices, 1);
				break;
			case 12: // 1100
				buildFaceTypeC(builder, indices, 0);
				break;
			case 9: // 1001
				buildFaceTypeC(builder, indices, 0);
				break;
			case 5: // 0101
				buildFaceTypeD(builder, indices, 0);
				break;
			case 10: // 1010
				buildFaceTypeD(builder, indices, 1);
				break;
			case 7: // 0111
				buildFaceTypeE(builder, indices, 6);
				break;
			case 11: // 1011
				buildFaceTypeE(builder, indices, 4);
				break;
			case 13: // 1101
				buildFaceTypeE(builder, indices, 2);
				break;
			case 14: // 1110
				buildFaceTypeE(builder, indices, 0);
				break;
			case 15: // 1111
				buildFaceTypeF(builder, indices);
				break;
			default:
				WARN("Invalid pattern.");
				break;
		}
	}

	return builder.buildMesh();
}

}
}
