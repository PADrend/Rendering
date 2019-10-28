/*
	This file is part of the Rendering library.
	Copyright (C) 2018-2019 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DIRECT_VERTEX_ACCESSOR_H_
#define RENDERING_DIRECT_VERTEX_ACCESSOR_H_

#include "../Mesh/MeshVertexData.h"
#include "../Mesh/VertexDescription.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexAttributeAccessors.h"

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
namespace MeshUtils {
	
/**
 * Directly maps the vertex data of a mesh in GPU memory.
 * \note Do not upload or render the mesh while the Accessor is active.
 * \see VertexAttributeAccessor
 */
class DirectVertexAccessor : public Util::ResourceAccessor {
private:
	MeshVertexData& vData;
	DirectVertexAccessor(MeshVertexData& _vData, uint8_t* ptr);
public:	
	virtual ~DirectVertexAccessor();
	
	static Util::Reference<DirectVertexAccessor> create(MeshVertexData& _vData);
	static Util::Reference<DirectVertexAccessor> create(Mesh* mesh);
		
	inline Geometry::Vec3 getPosition(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::POSITION) {
		Geometry::Vec3 v;
		readValues(index, name, reinterpret_cast<float*>(&v), 3);
		return v;
	}  
	inline void setPosition(uint32_t index, const Geometry::Vec3& p, Util::StringIdentifier name=VertexAttributeIds::POSITION) {
		writeValues(index, name, p.getVec(), 3);
	}
	
	inline Geometry::Vec3 getNormal(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::NORMAL) {
		return getPosition(index, name);
	}
	inline void setNormal(uint32_t index, const Geometry::Vec3& n, Util::StringIdentifier name=VertexAttributeIds::NORMAL) {
		setPosition(index, n, name);
	}
		
	inline Util::Color4f getColor4f(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
		const auto data = readValues<float>(index, name, 4);
		return Util::Color4f(data);
	}
	inline Util::Color4f getColor4ub(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
		return Util::Color4f(getColor4f(index, name));
	}
	inline void setColor(uint32_t index, const Util::Color4f& c, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
		writeValues(index, name, c.data(), 4);
	}
	inline void setColor(uint32_t index, const Util::Color4ub& c, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
		setColor(index, Util::Color4f(c), name);
	}
	
	inline Geometry::Vec2 getTexCoord(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::TEXCOORD0) {
		Geometry::Vec2 v;
		readValues(index, name, reinterpret_cast<float*>(&v), 2);
		return v;
	}
	inline void setTexCoord(uint32_t index, const Geometry::Vec2& p, Util::StringIdentifier name=VertexAttributeIds::POSITION) {
		writeValues(index, name, p.getVec(), 2);
	}  
	
	inline Geometry::Vec4 getVec4(uint32_t index, Util::StringIdentifier name) {
		Geometry::Vec4 v;
		readValues(index, name, reinterpret_cast<float*>(&v), 4);
		return v;
	}
	inline void setVec4(uint32_t index, const Geometry::Vec4& p, Util::StringIdentifier name) {
		writeValues(index, name, p.getVec(), 4);
	}
};
	
} /* MeshUtils */
} /* Rendering */

#endif /* end of include guard: RENDERING_MESH_VERTEX_ACCESSOR_H_ */
