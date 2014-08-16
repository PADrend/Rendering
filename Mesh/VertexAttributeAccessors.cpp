/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VertexAttributeAccessors.h"
#include "../GLHeader.h"
#include <Geometry/Convert.h>
#include <sstream>
#include <exception>

namespace Rendering {

//! (internal)
void VertexAttributeAccessor::throwRangeError(uint32_t index)const {

	std::ostringstream s;
	s << "Trying to access vertex " << index << " of overall " << vData.getVertexCount() << " vertices.";
	throw std::range_error(s.str());
}

// -----------

static const std::string noAttrErrorMsg("No attribute named '");
static const std::string unimplementedFormatMsg("Attribute format not implemented for attribute '");

//! (helper)
static const VertexAttribute & assertAttribute(MeshVertexData & _vData, const Util::StringIdentifier name) {
	const VertexAttribute & attr = _vData.getVertexDescription().getAttribute(name);
	if(attr.empty())
		throw std::invalid_argument(noAttrErrorMsg + name.toString() + '\'');
	return attr;
}

// ---------------------------------
// Color

/*! ColorAttributeAccessor3f ---|> ColorAttributeAccessor	*/
class ColorAttributeAccessor3f : public ColorAttributeAccessor {
	public:
		ColorAttributeAccessor3f(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			ColorAttributeAccessor(_vData, _attribute) {}
		virtual ~ColorAttributeAccessor3f() {}

		//! ---|> ColorAttributeAccessor
		Util::Color4f getColor4f(uint32_t index)const override {
			assertRange(index);
			const float * v = _ptr<const float>(index);
			return Util::Color4f(v[0], v[1], v[2], 1.0);
		}
		//! ---|> ColorAttributeAccessor
		Util::Color4ub getColor4ub(uint32_t index)const override {
			assertRange(index);
			const float * v = _ptr<const float>(index);
			return Util::Color4ub(Util::Color4f(v[0], v[1], v[2], 1.0));
		}
		//! ---|> ColorAttributeAccessor
		void setColor(uint32_t index, const Util::Color4f & c) override {
			assertRange(index);
			float * v = _ptr<float>(index);
			v[0] = c.getR() , v[1] = c.getG() , v[2] = c.getB();
		}
		//! ---|> ColorAttributeAccessor
		void setColor(uint32_t index, const Util::Color4ub & cub) override {
			const Util::Color4f c(cub);
			assertRange(index);
			float * v = _ptr<float>(index);
			v[0] = c.getR() , v[1] = c.getG() , v[2] = c.getB();
		}
};

/*! ColorAttributeAccessor4f ---|> ColorAttributeAccessor	*/
class ColorAttributeAccessor4f : public ColorAttributeAccessor {
	public:
		ColorAttributeAccessor4f(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			ColorAttributeAccessor(_vData, _attribute) {}
		virtual ~ColorAttributeAccessor4f() {}

		//! ---|> ColorAttributeAccessor
		Util::Color4f getColor4f(uint32_t index)const override {
			assertRange(index);
			const float * v = _ptr<const float>(index);
			return Util::Color4f(v[0], v[1], v[2], v[3]);
		}
		//! ---|> ColorAttributeAccessor
		Util::Color4ub getColor4ub(uint32_t index)const override {
			assertRange(index);
			const float * v = _ptr<const float>(index);
			return Util::Color4ub(Util::Color4f(v[0], v[1], v[2], v[3]));
		}
		//! ---|> ColorAttributeAccessor
		void setColor(uint32_t index, const Util::Color4f & c) override {
			assertRange(index);
			float * v = _ptr<float>(index);
			v[0] = c.getR() , v[1] = c.getG() , v[2] = c.getB() , v[3] = c.getA();
		}
		//! ---|> ColorAttributeAccessor
		void setColor(uint32_t index, const Util::Color4ub & cub) override {
			const Util::Color4f c(cub);
			assertRange(index);
			float * v = _ptr<float>(index);
			v[0] = c.getR() , v[1] = c.getG() , v[2] = c.getB() , v[3] = c.getA();
		}
};

/*! ColorAttributeAccessor4ub ---|> ColorAttributeAccessor	*/
class ColorAttributeAccessor4ub : public ColorAttributeAccessor {
	public:
		ColorAttributeAccessor4ub(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			ColorAttributeAccessor(_vData, _attribute) {}
		virtual ~ColorAttributeAccessor4ub() {}

		//! ---|> ColorAttributeAccessor
		Util::Color4f getColor4f(uint32_t index)const override {
			assertRange(index);
			const uint8_t * v = _ptr<const uint8_t>(index);
			return Util::Color4ub(v[0], v[1], v[2], v[3]);
		}
		//! ---|> ColorAttributeAccessor
		Util::Color4ub getColor4ub(uint32_t index)const override {
			assertRange(index);
			const uint8_t * v = _ptr<const uint8_t>(index);
			return Util::Color4ub(v[0], v[1], v[2], v[3]);
		}
		//! ---|> ColorAttributeAccessor
		void setColor(uint32_t index, const Util::Color4f & cf) override {
			const Util::Color4ub c(cf);
			assertRange(index);
			uint8_t * v = _ptr<uint8_t>(index);
			v[0] = c.getR() , v[1] = c.getG() , v[2] = c.getB() , v[3] = c.getA();
		}
		//! ---|> ColorAttributeAccessor
		void setColor(uint32_t index, const Util::Color4ub & c) override {
			assertRange(index);
			uint8_t * v = _ptr<uint8_t>(index);
			v[0] = c.getR() , v[1] = c.getG() , v[2] = c.getB() , v[3] = c.getA();
		}
};


//! (static) Factory
Util::Reference<ColorAttributeAccessor> ColorAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getNumValues() >= 4 && attr.getDataType() == GL_FLOAT) {
		return new ColorAttributeAccessor4f(_vData, attr);
	} else if(attr.getNumValues() >= 3 && attr.getDataType() == GL_FLOAT) {
		return new ColorAttributeAccessor3f(_vData, attr);
	} else if(attr.getNumValues() >= 4 && attr.getDataType() == GL_UNSIGNED_BYTE) {
		return new ColorAttributeAccessor4ub(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

// ---------------------------------
// Normals

/*! NormalAttributeAccessor4b ---|> NormalAttributeAccessor */
class NormalAttributeAccessor4b : public NormalAttributeAccessor {
	public:
		NormalAttributeAccessor4b(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			NormalAttributeAccessor(_vData, _attribute) {}
		virtual ~NormalAttributeAccessor4b() {}

		//! ---|> NormalAttributeAccessor
		Geometry::Vec3 getNormal(uint32_t index)const override {
			assertRange(index);
			const int8_t * v = _ptr<const int8_t>(index);
			return Geometry::Vec3(Geometry::Convert::fromSignedTo<float>(v[0]),
								  Geometry::Convert::fromSignedTo<float>(v[1]),
								  Geometry::Convert::fromSignedTo<float>(v[2]));
		}

		//! ---|> NormalAttributeAccessor
		void setNormal(uint32_t index, const Geometry::Vec3 & n) override {
			assertRange(index);
			int8_t * v = _ptr<int8_t>(index);
			v[0] = Geometry::Convert::toSigned<int8_t>(n.x());
			v[1] = Geometry::Convert::toSigned<int8_t>(n.y());
			v[2] = Geometry::Convert::toSigned<int8_t>(n.z());
		}
};

/*! NormalAttributeAccessor3f ---|> NormalAttributeAccessor */
class NormalAttributeAccessor3f : public NormalAttributeAccessor {
	public:
		NormalAttributeAccessor3f(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			NormalAttributeAccessor(_vData, _attribute) {}
		virtual ~NormalAttributeAccessor3f() {}

		//! ---|> NormalAttributeAccessor
		Geometry::Vec3 getNormal(uint32_t index)const override {
			assertRange(index);
			const float * v = _ptr<const float>(index);
			return Geometry::Vec3(v[0], v[1], v[2]);
		}

		//! ---|> NormalAttributeAccessor
		void setNormal(uint32_t index, const Geometry::Vec3 & n) override {
			assertRange(index);
			float * v = _ptr<float>(index);
			v[0] = n.x() , v[1] = n.y() , v[2] = n.z();
		}
};

//! (static)
Util::Reference<NormalAttributeAccessor> NormalAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getNumValues() >= 3 && attr.getDataType() == GL_FLOAT) {
		return new NormalAttributeAccessor3f(_vData, attr);
	} else if(attr.getNumValues() >= 4 && attr.getDataType() == GL_BYTE) {
		return new NormalAttributeAccessor4b(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

// ---------------------------------
// Position

//! (static)
Util::Reference<PositionAttributeAccessor> PositionAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getNumValues() >= 3 && attr.getDataType() == GL_FLOAT) {
		return new PositionAttributeAccessor(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

// ---------------------------------
// TexCoord

//! (static)
Util::Reference<TexCoordAttributeAccessor> TexCoordAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getNumValues() == 2 && attr.getDataType() == GL_FLOAT) {
		return new TexCoordAttributeAccessor(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

}
