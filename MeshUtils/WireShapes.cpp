/*
	This file is part of the Rendering library.
	Copyright (C) 2019 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "WireShapes.h"

#include "MeshBuilder.h"
#include "MeshUtils.h"
#include "../Mesh/Mesh.h"

#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Geometry/Rect.h>
#include <Geometry/Convert.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Sphere.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Frustum.h>
#include <Geometry/SRT.h>

#include <Util/Graphics/Color.h>

#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923
#endif

#define RESTART 0xffffffffu

namespace Rendering {
namespace MeshUtils {
namespace WireShapes {
	using namespace Geometry;
	
	static void addIndices(MeshBuilder& mb, const std::vector<uint32_t>& indices, uint32_t offset=0) {
		// TODO: handle line strips with restart index
		uint32_t pre = RESTART;
		for(auto i : indices) {
			if(pre != RESTART && i != RESTART) {
				mb.addIndex(offset+pre);
				mb.addIndex(offset+i);
			}
			pre = i;
		}
	}
	
	// ---------------------------------------------------------
	
	Mesh* createWireBox(const VertexDescription& vd, const Geometry::Box& box) {
		MeshBuilder mb(vd);
		addWireBox(mb, box);
		auto m = mb.buildMesh();
		m->setDrawMode(Mesh::DRAW_LINES);
		return m;
	}
	
	void addWireBox(MeshBuilder& mb, const Geometry::Box& box) {
		/*
		 *  Corners:
		 *     6---------7
		 *    /|        /|
		 *   / |       / |
		 *  2---------3  |
		 *  |  |      |  |
		 *  |  4------|--5
		 *  | /       | /
		 *  |/        |/
		 *  0---------1
		 */
		uint32_t offset = mb.getNextIndex();
		for(uint_fast8_t c = 0; c < 8; ++c) {
			mb.position(box.getCorner(static_cast<corner_t>(c)));
			mb.addVertex();
		}
		addIndices(mb, {0,2,3,1,5,7,6,4,0,RESTART,0,1,RESTART,3,7,RESTART,5,4,RESTART,6,2}, offset);
	}
	
	
	// ---------------------------------------------------------
	
	Mesh* createWireRectangle(const VertexDescription& vd, const Geometry::Rect& rect) {
		MeshBuilder mb(vd);
		addWireRectangle(mb, rect);
		auto m = mb.buildMesh();
		m->setDrawMode(Mesh::DRAW_LINES);
		return m;
	}
	
	void addWireRectangle(MeshBuilder& mb, const Geometry::Rect& rect) {
	 uint32_t offset = mb.getNextIndex();
		for(uint_fast8_t c = 0; c < 4; ++c) {
			mb.position(rect.getCorner(static_cast<rectCorner_t>(c)));
			mb.addVertex();
		}
		addIndices(mb, {0,1,3,2,0}, offset);
	}
	
	// ---------------------------------------------------------
	
	Mesh* createWireSphere(const VertexDescription& vd, const Geometry::Sphere& sphere, uint8_t numSegments) {
		MeshBuilder mb(vd);
		addWireSphere(mb, sphere, numSegments);
		auto m = mb.buildMesh();
		m->setDrawMode(Mesh::DRAW_LINES);
		return m;
	}
	
	void addWireSphere(MeshBuilder& mb, const Geometry::Sphere& sphere, uint8_t numSegments) {
		auto t = mb.getTransformation();
		SRT srt;
		srt.setTranslation(sphere.getCenter());
		mb.setTransformation(t * srt);
		addWireCircle(mb, sphere.getRadius(), numSegments);
		srt.setRotation({1,0,0}, {0,1,0});
		mb.setTransformation(t * srt);
		addWireCircle(mb, sphere.getRadius(), numSegments);
		srt.setRotation({0,1,0}, {1,0,0});
		mb.setTransformation(t * srt);
		addWireCircle(mb, sphere.getRadius(), numSegments);
		mb.setTransformation(t);
	}
	
	
	// ---------------------------------------------------------
	
	Mesh* createWireCircle(const VertexDescription& vd, float radius, uint8_t numSegments) {
		MeshBuilder mb(vd);
		addWireCircle(mb, radius, numSegments);
		auto m = mb.buildMesh();
		m->setDrawMode(Mesh::DRAW_LINES);
		return m;
	}
	
	void addWireCircle(MeshBuilder& mb, float radius, uint8_t numSegments) {
		const double TWO_PI = 2.0 * M_PI;
		const double step = TWO_PI / static_cast<double> (numSegments);
		uint32_t offset = mb.getNextIndex();
		std::vector<uint32_t> indices;
		for (uint_fast8_t segment = 0; segment < numSegments; ++segment) {
			const double segmentAngle = static_cast<double>(segment) * step;
			mb.position(Vec3(static_cast<float>(std::sin(segmentAngle)), static_cast<float>(std::cos(segmentAngle)), 0.0f) * radius);
			mb.addVertex();
			indices.emplace_back(segment);
		}
		indices.emplace_back(0);
		addIndices(mb, indices, offset);
	}
	
	
	// ---------------------------------------------------------
	
	Mesh* createWireFrustum(const VertexDescription& vd, const Geometry::Frustum& frustum) {
		MeshBuilder mb(vd);
		addWireFrustum(mb, frustum);
		auto m = mb.buildMesh();
		m->setDrawMode(Mesh::DRAW_LINES);
		return m;
	}
	
	void addWireFrustum(MeshBuilder& mb, const Geometry::Frustum& frustum) {
		uint32_t offset = mb.getNextIndex();
		for(uint_fast8_t c = 0; c < 8; ++c) {
			mb.position(frustum[static_cast<corner_t>(c)]);
			mb.addVertex();
		}
		addIndices(mb, {0,2,3,1,5,7,6,4,0,RESTART,0,1,RESTART,3,7,RESTART,5,4,RESTART,6,2}, offset);
	}
	
	
	// ---------------------------------------------------------
	
	Mesh* createLine(const VertexDescription& vd, const Geometry::Vec3& start, const Geometry::Vec3& end) {
		MeshBuilder mb(vd);
		addLine(mb, start, end);
		auto m = mb.buildMesh();
		m->setDrawMode(Mesh::DRAW_LINES);
		return m;
	}
	
	void addLine(MeshBuilder& mb, const Geometry::Vec3& start, const Geometry::Vec3& end) {
		uint32_t offset = mb.getNextIndex();
		mb.position(start);
		mb.addVertex();
		mb.position(end);
		mb.addVertex();
		addIndices(mb, {0,1}, offset);
	}
		
	// ---------------------------------------------------------
	
} /* WireShapes */
} /* MeshUtils */
} /* Rendering */