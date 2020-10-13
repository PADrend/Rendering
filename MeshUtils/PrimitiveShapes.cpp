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
#include "PrimitiveShapes.h"

#include "MeshBuilder.h"
#include "MeshUtils.h"
#include "../Mesh/Mesh.h"

#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Geometry/Convert.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Sphere.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>

#include <Util/Graphics/Color.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Graphics/Bitmap.h>

#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923
#endif

namespace Rendering {
namespace MeshUtils {
  
using namespace Geometry;

// ---------------------------------------------------------
  
void addBox(MeshBuilder& mb, const Box& box) {
	uint32_t nextIndex = mb.getNextIndex();

	static const Vec2f uvs[4] = {
		Vec2f(0, 0), // lower-left
		Vec2f(1, 0), // lower-right
		Vec2f(1, 1), // upper-right
		Vec2f(0, 1), // upper-left
	};
	for (uint_fast8_t s = 0; s < 6; ++s) {
		const side_t side = static_cast<side_t>(s);
		const corner_t * corners = Helper::getCornerIndices(side);
		const Vec3 & normal = Helper::getNormal(side);
		for (uint_fast8_t v = 0; v < 4; ++v) {
			const Vec3 & corner = box.getCorner(corners[v]);
			mb.position(corner);
			mb.normal(normal);
			mb.texCoord0(uvs[v]);
			mb.addVertex();
		}
		mb.addQuad(nextIndex + 0, nextIndex + 1, nextIndex + 2, nextIndex + 3);
		nextIndex += 4;
	}
}

Mesh* createBox(const VertexDescription& vd, const Geometry::Box& box) {
  MeshBuilder mb(vd);
  addBox(mb, box);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addDome(MeshBuilder& mb, const double radius, const int horiRes, const int vertRes, const double halfSphereFraction, const double imagePercentage) {
	const double azimuth_step = 2.0 * M_PI / static_cast<double> (horiRes);
	const double elevation_step = halfSphereFraction * M_PI_2 / static_cast<double> (vertRes);
	//const uint32_t numVertices = (horiRes + 1) * (vertRes + 1);
	//const uint32_t numFaces = (2 * vertRes - 1) * horiRes;
  uint32_t idx = mb.getNextIndex();
  
	double azimuth = 0.0f;
	for (int k = 0; k <= horiRes; ++k) {
		double elevation = M_PI_2;
		for (int j = 0; j <= vertRes; ++j) {
      Vec3f v(cos(elevation) * sin(azimuth), sin(elevation), cos(elevation) * cos(azimuth));
      v *= radius;
      Vec2f uv(static_cast<float>(k) / static_cast<float>(horiRes), 1.0f - static_cast<float>(j) / static_cast<float>(vertRes) * imagePercentage);
      mb.position(v);
      mb.texCoord0(uv);
      mb.addVertex();
			elevation -= elevation_step;
		}
		azimuth += azimuth_step;
	}
  
	for (int k = 0; k < horiRes; ++k) {
    mb.addTriangle(
      idx + vertRes + 2 + (vertRes + 1) * k, 
      idx + 1 + (vertRes + 1) * k, 
      idx + (vertRes + 1) * k
    );
		for (int j = 1; j < vertRes; ++j) {
      mb.addTriangle( 
        idx + vertRes + 2 + (vertRes + 1) * k + j, 
        idx + 1 + (vertRes + 1) * k + j, 
        idx + (vertRes + 1) * k + j
      );
      mb.addTriangle( 
        idx + vertRes + 1 + (vertRes + 1) * k + j, 
        idx + vertRes + 2 + (vertRes + 1) * k + j, 
        idx + (vertRes + 1) * k + j
      );
		}
	}
}

Mesh* createDome(const VertexDescription& vd, const double radius, const int horiRes, const int vertRes, const double halfSphereFraction, const double imagePercentage) {
  MeshBuilder mb(vd);
  addDome(mb, radius, horiRes, vertRes, halfSphereFraction, imagePercentage);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addSphere(MeshBuilder& mb, const Sphere_f & sphere, uint32_t inclinationSegments, uint32_t azimuthSegments) {
	const uint32_t indexOffset = mb.getNextIndex();
	const double TWO_PI = 2.0 * M_PI;
	const double inclinationIncrement = M_PI / static_cast<double>(inclinationSegments);
	const double azimuthIncrement = TWO_PI / static_cast<double>(azimuthSegments);

	// Multiple "North Poles"
	mb.position(sphere.getCenter() + Vec3f(0.0f, sphere.getRadius(), 0.0f));
	mb.normal(Vec3f(0.0f, 1.0f, 0.0f));
	for(uint_fast32_t azimuth = 0; azimuth <= azimuthSegments; ++azimuth) {
		const double u = 1.0 - ((static_cast<double>(azimuth) + 0.5) / static_cast<double>(azimuthSegments));
		mb.texCoord0(Vec2f(u, 1.0f));
		mb.addVertex();
	}

	// Multiple "South Poles"
	mb.position(sphere.getCenter() + Vec3f(0.0f, -sphere.getRadius(), 0.0f));
	mb.normal(Vec3f(0.0f, -1.0f, 0.0f));
	for(uint_fast32_t azimuth = 0; azimuth <= azimuthSegments; ++azimuth) {
		const double u = 1.0 - (static_cast<double>(azimuth) / static_cast<double>(azimuthSegments));
		mb.texCoord0(Vec2f(u, 0.0f));
		mb.addVertex();
	}

	for(uint_fast32_t inclination = 1; inclination < inclinationSegments; ++inclination) {
		// This loop runs until azimuth equals azimuthSegments, because we need the same vertex positions with different texture coordinates.
		for(uint_fast32_t azimuth = 0; azimuth <= azimuthSegments; ++azimuth) {
			const double inclinationAngle = inclinationIncrement * static_cast<double>(inclination);
			const double azimuthAngle = azimuthIncrement * static_cast<double>(azimuth);
			const Vec3f position = Sphere_f::calcCartesianCoordinateUnitSphere(inclinationAngle, azimuthAngle);
			mb.position(sphere.getCenter() + position*sphere.getRadius());
			mb.normal(position);
			mb.texCoord0(Vec2f(
				1.0 - (static_cast<double>(azimuth) / static_cast<double>(azimuthSegments)),
				1.0 - (static_cast<double>(inclination) / static_cast<double>(inclinationSegments))));
			mb.addVertex();
		}
	}

	for(uint_fast32_t inclination = 1; inclination < inclinationSegments; ++inclination) {
		const uint32_t rowOffset = indexOffset + (inclination + 1) * (azimuthSegments + 1);
		for(uint_fast32_t azimuth = 0; azimuth < azimuthSegments; ++azimuth) {
			if(inclination == 1) {
				// Connect first row to north pole.
				const uint32_t northPoleIndex = indexOffset + azimuth;
				mb.addTriangle(northPoleIndex, rowOffset + azimuth + 1, rowOffset + azimuth);
			} else {
				mb.addQuad(
					rowOffset - (azimuthSegments + 1) + azimuth,
					rowOffset - (azimuthSegments + 1) + azimuth + 1,
					rowOffset + azimuth + 1,
					rowOffset + azimuth);
				if(inclination == inclinationSegments - 1) {
					// Connect last row to south pole.
					const uint32_t southPoleIndex = indexOffset + (azimuthSegments + 1) + azimuth;
					mb.addTriangle(southPoleIndex, rowOffset + azimuth, rowOffset + azimuth + 1);
				}
			}
		}
	}
}

Mesh* createSphere(const VertexDescription& vd, const Geometry::Sphere_f& sphere, uint32_t inclinationSegments, uint32_t azimuthSegments) {
  MeshBuilder mb(vd);
  addSphere(mb, sphere, inclinationSegments, azimuthSegments);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addDiscSector(MeshBuilder& mb, float radius, uint8_t numSegments, float angle) {
	if(numSegments < 1)
		return;
  uint32_t idx = mb.getNextIndex();
	mb.normal(Vec3(-1.0f, 0.0f, 0.0f));
	mb.position(Vec3(0,0,0));
	mb.addVertex();

	// Calculate vertices on the circle.
	const float step = Convert::degToRad(angle) / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment <= numSegments; ++segment) {
		const float segmentAngle = static_cast<float> (segment) * step;
		mb.position( Vec3(0, std::sin(segmentAngle), std::cos(segmentAngle)) * radius );
		mb.addVertex();
	}
	for (uint_fast8_t segment = 1; segment <= numSegments; ++segment)
		mb.addTriangle(idx,idx + segment,idx + segment+1);
}

Mesh* createDiscSector(const VertexDescription& vd, float radius, uint8_t numSegments, float angle) {
 MeshBuilder mb(vd);
 addDiscSector(mb, radius, numSegments, angle);
 return mb.buildMesh();
}

// ---------------------------------------------------------

void addRingSector(MeshBuilder& mb, float innerRadius, float outerRadius, uint8_t numSegments, float angle){
	if (numSegments < 1 || innerRadius >= outerRadius)
		return;  
  uint32_t idx = mb.getNextIndex();
  mb.normal(Vec3f{ -1.0f, 0.0f, 0.0f });

	// Calculate vertices on the circle.
	const float step = Convert::degToRad(angle) / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment <= numSegments; ++segment) {
		const float segmentAngle = static_cast<float> (segment) * step;
    Vec3 v(0, std::sin(segmentAngle), std::cos(segmentAngle));
    mb.position(v * innerRadius);
    mb.addVertex();
    mb.position(v * outerRadius);
    mb.addVertex();
	}

	for (uint_fast8_t segment = 0; segment < numSegments; ++segment) {
    mb.addTriangle(idx + segment * 2, idx + 1 + segment * 2, idx + 3 + segment * 2);
    mb.addTriangle(idx + segment * 2, idx + 3 + segment * 2, idx + 2 + segment * 2);
	}
}

Mesh* createRingSector(const VertexDescription& vd, float innerRadius, float outerRadius, uint8_t numSegments, float angle) {
  MeshBuilder mb(vd);
  addRingSector(mb, innerRadius, outerRadius, numSegments, angle);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addCone(MeshBuilder& mb, float radius, float height, uint8_t numSegments) {
	if (numSegments < 2)
		return;
  uint32_t idx = mb.getNextIndex();

	// First vertex is the apex.
	const Vec3f apex(height, 0.0f, 0.0f);
  mb.position(apex);
  mb.normal(Vec3{1,0,0});
  mb.addVertex();
  
	// Calculate vertices of the base.
	const float step = 6.28318530717958647692f / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment < numSegments; ++segment) {
		const float angle = static_cast<float> (segment) * step;
		const Vec3f pos(0.0f, radius * std::sin(angle), radius * std::cos(angle));
		const Vec3f tangent(0.0f, pos.getZ(), -pos.getY());
		const Vec3f lateral = apex - pos;
    
    mb.position(pos);
    mb.normal(lateral.cross(tangent).normalize());
    mb.addVertex();
	}

	for (uint_fast8_t segment = 1; segment < numSegments; ++segment) {
    mb.addTriangle(idx + segment, idx, idx + segment+1);
	}
	// Connect triangles to the vertex of the first triangle.
  mb.addTriangle(idx + numSegments, idx, idx + 1);
}

Mesh* createCone(const VertexDescription& vd, float radius, float height, uint8_t numSegments) {
  MeshBuilder mb(vd);
  addCone(mb, radius, height, numSegments);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addConicalFrustum(MeshBuilder& mb, float radiusBottom, float radiusTop, float height, uint8_t numSegments) {
	if (numSegments < 2)
		return;
  uint32_t idx = mb.getNextIndex();

	const float step = 6.28318530717958647692f / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment < numSegments; ++segment) {
		const float angle = static_cast<float> (segment) * step;
		const float sinAngle = std::sin(angle);
		const float cosAngle = std::cos(angle);

		const Vec3f posBottom(0.0f, radiusBottom * sinAngle, radiusBottom * cosAngle);
		const Vec3f posTop(height, radiusTop * sinAngle, radiusTop * cosAngle);
		const Vec3f tangent(0.0f, posBottom.getZ(), -posBottom.getY());
		const Vec3f lateral = posTop - posBottom;
    mb.normal(lateral.cross(tangent).normalize());    
		// Set vertex on the bottom circle.
    mb.position(posBottom);
    mb.addVertex();
		// Set vertex on the top circle.
    mb.position(posTop);
    mb.addVertex();
	}
  
	for (int_fast16_t segment = 0; segment < 2 * (numSegments - 1); segment += 2) {
    mb.addTriangle(idx + segment    , idx + segment + 1, idx + segment + 2);
    mb.addTriangle(idx + segment + 2, idx + segment + 1, idx + segment + 3);
	}
	// Connect last two triangles to the vertices of the first triangle.
  mb.addTriangle(idx + 2 * numSegments - 2, idx + 2 * numSegments - 1, idx);  
  mb.addTriangle(idx, idx + 2 * numSegments - 1, idx + 1);  
}

Mesh* createConicalFrustum(const VertexDescription& vd, float radiusBottom, float radiusTop, float height, uint8_t numSegments) {
  MeshBuilder mb(vd);
  addConicalFrustum(mb, radiusBottom, radiusTop, height, numSegments);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addArrow(MeshBuilder& mb, float radius, float length) {
  auto m = mb.getTransformation();
	Matrix4x4 transform(m);
  
  addConicalFrustum(mb, radius, radius, length - 0.01f - 0.29f, 16);
  
	transform.translate(length - 0.29f - 0.01f, 0.0f, 0.0f);
  mb.setTransformation(transform);
  addConicalFrustum(mb, radius, 2.0f * radius, 0.01f, 16);

	transform.translate(0.01f, 0.0f, 0.0f);
  mb.setTransformation(transform);
	addCone(mb, 2.0f * radius, 0.29f, 16);
  
  mb.setTransformation(m);
}

Mesh* createArrow(const VertexDescription& vd, float radius, float length) {
  MeshBuilder mb(vd);
  addArrow(mb, radius, length);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addRectangle(MeshBuilder& mb, const Rect_f& rect) {
  uint32_t idx = mb.getNextIndex();
	mb.normal(Vec3(0,0,1));
  mb.position(Vec3(0,0,0));

	mb.texCoord0( Vec2(0,0) );
  mb.position(rect.getCorner(rectCorner_t::xy));
	mb.addVertex();

	mb.texCoord0( Vec2(0,1) );
  mb.position(rect.getCorner(rectCorner_t::xY));
	mb.addVertex();

	mb.texCoord0( Vec2(1,1) );
  mb.position(rect.getCorner(rectCorner_t::XY));
	mb.addVertex();

	mb.texCoord0( Vec2(1,0) );
  mb.position(rect.getCorner(rectCorner_t::Xy));
	mb.addVertex();

	mb.addQuad(idx,idx+1,idx+2,idx+3);
}

Mesh* createRectangle(const VertexDescription& vd, const Geometry::Rect_f& rect) {
  MeshBuilder mb(vd);
  addRectangle(mb, rect);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addGrid(MeshBuilder& mb, float width, float height, uint32_t rows, uint32_t columns) {
	const float xScale=width / columns;
	const float yScale=height / rows;
  uint32_t idx = mb.getNextIndex();
  mb.normal(Vec3(0,1,0));

	for(uint32_t y=0; y<=rows; ++y) {
		for(uint32_t x=0; x<=columns; ++x) {
      Vec3 pos(xScale * x, 0, yScale * y);
			mb.position(pos);
      mb.texCoord0({pos.x()/width,1.0f-pos.z()/height});
			mb.addVertex();
			if(y > 0 && x > 0) {
				uint32_t idx_0 = idx + (y-1)*(columns+1) + (x-1);
				uint32_t idx_1 = idx + (y  )*(columns+1) + (x-1);
				uint32_t idx_2 = idx + (y  )*(columns+1) + (x  );
				uint32_t idx_3 = idx + (y-1)*(columns+1) + (x  );
				// add quad to the mesh
				mb.addQuad(idx_0,idx_1,idx_2,idx_3);
			}
		}
	}
}

Mesh* createGrid(const VertexDescription& vd, float width, float height, uint32_t rows, uint32_t columns) {
  MeshBuilder mb(vd);
  addGrid(mb, width, height, rows, columns);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addHexGrid(MeshBuilder& mb, float width, float height, uint32_t rows, uint32_t columns) {
	const float xScale=width / columns;
	const float yScale=height / rows;
  uint32_t idx = mb.getNextIndex();
  mb.normal(Vec3(0,1,0));

	for(uint32_t y=0; y<=rows; ++y) {
		for(uint32_t x=0; x<=columns; ++x) {
      Vec3 pos(x, 0, y);
      if(x%2==0 && y>0) pos += {0,0,-0.5};
      pos.setValue(pos.x() * xScale, pos.y(), pos.z() * yScale);      
			mb.position(pos);
      mb.texCoord0({pos.x()/width,1.0f-pos.z()/height});
			mb.addVertex();
			if(y > 0 && x > 0) {
				uint32_t idx_0 = idx + (y-1)*(columns+1) + (x-1);
				uint32_t idx_1 = idx + (y  )*(columns+1) + (x-1);
				uint32_t idx_2 = idx + (y  )*(columns+1) + (x  );
				uint32_t idx_3 = idx + (y-1)*(columns+1) + (x  );
        
        if(x%2==1)
			    mb.addQuad(idx_0,idx_1,idx_2,idx_3);
        else
			    mb.addQuad(idx_1,idx_2,idx_3,idx_0);
			}
		}
	}
  idx = mb.getNextIndex();
  for(uint32_t x=0; x<=columns; x+=2) {
    Vec3 pos(x, 0, rows);
    pos.setValue(pos.x() * xScale, pos.y(), pos.z() * yScale);
    mb.position(pos);
    mb.texCoord0({pos.x()/width,1.0f-pos.z()/height});
    mb.addVertex();
    uint32_t i = idx-columns-1+x;
    if(x<columns)
      mb.addTriangle(idx+(x>>1), i+1, i);
    if(x>0)
      mb.addTriangle(idx+(x>>1), i, i-1);
  }
}

Mesh* createHexGrid(const VertexDescription& vd, float width, float height, uint32_t rows, uint32_t columns) {
  MeshBuilder mb(vd);
  addHexGrid(mb, width, height, rows, columns);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addVoxelMesh(MeshBuilder& mb, const Util::PixelAccessor& colorAcc, uint32_t depth) {
	//Util::Reference<Util::PixelAccessor> colorAcc = Util::PixelAccessor::create(std::move(voxelBitmap));
	if( colorAcc.getPixelFormat().getComponentCount() < 4 ){
		WARN("createVoxelMesh: unsupported color texture format. Requires 4 components.");
		return;
	}
	if(colorAcc.getHeight()%depth != 0) {
		WARN("createVoxelMesh: Bitmap height is not divisible by depth.");
		return;
	}
	
	Vec3i res(colorAcc.getWidth(), colorAcc.getHeight()/depth, depth);
	
	const auto createQuad = [&](uint32_t x, uint32_t y, uint32_t z, uint8_t xMod, uint8_t yMod, uint8_t zMod, const Vec3& normal){
    uint32_t idx = mb.getNextIndex();
    mb.normal(normal);
    Vec3 pos(x,y,z);
    mb.position(Vec3{pos.x() + ((xMod&1)>0?1.0f:0.0f),pos.y() + ((yMod&1)>0?1.0f:0.0f),pos.z() + ((zMod&1)>0?1.0f:0.0f)}); mb.addVertex();
    mb.position(Vec3{pos.x() + ((xMod&2)>0?1.0f:0.0f),pos.y() + ((yMod&2)>0?1.0f:0.0f),pos.z() + ((zMod&2)>0?1.0f:0.0f)}); mb.addVertex();
    mb.position(Vec3{pos.x() + ((xMod&4)>0?1.0f:0.0f),pos.y() + ((yMod&4)>0?1.0f:0.0f),pos.z() + ((zMod&4)>0?1.0f:0.0f)}); mb.addVertex();
    mb.position(Vec3{pos.x() + ((xMod&8)>0?1.0f:0.0f),pos.y() + ((yMod&8)>0?1.0f:0.0f),pos.z() + ((zMod&8)>0?1.0f:0.0f)}); mb.addVertex();
    mb.addQuad(idx,idx+1,idx+2,idx+3);
	};
	
  for(uint32_t z=0; z<res.z(); ++z) {    
    for(uint32_t y=0; y<res.y(); ++y) {      
      for(uint32_t x=0; x<res.x(); ++x) {
        auto color = colorAcc.readColor4f(x, y + z*res.y()); 
        if(color.a() > 0) {
          mb.color(color);
          if(x==0 || colorAcc.readColor4f(x-1, y + z*res.y()).a() < 0.1) 
            createQuad(x, y, z, 0, 4|8, 2|4, {-1,0,0});
          if(x==res.x()-1 || colorAcc.readColor4f(x+1, y + z*res.y()).a() < 0.1) 
            createQuad(x, y, z, 1|2|4|8, 2|4, 4|8, {1,0,0});
          if(y==0 || colorAcc.readColor4f(x, y-1 + z*res.y()).a() < 0.1) 
            createQuad(x, y, z, 2|4, 0, 4|8, {0,-1,0});
          if(y==res.y()-1 || colorAcc.readColor4f(x, y+1 + z*res.y()).a() < 0.1) 
            createQuad(x, y, z, 4|8, 1|2|4|8, 2|4, {0,1,0});
          if(z==0 || colorAcc.readColor4f(x, y + (z-1)*res.y()).a() < 0.1) 
            createQuad(x, y, z, 4|8, 2|4, 0, {0,0,-1});
          if(z==res.z()-1 || colorAcc.readColor4f(x, y + (z+1)*res.y()).a() < 0.1) 
            createQuad(x, y, z, 2|4, 4|8, 1|2|4|8, {0,0,1});
        }
      }
    }
  }
}

Mesh* createVoxelMesh(const VertexDescription& vd, const Util::PixelAccessor& colorAcc, uint32_t depth) {
  MeshBuilder mb(vd);
  addVoxelMesh(mb, colorAcc, depth);
  return mb.buildMesh();
}

// ---------------------------------------------------------

void addTorus(MeshBuilder& mb, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments) {  
  uint32_t idx = mb.getNextIndex();
	innerRadius = std::max(0.0f, innerRadius);
	majorSegments = std::max(3U, majorSegments);
	minorSegments = std::max(3U, minorSegments);
	if(innerRadius > outerRadius) {
		WARN("addTorus: innerRadius is greater than outerRadius.");
		return;
	}
	float minorRadius = (outerRadius - innerRadius) * 0.5;
	float majorRadius = innerRadius + minorRadius;
	for(uint32_t major=0; major<majorSegments; ++major) {
		float u = major * 2.0 * M_PI / majorSegments;
		Vec3 center(std::cos(u) * majorRadius, 0, std::sin(u) * majorRadius);
		for(uint32_t minor=0; minor<minorSegments; ++minor) {
			float v = minor * 2.0 * M_PI / minorSegments;
			Vec3 n = (center.getNormalized() * std::cos(v) + Vec3(0, std::sin(v), 0)).getNormalized();
			Vec3 p = center + n * minorRadius;
			mb.position(p);
			mb.normal(n);
			mb.texCoord0(Vec2(1.0f - static_cast<float>(major)/majorSegments, static_cast<float>(minor)/minorSegments));
			mb.addVertex();
			mb.addQuad(
				idx +  major*minorSegments 										+  minor,
		 	  idx +  major*minorSegments 										+ (minor+1)%minorSegments,
				idx + ((major+1)%majorSegments)*minorSegments + (minor+1)%minorSegments,
				idx + ((major+1)%majorSegments)*minorSegments +  minor
			);
		}
	}
}

Mesh* createTorus(const VertexDescription& vd, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments) {
  MeshBuilder mb(vd);
  addTorus(mb, innerRadius, outerRadius, majorSegments, minorSegments);
  return mb.buildMesh();
}

// ---------------------------------------------------------

//! Adds a mesh from bitmap to the given meshBuilder. \see createMeshFromBitmaps(...)
void addMeshFromBitmaps(MeshBuilder& mb, Util::Reference<Util::PixelAccessor> depth, Util::Reference<Util::PixelAccessor> color, Util::Reference<Util::PixelAccessor> normals) {
  const uint32_t width = depth->getWidth();
	const uint32_t height = depth->getHeight();

	if( depth->getPixelFormat()!=Util::PixelFormat::MONO_FLOAT ){
		WARN("createMeshFromBitmaps: unsupported depth texture format");
		return;
	}
	if(color.isNotNull() && (color->getPixelFormat() != Util::PixelFormat::RGBA && color->getPixelFormat() != Util::PixelFormat::RGB)) {
		WARN("createMeshFromBitmaps: unsupported color texture format");
		return;
	}
	const float xScale=2.0 / width;
	const float yScale=2.0 / height;
	const float cut=1;

	for(uint32_t y=0; y<height; ++y){
		for(uint32_t x=0; x<width; ++x){
			Vec3 pos(xScale * x - 1.0f, yScale * y - 1.0f, 2.0f * depth->readSingleValueFloat(x,y) - 1.0f);
			mb.position( pos );
      if(color.isNotNull())
			   mb.color(color->readColor4ub(x, y));
			if(normals.isNotNull()){
				Util::Color4f tmp = normals->readColor4f(x,y);
				Vec3 normal=Vec3(tmp.getR()-0.5f,tmp.getG()-0.5f,tmp.getB()-0.5f);
				if(!normal.isZero())
					normal.normalize();
				mb.normal(normal);
			}

			uint32_t index=mb.addVertex();
			// add triangles
			if(x>0 && y>0){
				const float z_1_1 = depth->readSingleValueFloat(x - 1, y - 1);
				const float z_1_0 = depth->readSingleValueFloat(x - 1, y);
				const float z_0_1 = depth->readSingleValueFloat(x, y - 1);
				const float z_0_0 = depth->readSingleValueFloat(x, y);

				if( std::abs( z_0_0 - z_1_1 ) > std::abs( z_1_0 - z_0_1 ) ){
					/*
					_1_1	_0_1
					  o---o  o
					  |  /  /|
					  | /  / |
					  |/  /  |
					  o  o---o
					_1_0	_0_0
					*/
					if( z_1_1<cut && z_1_0<cut && z_0_1<cut  ){
						mb.addIndex( index-width-1);
						mb.addIndex( index-width);
						mb.addIndex( index-1);
					}
//
					if( z_0_1<cut && z_1_0<cut && z_0_0<cut  ){
						mb.addIndex( index-width);
						mb.addIndex( index);
						mb.addIndex( index-1);
					}
				}else {
					/*
					_1_1	_0_1
					  o  o---o
					  |\  \  |
					  | \  \ |
					  |  \  \|
					  o---o  o
					_1_0	_0_0
					*/
					if( z_1_1<cut && z_1_0<cut && z_0_0<cut ){
						mb.addIndex( index-width-1);
						mb.addIndex( index);
						mb.addIndex( index-1);
					}

					if( z_1_1<cut && z_0_1<cut && z_0_0<cut ){
						mb.addIndex( index);
						mb.addIndex( index-width-1);
						mb.addIndex( index-width);
					}
				}
			}
		}
	}
}

Mesh* createMeshFromBitmaps(const VertexDescription& vd, Util::Reference<Util::PixelAccessor> depth, Util::Reference<Util::PixelAccessor> color, Util::Reference<Util::PixelAccessor> normals) {
  MeshBuilder mb(vd);
  addMeshFromBitmaps(mb, depth, color, normals);
  return mb.buildMesh();
}

// ---------------------------------------------------------
  
} /* MeshUtils */
} /* Rendering */