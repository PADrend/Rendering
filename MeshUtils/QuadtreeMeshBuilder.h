/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef QUADTREEMESHBUILDER_H_
#define QUADTREEMESHBUILDER_H_

#include <Util/References.h>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace Util {
class PixelAccessor;
}

namespace Rendering {
class VertexDescription;
class Mesh;
namespace MeshUtils {

/**
 * class QuadtreeMeshBuilder provides a static function for creating a mesh from the specified
 * depth-texture, color-texture and normal-texture.
 *
 */
class QuadtreeMeshBuilder {
public:
	typedef std::pair<uint16_t, uint16_t> vertex_t;

	/** quad tree used to subdivide the texture into areas */
	class QuadTree {
	private:
		/** array containing the pointers to the four children. */
		struct {
			std::unique_ptr<QuadTree> NW;
			std::unique_ptr<QuadTree> NE;
			std::unique_ptr<QuadTree> SW;
			std::unique_ptr<QuadTree> SE;
		} children;

		/** array containing the pointers to the neighbors */
		struct {
			QuadTree * WEST;
			QuadTree * NORTH;
			QuadTree * EAST;
			QuadTree * SOUTH;
		} neighbors;

		/** parent of current quad-tree node */
		QuadTree * parent;

		/** x-position of the first pixel */
		uint16_t x;

		/** y-position of the first pixel */
		uint16_t y;

		/** the width of the texture area stored in current node */
		uint16_t width;

		/** the height of the texture area */
		uint16_t height;


	private:
		QuadTree(const QuadTree &) = delete;
		QuadTree(QuadTree &&) = delete;
		QuadTree & operator=(const QuadTree &) = delete;
		QuadTree & operator=(QuadTree &&) = delete;
	public:
		/** [ctor] creates a QuadTree-root width specified x, y, width and height */
		QuadTree(uint16_t x, uint16_t y, uint16_t _width, uint16_t _height);

		/** [ctor] creates a QuadTree-node with specified parent, x, y, width and height */
		QuadTree(QuadTree * parent, uint16_t x, uint16_t y, uint16_t _width, uint16_t _height);

		/** [dtor] */
		~QuadTree();

		/**
		 * checks whether current quad-tree is leaf (has got no children)
		 * @return true if current quad-tree has no children, otherwise false
		 */
		inline bool isLeaf() const			{	return (!children.NW && !children.NE && !children.SW && !children.SE);	}

		inline uint16_t getWidth() const	{	return this->width;			}
		inline uint16_t getHeight() const	{	return this->height;		}
		inline uint16_t getX() const		{	return this->x;				}
		inline uint16_t getY() const 		{	return this->y;				}

		const QuadTree * getParent() const			{	return parent;	}

		const QuadTree * getWestNeighbor() const	{	return neighbors.WEST;	}
		const QuadTree * getNorthNeighbor() const	{	return neighbors.NORTH;	}
		const QuadTree * getEastNeighbor() const	{	return neighbors.EAST;	}
		const QuadTree * getSouthNeighbor() const	{	return neighbors.SOUTH;	}

		const QuadTree * getNorthWestChild() const	{	return children.NW.get();		}
		const QuadTree * getNorthEastChild() const	{	return children.NE.get();		}
		const QuadTree * getSouthWestChild() const	{	return children.SW.get();		}
		const QuadTree * getSouthEastChild() const	{	return children.SE.get();		}

		/**
		 * simply tries to split the current node into four smaller nodes
		 * @return true if splitting was successful, or false if the node has been already split
		 */
		bool split();

		/**
		 * collects all leaf-nodes from current node's subtree
		 * @param leaves : list to that all leaves will be collected
		 */
		void collectLeaves(std::deque<QuadTree *> & leaves);
		uint8_t collectVertices(std::vector<vertex_t> & vertices) const;

	private:
		/**
		 * arranges the neighbors and performs balancing the quadtree
		 */
		void arrangeNeighbors();

		static void makeHorizontalNeighbors(QuadTree * left, QuadTree * right) {
			if(left != nullptr) {
				left->neighbors.EAST = right;
			}
			if(right != nullptr) {
				right->neighbors.WEST = left;
			}
		}
		static void makeVerticalNeighbors(QuadTree * top, QuadTree * bottom) {
			if(top != nullptr) {
				top->neighbors.SOUTH = bottom;
			}
			if(bottom != nullptr) {
				bottom->neighbors.NORTH = top;
			}
		}
	};

	//! Type for all split functions.
	typedef std::function<bool (QuadTree *)> split_function_t;

	//! Split function that only uses the depth values.
	class DepthSplitFunction {
		public:
			/**
			 * Default constructor.
			 * The minimum and maximum depth values are initialized here.
			 *
			 * @param depthAccessor Access to the depth values
			 * @param depthDisruption This factor is multiplied with the depth range.
			 * If a difference larger than the result is found between two depth values, a split will be performed.
			 */
			DepthSplitFunction(Util::Reference<Util::PixelAccessor> depthAccessor, float depthDisruption);

			/**
			 * Determine whether the specified quad tree node shall be split.
			 * Only the depth values are used by this function.
			 *
			 * @param node Quad tree node that is to be analyzed
			 * @return @c true if the specified quad tree node should be split, @c false otherwise
			 */
			bool operator()(QuadTree * node);

		private:
			//! Access to the depth values.
			Util::Reference<Util::PixelAccessor> depth;
			//! Minimum depth of the whole texture.
			float minDepth;
			//! Maximum depth of the whole texture.
			float maxDepth;
			//! Determines the sensitivity of the split function
			float disruptionFactor;
	};

	//! Split function that only uses the color values.
	class ColorSplitFunction {
		public:
			/**
			 * @param colorAccessor Access to the color values
			 */
			ColorSplitFunction(Util::Reference<Util::PixelAccessor> colorAccessor);

			/**
			 * Determine whether the specified quad tree node shall be split.
			 * Only the color values are used by this function.
			 *
			 * @param node Quad tree node that is to be analyzed
			 * @return @c true if the specified quad tree node should be split, @c false otherwise
			 */
			bool operator()(QuadTree * node);

		private:
			//! Access to the color values.
			Util::Reference<Util::PixelAccessor> color;
	};

	//! Split function that only uses the stencil values.
	class StencilSplitFunction {
		public:
			/**
			 * @param stencilAccessor Access to the stencil values
			 */
			StencilSplitFunction(Util::Reference<Util::PixelAccessor> stencilAccessor);

			/**
			 * Determine whether the specified quad tree node shall be split.
			 * Only the stencil values are used by this function.
			 *
			 * @param node Quad tree node that is to be analyzed
			 * @return @c true if the specified quad tree node should be split, @c false otherwise
			 */
			bool operator()(QuadTree * node);

		private:
			//! Access to the stencil values.
			Util::Reference<Util::PixelAccessor> stencil;
	};

	//! Split function that uses multiple other split functions
	class MultipleSplitFunction {
		public:
			/**
			 * @param splitFunctions Array of split functions to use
			 */
			MultipleSplitFunction(std::deque<split_function_t>  splitFunctions) :
				functions(std::move(splitFunctions)) {
			}

			/**
			 * Determine whether the specified quad tree node shall be split.
			 * All split functions are used by this function.
			 *
			 * @param node Quad tree node that is to be analyzed
			 * @return @c true if the specified quad tree node should be split, @c false otherwise
			 */
			bool operator()(QuadTree * node) {
				for(auto & splitFunc : functions) {
					if(splitFunc(node)) {
						return true;
					}
				}
				return false;
			}

		private:
			std::deque<split_function_t> functions;
	};

	/**
	 * creates a mesh from the specified depth-texture, color-texture and normal-texture using a
	 * quad-tree.
	 *
	 * @param vd vertex description
	 * @param depthTexture containing the z-values for the vertices
	 *
	 * @param colorTexture (optional) containing color-data
	 * @param normalTexture (optional) containing normal-vectors
	 * @param stencilTexture (optional) Stencil values.
	 * If the stencil value of a pixel is zero, no vertices will be generated for that pixel.
	 * @param function split function determines whether a quad-tree node requires a split
	 * @return created mesh
	 */
	static Mesh * createMesh(const VertexDescription & vd,
							 Util::WeakPointer<Util::PixelAccessor> depthTexture,
							 Util::WeakPointer<Util::PixelAccessor> colorTexture,
							 Util::WeakPointer<Util::PixelAccessor> normalTexture,
							 Util::WeakPointer<Util::PixelAccessor> stencilTexture,
							 split_function_t function);

private:
	QuadtreeMeshBuilder() {}
	~QuadtreeMeshBuilder() {}

#ifndef NDEBUG
	static void createDebugOutput(const std::deque<QuadtreeMeshBuilder::QuadTree *> & leaves, Util::PixelAccessor * depth, Util::PixelAccessor * color);
#endif
};

}
}

#endif /* QUADTREEMESHBUILDER_H_ */
