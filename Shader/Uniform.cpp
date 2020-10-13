/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Uniform.h"
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>
#include <Geometry/Matrix3x3.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <vector>

namespace Rendering {

//! (static)
const Uniform Uniform::nullUniform;

//! dataType_t ---> bytes per value
const size_t Uniform::dataSizeIndex[] = {
	//	UNIFORM_BOOL = 0, 	UNIFORM_VEC2B = 1,		UNIFORM_VEC3B = 2,		UNIFORM_VEC4B = 3,
	sizeof(int32_t) * 1,	sizeof(int32_t) * 2,	sizeof(int32_t) * 3,	sizeof(int32_t) * 4,
	//	UNIFORM_FLOAT = 4, 	UNIFORM_VEC2F = 5,		UNIFORM_VEC3F = 6,		UNIFORM_VEC4F = 7,
	sizeof(float) * 1,		sizeof(float) * 2,		sizeof(float) * 3,		sizeof(float) * 4,
	//	UNIFORM_INT = 8, 	UNIFORM_VEC2I = 9,		UNIFORM_VEC3I = 10,		UNIFORM_VEC4I = 11,
	sizeof(int32_t) * 1,	sizeof(int32_t) * 2,	sizeof(int32_t) * 3,	sizeof(int32_t) * 4,
	//	UNIFORM_MATRIX_2X2F = 12,	UNIFORM_MATRIX_3X3F = 13, 	UNIFORM_MATRIX_4X4F = 14
	sizeof(float) * 4,				sizeof(float) * 9,			sizeof(float) * 16,
	//	UNIFORM_UINT = 15, 	UNIFORM_VEC2UI = 16,		UNIFORM_VEC3UI = 17,		UNIFORM_VEC4UI = 18,
	//sizeof(uint32_t) * 1,	sizeof(uint32_t) * 2,	sizeof(uint32_t) * 3,	sizeof(uint32_t) * 4
};

std::string Uniform::getTypeString(const dataType_t t) {
	switch(t) {
		case UNIFORM_BOOL: return "bool";
		case UNIFORM_VEC2B: return "vec2b";
		case UNIFORM_VEC3B: return "vec3b";
		case UNIFORM_VEC4B: return "vec4b";
		case UNIFORM_FLOAT: return "float";
		case UNIFORM_VEC2F: return "vec2f";
		case UNIFORM_VEC3F: return "vec3f";
		case UNIFORM_VEC4F: return "vec4f";
		case UNIFORM_INT: return "int";
		case UNIFORM_VEC2I: return "vec2i";
		case UNIFORM_VEC3I: return "vec3i";
		case UNIFORM_VEC4I: return "vec4i";
		case UNIFORM_MATRIX_2X2F: return "mat2x2f";
		case UNIFORM_MATRIX_3X3F: return "mat3x3f";
		case UNIFORM_MATRIX_4X4F: return "mat4x4f";
		default: return "unknown";
	}
}

// generic ---------------------------------------------------------------

//! (ctor)
Uniform::Uniform() :
		name(""), type(UNIFORM_FLOAT), numValues(0), data() {
}

//! (ctor)
Uniform::Uniform(UniformName _name, dataType_t _type, size_t _numValues) :
		name(std::move(_name)), type(_type), numValues(_numValues), data(numValues * getValueSize(type)){
}

//! (ctor)
Uniform::Uniform(UniformName _name, dataType_t _type, size_t _numValues,std::vector<uint8_t> _data) :
		name(std::move(_name)), type(_type), numValues(_numValues), data(std::move(_data)){
	if(data.size()!=_numValues * getValueSize(type))
		INVALID_ARGUMENT_EXCEPTION("data is of wrong size");
}


// generic ---------------------------------------------------------------


//! (ctor) UNIFORM_BOOL | UNIFORM_VEC(2|3|4)B *
Uniform::Uniform(UniformName _name, dataType_t _type, const std::deque<bool> & values) :
		name(std::move(_name)), type(_type),
		numValues( (values.size()*sizeof(int32_t)) /getValueSize(type)),
		data(numValues * getValueSize(type)) {
	// check type
	if( type!=UNIFORM_BOOL && type!=UNIFORM_VEC2B && type!=UNIFORM_VEC3B && type!=UNIFORM_VEC4B)
		INVALID_ARGUMENT_EXCEPTION("Only bool-types accepted here");
	// check size
	if(values.size()*sizeof(uint32_t) != numValues * getValueSize(type))
		INVALID_ARGUMENT_EXCEPTION("wrong value count for type");

	int32_t * ptr = reinterpret_cast<int32_t *>(data.data());
	size_t idx = 0;
	for(const auto & val : values) {
		ptr[idx++] = val ? 1 : 0;
	}
}

//! (ctor) UNIFORM_FLOAT | UNIFORM_VEC(2|3|4)F | UNIFORM_MATRIX_(2X2|3X3|4X4)F *
Uniform::Uniform(UniformName _name, dataType_t _type, const std::vector<float> & values) :
		name(std::move(_name)), type(_type),
		numValues( (values.size()*sizeof(float)) /getValueSize(type)),
		data(numValues * getValueSize(type)) {
	// check type
	if( type!=UNIFORM_FLOAT && type!=UNIFORM_VEC2F && type!=UNIFORM_VEC3F && type!=UNIFORM_VEC4F &&
			type!=UNIFORM_MATRIX_2X2F && type!=UNIFORM_MATRIX_3X3F && type!=UNIFORM_MATRIX_4X4F )
		INVALID_ARGUMENT_EXCEPTION("Only float-types accepted here");
	// check size
	if(values.size()*sizeof(float) != numValues * getValueSize(type))
		INVALID_ARGUMENT_EXCEPTION("wrong value count for type");

	float * ptr = reinterpret_cast<float *>(data.data());
	std::copy(std::begin(values), std::end(values), ptr);
}

//! (ctor) UNIFORM_INT | UNIFORM_VEC(2|3|4)I *
Uniform::Uniform(UniformName _name, dataType_t _type, const std::vector<int32_t> & values) :
		name(std::move(_name)), type(_type),
		numValues( (values.size()*sizeof(int32_t)) /getValueSize(type)),
		data(numValues * getValueSize(type)) {
	// check type
	if( type!=UNIFORM_INT && type!=UNIFORM_VEC2I && type!=UNIFORM_VEC3I && type!=UNIFORM_VEC4I)
		INVALID_ARGUMENT_EXCEPTION("Only int-types accepted here");
	// check size
	if(values.size()*sizeof(int32_t) != numValues * getValueSize(type))
		INVALID_ARGUMENT_EXCEPTION("wrong value count for type");

	int32_t * ptr = reinterpret_cast<int32_t *>(data.data());
	std::copy(std::begin(values), std::end(values), ptr);
}


// bool ---------------------------------------------------------------

//! (ctor) UNIFORM_BOOL
Uniform::Uniform(UniformName _name, bool value) :
		name(std::move(_name)), type(UNIFORM_BOOL), numValues(1), data(numValues * getValueSize(type)) {
	*reinterpret_cast<int32_t *>(data.data()) = value ? 1 : 0;
}

//! (ctor) UNIFORM_BOOL *
Uniform::Uniform(UniformName _name, const std::deque<bool> & values) :
		name(std::move(_name)), type(UNIFORM_BOOL), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	int32_t * ptr = reinterpret_cast<int32_t *>(data.data());
	size_t idx = 0;
	for(const auto & val : values) {
		ptr[idx++] = val ? 1 : 0;
	}
}


// float ---------------------------------------------------------------

//! (ctor) UNIFORM_FLOAT
Uniform::Uniform(UniformName _name, float value) :
		name(std::move(_name)), type(UNIFORM_FLOAT), numValues(1),
		data(reinterpret_cast<const uint8_t *>(&value), reinterpret_cast<const uint8_t *>(&value) + sizeof(float)) {
}

//! (ctor) UNIFORM_FLOAT *
Uniform::Uniform(UniformName _name, const std::vector<float> & values) :
		name(std::move(_name)), type(UNIFORM_FLOAT), numValues(values.size()),
		data(reinterpret_cast<const uint8_t *>(&values[0]), reinterpret_cast<const uint8_t *>(&values[0]) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC2F
Uniform::Uniform(UniformName _name, const Geometry::Vec2 & value) :
		name(std::move(_name)), type(UNIFORM_VEC2F), numValues(1),
		data(reinterpret_cast<const uint8_t *>(value.getVec()), reinterpret_cast<const uint8_t *>(value.getVec()) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC2F *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Vec2> & values) :
		name(std::move(_name)), type(UNIFORM_VEC2F), numValues(values.size()),
		data(numValues * getValueSize(type)) {
	float * ptr = reinterpret_cast<float *>(data.data());
	size_t idx = 0;
	for(const auto & vec : values) {
		ptr[idx++] = vec.getX();
		ptr[idx++] = vec.getY();
	}
}

//! (ctor) UNIFORM_VEC3F
Uniform::Uniform(UniformName _name, const Geometry::Vec3 & value) :
		name(std::move(_name)), type(UNIFORM_VEC3F), numValues(1),
		data(reinterpret_cast<const uint8_t *>(value.getVec()), reinterpret_cast<const uint8_t *>(value.getVec()) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC3F *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Vec3> & values) :
		name(std::move(_name)), type(UNIFORM_VEC3F), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	float * ptr = reinterpret_cast<float *>(data.data());
	size_t idx = 0;
	for(const auto & vec : values) {
		ptr[idx++] = vec.getX();
		ptr[idx++] = vec.getY();
		ptr[idx++] = vec.getZ();
	}
}

//! (ctor) UNIFORM_VEC4F
Uniform::Uniform(UniformName _name, const Geometry::Vec4 & value) :
		name(std::move(_name)), type(UNIFORM_VEC4F), numValues(1),
		data(reinterpret_cast<const uint8_t *>(value.getVec()), reinterpret_cast<const uint8_t *>(value.getVec()) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC4F *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Vec4> & values) :
		name(std::move(_name)), type(UNIFORM_VEC4F), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	float * ptr = reinterpret_cast<float *>(data.data());
	size_t idx = 0;
	for(const auto & vec : values) {
		ptr[idx++] = vec.getX();
		ptr[idx++] = vec.getY();
		ptr[idx++] = vec.getZ();
		ptr[idx++] = vec.getW();
	}
}

//! (ctor) UNIFORM_VEC4F (Color)
Uniform::Uniform(UniformName _name, const Util::Color4f & color) :
		name(std::move(_name)), type(UNIFORM_VEC4F), numValues(1),
		data(reinterpret_cast<const uint8_t *>(color.data()), reinterpret_cast<const uint8_t *>(color.data()) + numValues * getValueSize(type)) {
}

// int ---------------------------------------------------------------

//! (ctor) UNIFORM_INT
Uniform::Uniform(UniformName _name, int32_t value) :
		name(std::move(_name)), type(UNIFORM_INT), numValues(1),
		data(reinterpret_cast<const uint8_t *>(&value), reinterpret_cast<const uint8_t *>(&value) + sizeof(int32_t)) {
}

//! (ctor) UNIFORM_INT *
Uniform::Uniform(UniformName _name, const std::vector<int32_t> & values) :
		name(std::move(_name)), type(UNIFORM_INT), numValues(values.size()),
		data(reinterpret_cast<const uint8_t *>(&values[0]), reinterpret_cast<const uint8_t *>(&values[0]) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC2I
Uniform::Uniform(UniformName _name, const Geometry::Vec2i & value) :
		name(std::move(_name)), type(UNIFORM_VEC2I), numValues(1),
		data(reinterpret_cast<const uint8_t *>(value.getVec()), reinterpret_cast<const uint8_t *>(value.getVec()) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC2I *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Vec2i> & values) :
		name(std::move(_name)), type(UNIFORM_VEC2I), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	int32_t * ptr = reinterpret_cast<int32_t *>(data.data());
	size_t idx = 0;
	for(const auto & vec : values) {
		ptr[idx++] = vec.getX();
		ptr[idx++] = vec.getY();
	}
}

//! (ctor) UNIFORM_VEC3I
Uniform::Uniform(UniformName _name, const Geometry::Vec3i & value) :
		name(std::move(_name)), type(UNIFORM_VEC3I), numValues(1),
		data(reinterpret_cast<const uint8_t *>(value.getVec()), reinterpret_cast<const uint8_t *>(value.getVec()) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC3I *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Vec3i> & values) :
		name(std::move(_name)), type(UNIFORM_VEC3I), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	int32_t * ptr = reinterpret_cast<int32_t *>(data.data());
	size_t idx = 0;
	for(const auto & vec : values) {
		ptr[idx++] = vec.getX();
		ptr[idx++] = vec.getY();
		ptr[idx++] = vec.getZ();
	}
}

//! (ctor) UNIFORM_VEC4I
Uniform::Uniform(UniformName _name, const Geometry::Vec4i & value) :
		name(std::move(_name)), type(UNIFORM_VEC4I), numValues(1),
		data(reinterpret_cast<const uint8_t *>(value.getVec()), reinterpret_cast<const uint8_t *>(value.getVec()) + numValues * getValueSize(type)) {
}

//! (ctor) UNIFORM_VEC4I *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Vec4i> & values) :
		name(std::move(_name)), type(UNIFORM_VEC4I), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	int32_t * ptr = reinterpret_cast<int32_t *>(data.data());
	size_t idx = 0;
	for(const auto & vec : values) {
		ptr[idx++] = vec.getX();
		ptr[idx++] = vec.getY();
		ptr[idx++] = vec.getZ();
		ptr[idx++] = vec.getW();
	}
}


// float matrixes -------------------------------------------------------------------------------

//! (ctor) UNIFORM_MATRIX_3X3F
Uniform::Uniform(UniformName _name, const Geometry::Matrix3x3 & value) :
		name(std::move(_name)), type(UNIFORM_MATRIX_3X3F), numValues(1),
		data(numValues * getValueSize(type)) {
	float * ptr = reinterpret_cast<float *>(data.data());
	size_t idx = 0;
	for(uint_fast8_t i=0;i<3;++i){
		for(uint_fast8_t j=0;j<3;++j){
			// Transpose the matrix here.
			ptr[idx++] = value.at(j, i);
		}
	}
}

//! (ctor) UNIFORM_MATRIX_3X3F *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Matrix3x3> & values) :
		name(std::move(_name)), type(UNIFORM_MATRIX_3X3F), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	float * ptr = reinterpret_cast<float *>(data.data());
	size_t idx = 0;
	for(const auto & matrix : values) {
		for(uint_fast8_t i=0;i<3;++i){
			for(uint_fast8_t j=0;j<3;++j){
				// Transpose the matrix here.
				ptr[idx++] = matrix.at(j, i);
			}
		}
	}
}

//! (ctor) UNIFORM_MATRIX_4X4F
Uniform::Uniform(UniformName _name, const Geometry::Matrix4x4 & value) :
		name(std::move(_name)), type(UNIFORM_MATRIX_4X4F), numValues(1),
		data(numValues * getValueSize(type)) {
	const Geometry::Matrix4x4 transposed = value.getTransposed();
	float * ptr = reinterpret_cast<float *>(data.data());
	std::copy(transposed.getData(), transposed.getData() + 16, ptr);
}

//! (ctor) UNIFORM_MATRIX_4X4F *
Uniform::Uniform(UniformName _name, const std::vector<Geometry::Matrix4x4> & values) :
		name(std::move(_name)), type(UNIFORM_MATRIX_4X4F), numValues(values.size()),
		data(numValues * getValueSize(type)) {

	float * ptr = reinterpret_cast<float *>(data.data());
	for(const auto & matrix : values) {
		const Geometry::Matrix4x4 transposed = matrix.getTransposed();
		std::copy(transposed.getData(), transposed.getData() + 16, ptr);
		ptr += 16;
	}
}

std::string Uniform::toString() const {
	std::stringstream s;
	s << "Uniform: '" << getName() << "' ";

	switch (getType()) {
		case UNIFORM_BOOL: {
			s << "bool["<<numValues<<"]";

			const int32_t * ptr = reinterpret_cast<const int32_t *> (data.data());
			for (size_t i = 0; i < numValues; ++i)
				s << " " << ptr[i];

			break;
		}
		case UNIFORM_VEC2B: {
			s << "vec2b["<<numValues<<"] (?)"; // unimplemented
			break;
		}
		case UNIFORM_VEC3B: {
			s << "vec3b["<<numValues<<"] (?)";// unimplemented
			break;
		}
		case UNIFORM_VEC4B: {
			s << "vec4b["<<numValues<<"] (?)";// unimplemented
			break;
		}

		case UNIFORM_FLOAT: {
			s << "float["<<numValues<<"]";

			const float * ptr = reinterpret_cast<const float *> (data.data());
			for (size_t i = 0; i < numValues; ++i)
				s << " " << ptr[i];
			break;
		}

		case UNIFORM_VEC2F: {
			s << "vec2f["<<numValues<<"]";

			const float * ptr = reinterpret_cast<const float *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " (" << ptr[0] << "," << ptr[1] << ")";
				ptr+=2;
			}
			break;
		}
		case UNIFORM_VEC3F: {
			s << "vec3f["<<numValues<<"]";

			const float * ptr = reinterpret_cast<const float *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " (" << ptr[0] << "," << ptr[1] << "," << ptr[2] << ")";
				ptr+=3;
			}
			break;
		}
		case UNIFORM_VEC4F: {
			s << "vec4f["<<numValues<<"]";

			const float * ptr = reinterpret_cast<const float *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " (" << ptr[0] << "," << ptr[1] << "," << ptr[2] << "," << ptr[3] << ")";
				ptr+=4;
			}
			break;
		}
		case UNIFORM_INT: {
			s << "int["<<numValues<<"]";

			const int32_t * ptr = reinterpret_cast<const int32_t *> (data.data());
			for (size_t i = 0; i < numValues; ++i)
				s << " " << ptr[i];
			break;
		}

		case UNIFORM_VEC2I: {
			s << "vec2i["<<numValues<<"]";

			const int32_t * ptr = reinterpret_cast<const int32_t *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " (" << ptr[0] << "," << ptr[1] << ")";
				ptr+=2;
			}
			break;
		}
		case UNIFORM_VEC3I: {
			s << "vec3i["<<numValues<<"]";

			const int32_t * ptr = reinterpret_cast<const int32_t *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " (" << ptr[0] << "," << ptr[1] << "," << ptr[2] << ")";
				ptr+=3;
			}
			break;
		}
		case UNIFORM_VEC4I: {
			s << "vec4i["<<numValues<<"]";

			const int32_t * ptr = reinterpret_cast<const int32_t *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " (" << ptr[0] << "," << ptr[1] << "," << ptr[2] << "," << ptr[3] << ")";
				ptr+=4;
			}
			break;
		}

		case UNIFORM_MATRIX_2X2F: {
			s << "matrix2x2["<<numValues<<"] (?)";// unimplemented
			break;
		}

		case UNIFORM_MATRIX_3X3F: {
			s << "matrix3x3["<<numValues<<"]";
			const float * ptr = reinterpret_cast<const float *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " ("<<ptr[0];
				for (size_t j = 1; j < 9; ++j)
					s<<","<<ptr[j];
				s << ")";
				ptr+=9;
			}
			break;
		}

		case UNIFORM_MATRIX_4X4F: {
			s << "matrix4x4["<<numValues<<"]";
			const float * ptr = reinterpret_cast<const float *> (data.data());
			for (size_t i = 0; i < numValues; ++i) {
				s << " ("<<ptr[0];
				for (size_t j = 1; j < 16; ++j)
					s<<","<<ptr[j];
				s << ")";
				ptr+=16;
			}
			break;
		}
		default:
			WARN("unexpected case in switch statement");
	}
	return s.str();
}

}
