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
		static Mesh * createBox(const VertexDescription & vd, const Geometry::Box & box) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::addBox(...)
		static void addBox(MeshBuilder & mb, const Geometry::Box & box) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createDome(...)
		static Mesh * createDome(const double radius = 100.0, const int horiRes = 40, const int vertRes = 40, const double halfSphereFraction = 1.0, const double imagePercentage = 1.0) __attribute__((deprecated));			 
	 	//! Deprecated \see MeshUtils::createSphere(...)
		static Mesh * createSphere(const VertexDescription & vd, uint32_t inclinationSegments, uint32_t azimuthSegments) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::addSphere(...)
		static void addSphere(MeshBuilder & mb, const Geometry::Sphere_f & sphere, uint32_t inclinationSegments, uint32_t azimuthSegments) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createDiscSector(...)
		static Mesh * createDiscSector(float radius, uint8_t numSegments, float angle = 360.0f) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createRingSector(...)
		static Mesh * createRingSector(float innerRadius, float outerRadius, uint8_t numSegments, float angle = 360.0f) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createCone(...)
		static Mesh * createCone(float radius, float height, uint8_t numSegments) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createConicalFrustum(...)
		static Mesh * createConicalFrustum(float radiusBottom, float radiusTop, float height, uint8_t numSegments) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createArrow(...)
		static Mesh * createArrow(float radius, float length) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createRectangle(...)
		static Mesh * createRectangle(const VertexDescription & vd,float width, float height) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createMeshFromBitmaps(...)
		static Mesh * createMeshFromBitmaps(const VertexDescription& vd, Util::Reference<Util::Bitmap> depth, Util::Reference<Util::Bitmap> color = nullptr, Util::Reference<Util::Bitmap> normals = nullptr ) __attribute__((deprecated));								
		//! Deprecated \see MeshUtils::createHexGrid(...)
		static Mesh * createHexGrid(const VertexDescription & vd, float width, float height, uint32_t rows, uint32_t columns) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createVoxelMesh(...)
		static Mesh * createVoxelMesh(const VertexDescription & vd, const Util::PixelAccessor& colorAcc, uint32_t depth) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::createTorus(...)
		static Mesh * createTorus(const VertexDescription & vd, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments) __attribute__((deprecated));		
		//! Deprecated \see MeshUtils::addTorus(...)
		static void addTorus(MeshBuilder & mb, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments) __attribute__((deprecated));

public:
	MeshBuilder();
	explicit MeshBuilder(VertexDescription description);
	~MeshBuilder();
	
	/*! true if no no vertices were added so far.	*/
	bool isEmpty() const { return vSize == 0; }

	//! Build a new mesh using the internal vertex and index buffer.
	Mesh * buildMesh();

	/*! Sets the current vertex data for the following vertices (like a state in OpenGL). 
		If a tranformation is set, the position and normal are transformed accordingly before being set. */
	void position(const Geometry::Vec2 & v, const Util::StringIdentifier& attr=VertexAttributeIds::POSITION);
	void position(const Geometry::Vec3f & v, const Util::StringIdentifier& attr=VertexAttributeIds::POSITION);
	void position(const Geometry::Vec4 & v, const Util::StringIdentifier& attr=VertexAttributeIds::POSITION);
	void normal(const Geometry::Vec3f & n, const Util::StringIdentifier& attr=VertexAttributeIds::NORMAL);
	void normal(const Geometry::Vec3b & n, const Util::StringIdentifier& attr=VertexAttributeIds::NORMAL);
	void normal(const Geometry::Vec4 & n, const Util::StringIdentifier& attr=VertexAttributeIds::NORMAL);
	void color(const Util::Color4f & c, const Util::StringIdentifier& attr=VertexAttributeIds::COLOR);
	void color(const Util::Color4ub & c, const Util::StringIdentifier& attr=VertexAttributeIds::COLOR);
	void texCoord0(const Geometry::Vec2 & uv, const Util::StringIdentifier& attr=VertexAttributeIds::TEXCOORD0);
	void values(const std::vector<float> & v, const Util::StringIdentifier& attr);
	void values(const std::vector<uint32_t> & v, const Util::StringIdentifier& attr);
	void value(float v, const Util::StringIdentifier& attr);
	void value(uint32_t v, const Util::StringIdentifier& attr);

	/*! Add a vertex with the current data (set by position(...),normal(...) etc.).
		The index of the new vertex is returned.*/
	uint32_t addVertex();

	/*! Add a vertex to the internal buffer. The index of the new vertex is returned.  deprecated!!!!!	*/
	uint32_t addVertex(const Geometry::Vec3& pos, const Geometry::Vec3& n,
						float r, float g, float b, float a,
						float u, float v) __attribute__((deprecated));

	/*! Add a index to the interal buffer	*/
	void addIndex(uint32_t idx);

	/*! Adds a quad to the internal buffer, clockwise.	*/
	void addQuad(uint32_t idx0, uint32_t idx1, uint32_t idx2, uint32_t idx3);

	/*! Adds a three indices	*/
	void addTriangle(uint32_t idx0, uint32_t idx1, uint32_t idx2);

	/*! Get current vertex count which is the index of next vertex added. */
	uint32_t getNextIndex() const { return vSize; }
	
	//! Add entire mesh to meshBuilder
	void addMesh(Mesh* mesh);
	
	//! Get the current transformation.
	Geometry::Matrix4x4 getTransformation() const;

	//! The transformation is applied to following 'position' and 'normal' calls.
	void setTransformation(const Geometry::Matrix4x4 & m);
	void setTransformation(const Geometry::SRT & s);
	
	//! Multiply on current transform.
	void transform(const Geometry::Matrix4x4 & m);
	
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
