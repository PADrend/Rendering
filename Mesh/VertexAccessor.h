/*
 	This file is part of the Rendering library.
 	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
 	
 	This library is subject to the terms of the Mozilla Public License, v. 2.0.
 	You should have received a copy of the MPL along with this library; see the
 	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_MESH_VERTEX_ACCESSOR_H_
#define RENDERING_MESH_VERTEX_ACCESSOR_H_

#include "MeshVertexData.h"
#include "VertexDescription.h"
#include "VertexAttributeIds.h"
#include "VertexAttributeAccessors.h"

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
  
/**
 * Utility class to access all vertex attributes of a mesh.
 * Lazily creates vertex attribute accessors as needed.
 *
 * \note Due to the lazy initialization, using a VertexAttributeAccessor for a single attribute directly might be faster
 * \see VertexAttributeAccessor
 * @ingroup mesh
 */
class VertexAccessor : public Util::ReferenceCounter<VertexAccessor> {
private:
  MeshVertexData& vData;
  mutable std::unordered_map<Util::StringIdentifier, std::unique_ptr<VertexAttributeAccessor>> accessors;
  
  template<typename Accessor>
  Accessor* getAccessor(Util::StringIdentifier name) const;
public:
  VertexAccessor(MeshVertexData& _vData) : 
    ReferenceCounter_t(), vData(_vData) { }
  virtual ~VertexAccessor() = default;
  
  const std::vector<float> getFloats(uint32_t index, Util::StringIdentifier name) const;
	void setFloats(uint32_t index, const float* values, uint32_t count, Util::StringIdentifier name);
	inline void setFloats(uint32_t index, const std::vector<float>& values, Util::StringIdentifier name) {
    setFloats(index, values.data(), values.size(), name);
  }
  inline float getFloat(uint32_t index, Util::StringIdentifier name) const {
    return getFloats(index, name).front();
  }
	inline void setFloat(uint32_t index, float value, Util::StringIdentifier name) {
    setFloats(index, &value, 1, name);
  }
    
  const std::vector<uint32_t> getUInts(uint32_t index, Util::StringIdentifier name) const;
	void setUInts(uint32_t index, const uint32_t* values, uint32_t count, Util::StringIdentifier name);
	inline void setUInts(uint32_t index, const std::vector<uint32_t>& values, Util::StringIdentifier name) {
    setUInts(index, values.data(), values.size(), name);
  }
  inline uint32_t getUInt(uint32_t index, Util::StringIdentifier name) const {
    return getUInts(index, name).front();
  }
	inline void setUInt(uint32_t index, uint32_t value, Util::StringIdentifier name) {
    setUInts(index, &value, 1, name);
  }
  
  inline Geometry::Vec3 getPosition(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::POSITION) const {
    const auto data = getFloats(index, name);
    return Geometry::Vec3(data.data());
  }
  inline void setPosition(uint32_t index, const Geometry::Vec3& p, Util::StringIdentifier name=VertexAttributeIds::POSITION) {
    setFloats(index, reinterpret_cast<const float*>(&p), 3, name);
  }
  
  inline Geometry::Vec3 getNormal(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::NORMAL) const {
    return getPosition(index, name);
  }
  inline void setNormal(uint32_t index, const Geometry::Vec3& n, Util::StringIdentifier name=VertexAttributeIds::NORMAL) {
    setPosition(index, n, name);
  }
    
  inline Util::Color4f getColor4f(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::COLOR) const {
    const auto data = getFloats(index, name);
    return Util::Color4f(data);
  }
  inline Util::Color4f getColor4ub(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::COLOR) const {
    return Util::Color4f(getColor4f(index, name));
  }
  inline void setColor(uint32_t index, const Util::Color4f& c, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
    setFloats(index, c.data(), 4, name);
  }
  inline void setColor(uint32_t index, const Util::Color4ub& c, Util::StringIdentifier name=VertexAttributeIds::COLOR) {
    setColor(index, Util::Color4f(c), name);
  }
  
  inline Geometry::Vec2 getTexCoord(uint32_t index, Util::StringIdentifier name=VertexAttributeIds::TEXCOORD0) const {
    const auto data = getFloats(index, name);
    return Geometry::Vec2(data.data());
  }
  inline void setTexCoord(uint32_t index, const Geometry::Vec2& p, Util::StringIdentifier name=VertexAttributeIds::POSITION) {
    setFloats(index, reinterpret_cast<const float*>(&p), 2, name);
  }  
  
  inline Geometry::Vec4 getVec4(uint32_t index, Util::StringIdentifier name) const {
    const auto data = getFloats(index, name);
    return Geometry::Vec4(data.data());
  }
  inline void setVec4(uint32_t index, const Geometry::Vec4& p, Util::StringIdentifier name) {
    setFloats(index, reinterpret_cast<const float*>(&p), 4, name);
  }
};
  
} /* Rendering */

#endif /* end of include guard: RENDERING_MESH_VERTEX_ACCESSOR_H_ */
