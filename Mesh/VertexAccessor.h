/*
	This file is part of the Rendering library.
	Copyright (C) 2018-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_MESH_VERTEX_ACCESSOR_H_
#define RENDERING_MESH_VERTEX_ACCESSOR_H_

#include "MeshVertexData.h"
#include "VertexDescription.h"
#include "VertexAttributeIds.h"

#include <Util/Resources/ResourceAccessor.h>
#include <Util/StringIdentifier.h>
#include <Util/ReferenceCounter.h>
#include <Util/Graphics/Color.h>

#include <Geometry/Vec4.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec2.h>

#include <unordered_map>
#include <memory>
#include <vector>

namespace Rendering {
class Mesh;

/** @addtogroup mesh
 * @{
 * @defgroup mesh_accessor Mesh Accessors
 * Mesh accessors should be used when modifying the vertex data of an existing mesh.
 * @}
 */

/**
 * Utility class to access all vertex attributes of a mesh.
 * Directly maps the vertex data of a mesh in GPU memory if it uploaded.
 *
 * \note Do not upload or render the mesh while the Accessor is active.
 * \see VertexAttributeAccessor
 * @ingroup mesh_accessor
 */
class VertexAccessor : public Util::ResourceAccessor {
private:
	MeshVertexData& vData;
	RENDERINGAPI explicit VertexAccessor(MeshVertexData& _vData, uint8_t* ptr);
public:	
	RENDERINGAPI virtual ~VertexAccessor();
	
	RENDERINGAPI static Util::Reference<VertexAccessor> create(MeshVertexData& _vData);
	RENDERINGAPI static Util::Reference<VertexAccessor> create(Mesh* mesh);
		
	Geometry::Vec3 getPosition(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::POSITION) const {
		Geometry::Vec3 v;
		readValues(index, name, reinterpret_cast<float*>(&v), 3);
		return v;
	}
	Geometry::Vec3 getPosition(uint32_t index, uint16_t location) const {
		Geometry::Vec3 v;
		readValues(index, location, reinterpret_cast<float*>(&v), 3);
		return v;
	}
	void setPosition(uint32_t index, const Geometry::Vec3& p, Util::StringIdentifier name=VertexAttributeIds::POSITION) {
		writeValues(index, name, p.getVec(), 3);
	}
	void setPosition(uint32_t index, const Geometry::Vec3& p, uint16_t location) {
		writeValues(index, location, p.getVec(), 3);
	}
	
	Geometry::Vec3 getNormal(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::NORMAL) const {
		return getPosition(index, name);
	}
	Geometry::Vec3 getNormal(uint32_t index, uint16_t location) const {
		return getPosition(index, location);
	}
	void setNormal(uint32_t index, const Geometry::Vec3& n, Util::StringIdentifier name=VertexAttributeIds::NORMAL) {
		setPosition(index, n, name);
	}
	void setNormal(uint32_t index, const Geometry::Vec3& n, uint16_t location) {
		setPosition(index, n, location);
	}
		
	Util::Color4f getColor4f(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::COLOR) const {
		const auto data = readValues<float>(index, name, 4);
		return Util::Color4f(data);
	}
	Util::Color4f getColor4ub(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::COLOR) const {
		return Util::Color4f(getColor4f(index, name));
	}
	Util::Color4f getColor4f(uint32_t index, uint16_t location) const {
		const auto data = readValues<float>(index, location, 4);
		return Util::Color4f(data);
	}
	Util::Color4ub getColor4ub(uint32_t index, uint16_t location) const {
		return Util::Color4ub(getColor4f(index, location));
	}
	void setColor(uint32_t index, const Util::Color4f& c, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
		writeValues(index, name, c.data(), 4);
	}
	void setColor(uint32_t index, const Util::Color4ub& c, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
		setColor(index, Util::Color4f(c), name);
	}
	void setColor(uint32_t index, const Util::Color4f& c, uint16_t location) {
		writeValues(index, location, c.data(), 4);
	}
	void setColor(uint32_t index, const Util::Color4ub& c, uint16_t location) {
		setColor(index, Util::Color4f(c), location);
	}
	
	Geometry::Vec2 getTexCoord(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::TEXCOORD0) const {
		Geometry::Vec2 v;
		readValues(index, name, reinterpret_cast<float*>(&v), 2);
		return v;
	}
	Geometry::Vec2 getTexCoord(uint32_t index, uint16_t location) const {
		Geometry::Vec2 v;
		readValues(index, location, reinterpret_cast<float*>(&v), 2);
		return v;
	}
	void setTexCoord(uint32_t index, const Geometry::Vec2& p, Util::StringIdentifier name=VertexAttributeIds::TEXCOORD0) {
		writeValues(index, name, p.getVec(), 2);
	}
	void setTexCoord(uint32_t index, const Geometry::Vec2& p, uint16_t location) {
		writeValues(index, location, p.getVec(), 2);
	}  
	
	Geometry::Vec4 getVec4(uint32_t index, Util::StringIdentifier name) const {
		Geometry::Vec4 v;
		readValues(index, name, reinterpret_cast<float*>(&v), 4);
		return v;
	}
	Geometry::Vec4 getVec4(uint32_t index, uint16_t location) const {
		Geometry::Vec4 v;
		readValues(index, location, reinterpret_cast<float*>(&v), 4);
		return v;
	}
	void setVec4(uint32_t index, const Geometry::Vec4& p, Util::StringIdentifier name) {
		writeValues(index, name, p.getVec(), 4);
	}
	void setVec4(uint32_t index, const Geometry::Vec4& p, uint16_t location) {
		writeValues(index, location, p.getVec(), 4);
	}
};
	
} /* Rendering */

#endif /* end of include guard: RENDERING_MESH_VERTEX_ACCESSOR_H_ */
