/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_UNIFORM_H
#define RENDERING_UNIFORM_H

#include <Util/StringIdentifier.h>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

// Forward declarations
namespace Geometry {

template<typename _T> class _Vec2;
typedef _Vec2<float> Vec2;
typedef _Vec2<float> Vec2f;
typedef _Vec2<int32_t> Vec2i;

template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3;
typedef _Vec3<float> Vec3f;
typedef _Vec3<int32_t> Vec3i;

template<typename _T> class _Vec4;
typedef _Vec4<float> Vec4;
typedef _Vec4<float> Vec4f;
typedef _Vec4<int32_t> Vec4i;

template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4;

template<typename _T> class _Matrix3x3;
typedef _Matrix3x3<float> Matrix3x3;





}
namespace Util {
class Color4f;
}

// ------------------------------

namespace Rendering {

/***
 ** Uniform
 **/
class Uniform {
		//! dataType_t ---> bytes per value
		static const size_t dataSizeIndex[];

	public:
		//! \note if something is changed here, make sure that the dataSize-index is also updated.
		enum dataType_t {
			UNIFORM_BOOL = 0, 		UNIFORM_VEC2B = 1,	UNIFORM_VEC3B = 2,	UNIFORM_VEC4B = 3,
			UNIFORM_FLOAT = 4, 		UNIFORM_VEC2F = 5,	UNIFORM_VEC3F = 6,	UNIFORM_VEC4F = 7,
			UNIFORM_INT = 8, 		UNIFORM_VEC2I = 9,	UNIFORM_VEC3I = 10,	UNIFORM_VEC4I = 11,
			UNIFORM_MATRIX_2X2F = 12,	UNIFORM_MATRIX_3X3F = 13, UNIFORM_MATRIX_4X4F = 14
		};

		//! returns the size in bytes of a value of the given type
		static size_t getValueSize(const dataType_t t){		return dataSizeIndex[t];	}


		class UniformName{
				Util::StringIdentifier id;
		public:
				UniformName() : id(0) {}
				UniformName(const char * s) : id(Util::StringIdentifier(s)) {}
				UniformName(const std::string & s) : id(Util::StringIdentifier(s)) {}
				UniformName(Util::StringIdentifier _id) : id(_id) {}

				const std::string getString()const				{	return id.toString();	}
				Util::StringIdentifier getStringId()const				{	return id;	}
				bool operator==(const UniformName & other)const	{	return id==other.id;	}
		};
		static const Uniform nullUniform;

		Uniform();
		Uniform(const UniformName & _name, dataType_t _type, size_t arraySize);
		Uniform(const UniformName & _name, dataType_t _type, size_t arraySize,const std::vector<uint8_t> & data);

		/*! Generic bool-constructor (use another contructor whenever possible)
			\throw may throw an invalid_argument-exception	*/
		Uniform(const UniformName & _name, dataType_t _type, const std::deque<bool> & values);
		/*! Generic float-constructor (use another contructor whenever possible)
			\throw may throw an invalid_argument-exception	*/
		Uniform(const UniformName & _name, dataType_t _type, const std::vector<float> & values);
		/*! Generic int-constructor (use another contructor whenever possible)
			\throw may throw an invalid_argument-exception	*/
		Uniform(const UniformName & _name, dataType_t _type, const std::vector<int32_t> & values);

		//! UNIFORM_BOOL
		Uniform(const UniformName & _name, bool value);
		Uniform(const UniformName & _name, const std::deque<bool> & values);

		//! UNIFORM_FLOAT
		Uniform(const UniformName & _name, float value);
		Uniform(const UniformName & _name, const std::vector<float> & values);

		//! UNIFORM_VEC2F
		Uniform(const UniformName & _name, const Geometry::Vec2 & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Vec2> & values);

		//! UNIFORM_VEC3F
		Uniform(const UniformName & _name, const Geometry::Vec3 & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Vec3> & values);

		//! UNIFORM_VEC4F
		Uniform(const UniformName & _name, const Geometry::Vec4 & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Vec4> & values);
		Uniform(const UniformName & _name, const Util::Color4f & color);

		//! UNIFORM_INT
		Uniform(const UniformName & _name, int32_t value);
		Uniform(const UniformName & _name, const std::vector<int32_t> & values);

		//! UNIFORM_VEC2I
		Uniform(const UniformName & _name, const Geometry::Vec2i & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Vec2i> & values);

		//! UNIFORM_VEC3I
		Uniform(const UniformName & _name, const Geometry::Vec3i & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Vec3i> & values);

		//! UNIFORM_VEC4I
		Uniform(const UniformName & _name, const Geometry::Vec4i & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Vec4i> & values);

		/**
		 * Create a uniform containing a matrix.
		 * \note The matrix is transposed and stored in the uniform data.
		 */
		//! UNIFORM_MATRIX_3X3F
		Uniform(const UniformName & _name, const Geometry::Matrix3x3 & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Matrix3x3> & values);

		//! UNIFORM_MATRIX_4X4F
		Uniform(const UniformName & _name, const Geometry::Matrix4x4 & value);
		Uniform(const UniformName & _name, const std::vector<Geometry::Matrix4x4> & values);


		std::string toString() const;
		const std::string getName() const 			{	return name.getString();	}
		Util::StringIdentifier getNameId() const 			{	return name.getStringId();	}
		dataType_t getType() const 					{	return type;	}
		const uint8_t * getData() const 			{	return data.data();	}
		size_t getDataSize() const 					{	return data.size();		}

		size_t getNumValues() const 				{	return numValues;	}
		bool operator==(const Uniform & other) const{
			return (isNull() && other.isNull()) ||
				(other.name == name && other.numValues == numValues && other.type == type && other.data==data);
		}

		bool isNull()const							{	return name == Util::StringIdentifier();	}

	private:
		UniformName name;
		dataType_t type;
		size_t numValues;
		std::vector<uint8_t> data;
};
}

#endif // RENDERING_UNIFORM_H
