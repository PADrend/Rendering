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
#ifndef MESHBUILDER_H_
#define MESHBUILDER_H_

#include "../Mesh/Mesh.h"
#include "../Mesh/MeshVertexData.h"
#include "../Mesh/MeshIndexData.h"
#include "../Mesh/VertexDescription.h"
#include "../Mesh/VertexAttributeIds.h"
#include <Util/ReferenceCounter.h>
#include <Util/StringUtils.h>

#include <cstdint>
#include <memory>

/** @addtogroup mesh
 * @{
 * @defgroup mesh_builder Mesh Builder
 * Mesh builders can be used to create meshes.
 * @}
 */
 
namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4;
template<typename T_> class _SRT;
typedef _SRT<float> SRT;
template<typename _T> class _Vec2;
typedef _Vec2<float> Vec2;
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3;
typedef _Vec3<float> Vec3f;
typedef _Vec3<char> Vec3b;
template<typename _T> class _Vec4;
typedef _Vec4<float> Vec4;
template<typename T_>
class _Sphere;
typedef _Sphere<float> Sphere_f;
}

namespace Util {
class Color4f;
class Color4ub;
class Bitmap;
class PixelAccessor;
}

namespace Rendering {
class Mesh;
class VertexAccessor;

namespace MeshUtils {

/** Utility class that allows building meshes
* @ingroup mesh_builder
*/
class MeshBuilder : public Util::ReferenceCounter<MeshBuilder> {
	public:
		//! Deprecated \see MeshUtils::createBox(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createBox(const VertexDescription & vd, const Geometry::Box & box);		
		//! Deprecated \see MeshUtils::addBox(...)
		RENDERINGAPI RENDERING_DEPRECATED static void addBox(MeshBuilder & mb, const Geometry::Box & box);		
		//! Deprecated \see MeshUtils::createDome(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createDome(const double radius = 100.0, const int horiRes = 40, const int vertRes = 40, const double halfSphereFraction = 1.0, const double imagePercentage = 1.0);			 
	 	//! Deprecated \see MeshUtils::createSphere(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createSphere(const VertexDescription & vd, uint32_t inclinationSegments, uint32_t azimuthSegments);		
		//! Deprecated \see MeshUtils::addSphere(...)
		RENDERINGAPI RENDERING_DEPRECATED static void addSphere(MeshBuilder & mb, const Geometry::Sphere_f & sphere, uint32_t inclinationSegments, uint32_t azimuthSegments);		
		//! Deprecated \see MeshUtils::createDiscSector(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createDiscSector(float radius, uint8_t numSegments, float angle = 360.0f);		
		//! Deprecated \see MeshUtils::createRingSector(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createRingSector(float innerRadius, float outerRadius, uint8_t numSegments, float angle = 360.0f);		
		//! Deprecated \see MeshUtils::createCone(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createCone(float radius, float height, uint8_t numSegments);		
		//! Deprecated \see MeshUtils::createConicalFrustum(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createConicalFrustum(float radiusBottom, float radiusTop, float height, uint8_t numSegments);		
		//! Deprecated \see MeshUtils::createArrow(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createArrow(float radius, float length);		
		//! Deprecated \see MeshUtils::createRectangle(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createRectangle(const VertexDescription & vd,float width, float height);		
		//! Deprecated \see MeshUtils::createMeshFromBitmaps(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createMeshFromBitmaps(const VertexDescription& vd, Util::Reference<Util::Bitmap> depth, Util::Reference<Util::Bitmap> color = nullptr, Util::Reference<Util::Bitmap> normals = nullptr );								
		//! Deprecated \see MeshUtils::createHexGrid(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createHexGrid(const VertexDescription & vd, float width, float height, uint32_t rows, uint32_t columns);		
		//! Deprecated \see MeshUtils::createVoxelMesh(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createVoxelMesh(const VertexDescription & vd, const Util::PixelAccessor& colorAcc, uint32_t depth);		
		//! Deprecated \see MeshUtils::createTorus(...)
		RENDERINGAPI RENDERING_DEPRECATED static Mesh * createTorus(const VertexDescription & vd, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments);		
		//! Deprecated \see MeshUtils::addTorus(...)
		RENDERINGAPI RENDERING_DEPRECATED static void addTorus(MeshBuilder & mb, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments);

public:
	RENDERINGAPI MeshBuilder();
	RENDERINGAPI explicit MeshBuilder(VertexDescription description);
	RENDERINGAPI ~MeshBuilder();
	
	/*! true if no no vertices were added so far.	*/
	bool isEmpty() const { return vSize == 0; }

	//! Build a new mesh using the internal vertex and index buffer.
	RENDERINGAPI Mesh * buildMesh();

	/*! Sets the current vertex data for the following vertices (like a state in OpenGL). 
		If a tranformation is set, the position and normal are transformed accordingly before being set. */
	RENDERINGAPI void position(const Geometry::Vec2 & v, const Util::StringIdentifier& attr=VertexAttributeIds::POSITION);
	RENDERINGAPI void position(const Geometry::Vec3f & v, const Util::StringIdentifier& attr=VertexAttributeIds::POSITION);
	RENDERINGAPI void position(const Geometry::Vec4 & v, const Util::StringIdentifier& attr=VertexAttributeIds::POSITION);
	RENDERINGAPI void normal(const Geometry::Vec3f & n, const Util::StringIdentifier& attr=VertexAttributeIds::NORMAL);
	RENDERINGAPI void normal(const Geometry::Vec3b & n, const Util::StringIdentifier& attr=VertexAttributeIds::NORMAL);
	RENDERINGAPI void color(const Util::Color4f & c, const Util::StringIdentifier& attr=VertexAttributeIds::COLOR);
	RENDERINGAPI void color(const Util::Color4ub & c, const Util::StringIdentifier& attr=VertexAttributeIds::COLOR);
	RENDERINGAPI void texCoord0(const Geometry::Vec2 & uv, const Util::StringIdentifier& attr=VertexAttributeIds::TEXCOORD0);
	RENDERINGAPI void values(const std::vector<float> & v, const Util::StringIdentifier& attr);
	RENDERINGAPI void values(const std::vector<uint32_t> & v, const Util::StringIdentifier& attr);
	RENDERINGAPI void value(float v, const Util::StringIdentifier& attr);
	RENDERINGAPI void value(uint32_t v, const Util::StringIdentifier& attr);

	/*! Add a vertex with the current data (set by position(...),normal(...) etc.).
		The index of the new vertex is returned.*/
	RENDERINGAPI uint32_t addVertex();

	/*! Add a vertex to the internal buffer. The index of the new vertex is returned.  deprecated!!!!!	*/
	RENDERINGAPI RENDERING_DEPRECATED uint32_t addVertex(const Geometry::Vec3& pos, const Geometry::Vec3& n,
						float r, float g, float b, float a,
						float u, float v);

	/*! Add a index to the interal buffer	*/
	RENDERINGAPI void addIndex(uint32_t idx);

	/*! Adds a quad to the internal buffer, clockwise.	*/
	RENDERINGAPI void addQuad(uint32_t idx0, uint32_t idx1, uint32_t idx2, uint32_t idx3);

	/*! Adds a three indices	*/
	RENDERINGAPI void addTriangle(uint32_t idx0, uint32_t idx1, uint32_t idx2);

	/*! Get current vertex count which is the index of next vertex added. */
	uint32_t getNextIndex() const { return vSize; }
	
	//! Add entire mesh to meshBuilder
	RENDERINGAPI void addMesh(Mesh* mesh);
	
	//! Get the current transformation.
	RENDERINGAPI Geometry::Matrix4x4 getTransformation() const;

	//! The transformation is applied to following 'position' and 'normal' calls.
	RENDERINGAPI void setTransformation(const Geometry::Matrix4x4 & m);
	RENDERINGAPI void setTransformation(const Geometry::SRT & s);
	
	//! Multiply on current transform.
	RENDERINGAPI void transform(const Geometry::Matrix4x4 & m);
	
private:
	VertexDescription description;
	uint32_t vSize=0;
	uint32_t iSize=0;
	MeshVertexData vData; //!< vertex buffer
	MeshIndexData iData; //!< index buffer

	MeshVertexData currentVertex;
	Util::Reference<VertexAccessor> acc;
	std::unique_ptr<Geometry::Matrix4x4> transMat;
};

}
}

#endif /* MESHBUILDER_H_ */
