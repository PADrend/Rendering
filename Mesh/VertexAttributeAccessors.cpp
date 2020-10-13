/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VertexAttributeAccessors.h"
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

//! (internal)
void VertexAttributeAccessor::assertNumValues(uint32_t index, uint32_t count) const {
	uint32_t num = this->getAttribute().getNumValues();
	if( num != count ) {
		std::ostringstream s;
		s << "Trying to access " << count << " number of values of vertex " << index << " of overall " << num << " values.";
		throw std::range_error(s.str());
	}
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
	if(attr.getNumValues() >= 4 && attr.getDataType() == Util::TypeConstant::FLOAT) {
		return new ColorAttributeAccessor4f(_vData, attr);
	} else if(attr.getNumValues() >= 3 && attr.getDataType() == Util::TypeConstant::FLOAT) {
		return new ColorAttributeAccessor3f(_vData, attr);
	} else if(attr.getNumValues() >= 4 && attr.getDataType() == Util::TypeConstant::UINT8) {
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
			v[3] = 0;
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
	if(attr.getNumValues() >= 3 && attr.getDataType() == Util::TypeConstant::FLOAT) {
		return new NormalAttributeAccessor3f(_vData, attr);
	} else if(attr.getNumValues() >= 4 && attr.getDataType() == Util::TypeConstant::INT8) {
		return new NormalAttributeAccessor4b(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

// ---------------------------------
// Position

/*! PositionAttributeAccessorF ---|> PositionAttributeAccessor */
class PositionAttributeAccessorF : public PositionAttributeAccessor {
	public:
		PositionAttributeAccessorF(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			PositionAttributeAccessor(_vData, _attribute) {}
		virtual ~PositionAttributeAccessorF() {}

		//! ---|> PositionAttributeAccessor
		const Geometry::Vec3 getPosition(uint32_t index) const override {
			assertRange(index);
			const float * v=_ptr<const float>(index);
			return Geometry::Vec3(v[0],v[1],v[2]);
		}
		
		//! ---|> PositionAttributeAccessor
		void setPosition(uint32_t index,const Geometry::Vec3 & p) override {
			assertRange(index);
			float * v=_ptr<float>(index);
			v[0] = p.x() , v[1] = p.y() , v[2] = p.z();
		}
};

/*! PositionAttributeAccessorHF ---|> PositionAttributeAccessor */
class PositionAttributeAccessorHF : public PositionAttributeAccessor {
	public:
		PositionAttributeAccessorHF(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			PositionAttributeAccessor(_vData, _attribute) {}
		virtual ~PositionAttributeAccessorHF() {}

		//! ---|> PositionAttributeAccessor
		const Geometry::Vec3 getPosition(uint32_t index) const override {
			assertRange(index);
			const uint16_t * v=_ptr<const uint16_t>(index);			
			return Geometry::Vec3(
				Geometry::Convert::halfToFloat(v[0]),
				Geometry::Convert::halfToFloat(v[1]),
				Geometry::Convert::halfToFloat(v[2])
			);
		}
		
		//! ---|> PositionAttributeAccessor
		void setPosition(uint32_t index,const Geometry::Vec3 & p) override {
			assertRange(index);
			uint16_t * v=_ptr<uint16_t>(index);
			v[0] = Geometry::Convert::floatToHalf(p.x());
			v[1] = Geometry::Convert::floatToHalf(p.y());
			v[2] = Geometry::Convert::floatToHalf(p.z());
		}
};

//! (static)
Util::Reference<PositionAttributeAccessor> PositionAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getNumValues() >= 3 && attr.getDataType() == Util::TypeConstant::FLOAT) {
		return new PositionAttributeAccessorF(_vData, attr);
	} else if(attr.getNumValues() >= 3 && attr.getDataType() == Util::TypeConstant::HALF) {
		return new PositionAttributeAccessorHF(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

// ---------------------------------
// TexCoord

//! (static)
Util::Reference<TexCoordAttributeAccessor> TexCoordAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getNumValues() == 2 && attr.getDataType() == Util::TypeConstant::FLOAT) {
		return new TexCoordAttributeAccessor(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

// ---------------------------------
// Float

/*! FloatAttributeAccessorub ---|> FloatAttributeAccessor */
class FloatAttributeAccessorub : public FloatAttributeAccessor {
	public:
		FloatAttributeAccessorub(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			FloatAttributeAccessor(_vData, _attribute) {}
		virtual ~FloatAttributeAccessorub() {}
		
		//! ---|> FloatAttributeAccessor
		float getValue(uint32_t index) const override {
			assertRange(index);
			const uint8_t * v = _ptr<const uint8_t>(index);
			return Geometry::Convert::fromUnsignedTo<float>(v[0]);
		}

		//! ---|> FloatAttributeAccessor
		void setValue(uint32_t index, float value) override {
			assertRange(index);
			uint8_t * v = _ptr<uint8_t>(index);
			v[0] = Geometry::Convert::toUnsigned<uint8_t>(value);
		}

		//! ---|> FloatAttributeAccessor
		const std::vector<float> getValues(uint32_t index) const override {
			assertRange(index);
			const uint8_t * v = _ptr<const uint8_t>(index);
			std::vector<float> out(getAttribute().getNumValues());
			for(uint32_t i=0; i<out.size(); ++i)
				out[i] = Geometry::Convert::fromUnsignedTo<float>(v[i]);
			return out;
		}

		//! ---|> FloatAttributeAccessor
		void setValues(uint32_t index, const float* values, uint32_t count) override {
			assertRange(index);
			count = std::min<uint32_t>(count, getAttribute().getNumValues());
			uint8_t * v = _ptr<uint8_t>(index);
			for(uint32_t i=0; i<count; ++i)
				v[i] = Geometry::Convert::toUnsigned<uint8_t>(values[i]);
		}
};

/*! FloatAttributeAccessorb ---|> FloatAttributeAccessor */
class FloatAttributeAccessorb : public FloatAttributeAccessor {
	public:
		FloatAttributeAccessorb(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			FloatAttributeAccessor(_vData, _attribute) {}
		virtual ~FloatAttributeAccessorb() {}
		
		//! ---|> FloatAttributeAccessor
		float getValue(uint32_t index) const override {
			assertRange(index);
			const int8_t * v = _ptr<const int8_t>(index);
			return Geometry::Convert::fromSignedTo<float>(v[0]);
		}

		//! ---|> FloatAttributeAccessor
		void setValue(uint32_t index, float value) override {
			assertRange(index);
			int8_t * v = _ptr<int8_t>(index);
			v[0] = Geometry::Convert::toSigned<int8_t>(value);
		}

		//! ---|> FloatAttributeAccessor
		const std::vector<float> getValues(uint32_t index) const override {
			assertRange(index);
			const int8_t * v = _ptr<const int8_t>(index);
			std::vector<float> out(getAttribute().getNumValues());
			for(uint32_t i=0; i<out.size(); ++i)
				out[i] = Geometry::Convert::fromSignedTo<float>(v[i]);
			return out;
		}

		//! ---|> FloatAttributeAccessor
		void setValues(uint32_t index, const float* values, uint32_t count) override {
			assertRange(index);
			count = std::min<uint32_t>(count, getAttribute().getNumValues());
			int8_t * v = _ptr<int8_t>(index);
			for(uint32_t i=0; i<count; ++i)
				v[i] = Geometry::Convert::toSigned<int8_t>(values[i]);
		}
};

/*! FloatAttributeAccessorHF ---|> FloatAttributeAccessor */
class FloatAttributeAccessorHF : public FloatAttributeAccessor {
	public:
		FloatAttributeAccessorHF(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			FloatAttributeAccessor(_vData, _attribute) {}
		virtual ~FloatAttributeAccessorHF() {}

		//! ---|> FloatAttributeAccessor
		float getValue(uint32_t index) const override {
			assertRange(index);
			const uint16_t * v = _ptr<const uint16_t>(index);
			return Geometry::Convert::halfToFloat(v[0]);
		}
		
		//! ---|> FloatAttributeAccessor
		void setValue(uint32_t index, float value) override {
			assertRange(index);
			uint16_t * v = _ptr<uint16_t>(index);
			v[0] = Geometry::Convert::floatToHalf(value);
		}
		
		//! ---|> FloatAttributeAccessor
		const std::vector<float> getValues(uint32_t index) const override {
			assertRange(index);
			const uint16_t * v = _ptr<const uint16_t>(index);
			std::vector<float> out(getAttribute().getNumValues());
			for(uint32_t i=0; i<out.size(); ++i)
				out[i] = Geometry::Convert::halfToFloat(v[i]);
			return out;
		}
		
		//! ---|> FloatAttributeAccessor
		void setValues(uint32_t index, const float* values, uint32_t count) override {
			assertRange(index);
			count = std::min<uint32_t>(count, getAttribute().getNumValues());
			uint8_t * v = _ptr<uint8_t>(index);
			for(uint32_t i=0; i<count; ++i)
				v[i] = Geometry::Convert::floatToHalf(values[i]);
		}
};

/*! FloatAttributeAccessorf ---|> FloatAttributeAccessor */
class FloatAttributeAccessorf : public FloatAttributeAccessor {
	public:
		FloatAttributeAccessorf(MeshVertexData & _vData, const VertexAttribute & _attribute) :
			FloatAttributeAccessor(_vData, _attribute) {}
		virtual ~FloatAttributeAccessorf() {}

		//! ---|> FloatAttributeAccessor
		float getValue(uint32_t index) const override {
			assertRange(index);
			const float * v=_ptr<const float>(index);
			return v[0];
		}
		
		//! ---|> FloatAttributeAccessor
		void setValue(uint32_t index, float value) override {
			assertRange(index);
			float * v=_ptr<float>(index);
			v[0] = value;
		}
		
		//! ---|> FloatAttributeAccessor
		const std::vector<float> getValues(uint32_t index) const override {
			assertRange(index);
			const float * v=_ptr<const float>(index);
			return std::vector<float>(v, v + getAttribute().getNumValues());
		}
		
		//! ---|> FloatAttributeAccessor
		void setValues(uint32_t index, const float* values, uint32_t count) override {
			assertRange(index);
			count = std::min<uint32_t>(count, getAttribute().getNumValues());
			float * v=_ptr<float>(index);
			std::copy(values, values + count, v);
		}
};

//! (static)
Util::Reference<FloatAttributeAccessor> FloatAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getDataType() == Util::TypeConstant::FLOAT) {
		return new FloatAttributeAccessorf(_vData, attr);
	} else if(attr.getDataType() == Util::TypeConstant::INT8) {
		return new FloatAttributeAccessorb(_vData, attr);
	} else if(attr.getDataType() == Util::TypeConstant::UINT8) {
		return new FloatAttributeAccessorub(_vData, attr);
	} else if(attr.getDataType() == Util::TypeConstant::HALF) {
		return new FloatAttributeAccessorHF(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

// ---------------------------------
// Unisigned Integer

class UIntAttributeAccessorUI : public UIntAttributeAccessor {
public:
	UIntAttributeAccessorUI(MeshVertexData & _vData,const VertexAttribute & _attribute) :
			UIntAttributeAccessor(_vData,_attribute) {}
	virtual ~UIntAttributeAccessorUI() = default;

	uint32_t getValue(uint32_t index) const  override {
		assertRange(index);
		const uint32_t * v=_ptr<const uint32_t>(index);
		return v[0];
	}

	void setValue(uint32_t index, uint32_t value)  override {
		assertRange(index);
		uint32_t * v=_ptr<uint32_t>(index);
		v[0] = value;
	}

	const std::vector<uint32_t> getValues(uint32_t index) const override {
		assertRange(index);
		const uint32_t * v=_ptr<const uint32_t>(index);
		return std::vector<uint32_t>(v, v + getAttribute().getNumValues());
	}

	void setValues(uint32_t index, const uint32_t* values, uint32_t count) override {
		assertRange(index);
		count = std::min<uint32_t>(count, getAttribute().getNumValues());
		uint32_t * v=_ptr<uint32_t>(index);
		std::copy(values, values + count, v);
	}
};

//! (static)
Util::Reference<UIntAttributeAccessor> UIntAttributeAccessor::create(MeshVertexData & _vData, Util::StringIdentifier name) {
	const VertexAttribute & attr = assertAttribute(_vData, name);
	if(attr.getDataType() == Util::TypeConstant::UINT32) {
		return new UIntAttributeAccessorUI(_vData, attr);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
	}
}

}
