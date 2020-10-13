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
typedef _Vec2<uint32_t> Vec2ui;

template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3;
typedef _Vec3<float> Vec3f;
typedef _Vec3<int32_t> Vec3i;
typedef _Vec3<uint32_t> Vec3ui;

template<typename _T> class _Vec4;
typedef _Vec4<float> Vec4;
typedef _Vec4<float> Vec4f;
typedef _Vec4<int32_t> Vec4i;
typedef _Vec4<uint32_t> Vec4ui;

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

/**
 * Uniform
 * @ingroup shader
 */
class Uniform {
		//! dataType_t ---> bytes per value
		static const size_t dataSizeIndex[];

	public:
		//! \note if something is changed here, make sure that the dataSize-index is also updated.
		enum dataType_t {
			UNIFORM_BOOL = 0, 		UNIFORM_VEC2B = 1,	UNIFORM_VEC3B = 2,	UNIFORM_VEC4B = 3,
			UNIFORM_FLOAT = 4, 		UNIFORM_VEC2F = 5,	UNIFORM_VEC3F = 6,	UNIFORM_VEC4F = 7,
			UNIFORM_INT = 8, 		UNIFORM_VEC2I = 9,	UNIFORM_VEC3I = 10,	UNIFORM_VEC4I = 11,
			UNIFORM_MATRIX_2X2F = 12,	UNIFORM_MATRIX_3X3F = 13, UNIFORM_MATRIX_4X4F = 14,
			UNIFORM_UINT = 15, 	UNIFORM_VEC2UI = 16,		UNIFORM_VEC3UI = 17,		UNIFORM_VEC4UI = 18,
		};

		//! returns the size in bytes of a value of the given type
		static size_t getValueSize(const dataType_t t){		return dataSizeIndex[t];	}
		static std::string getTypeString(const dataType_t t);

		class UniformName{
				Util::StringIdentifier id;
		public:
				UniformName() : id(0) {}
				UniformName(const char * s) : id(Util::StringIdentifier(s)) {}
				UniformName(const std::string & s) : id(Util::StringIdentifier(s)) {}
				UniformName(Util::StringIdentifier _id) : id(std::move(_id)) {}

				const std::string getString()const				{	return id.toString();	}
				Util::StringIdentifier getStringId()const				{	return id;	}
				bool operator==(const UniformName & other)const	{	return id==other.id;	}
		};
		static const Uniform nullUniform;

		Uniform();
		Uniform(UniformName _name, dataType_t _type, size_t arraySize);
		Uniform(UniformName _name, dataType_t _type, size_t arraySize,std::vector<uint8_t> data);

		/*! Generic bool-constructor (use another contructor whenever possible)
			\throw may throw an invalid_argument-exception	*/
		Uniform(UniformName _name, dataType_t _type, const std::deque<bool> & values);
		/*! Generic float-constructor (use another contructor whenever possible)
			\throw may throw an invalid_argument-exception	*/
		Uniform(UniformName _name, dataType_t _type, const std::vector<float> & values);
		/*! Generic int-constructor (use another contructor whenever possible)
			\throw may throw an invalid_argument-exception	*/
		Uniform(UniformName _name, dataType_t _type, const std::vector<int32_t> & values);
		/*! Generic uint-constructor (use another contructor whenever possible)
			\throw may throw an invalid_argument-exception	*/
		Uniform(UniformName _name, dataType_t _type, const std::vector<uint32_t> & values);

		//! UNIFORM_BOOL
		Uniform(UniformName _name, bool value);
		Uniform(UniformName _name, const std::deque<bool> & values);

		//! UNIFORM_FLOAT
		Uniform(UniformName _name, float value);
		Uniform(UniformName _name, const std::vector<float> & values);

		//! UNIFORM_VEC2F
		Uniform(UniformName _name, const Geometry::Vec2 & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec2> & values);

		//! UNIFORM_VEC3F
		Uniform(UniformName _name, const Geometry::Vec3 & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec3> & values);

		//! UNIFORM_VEC4F
		Uniform(UniformName _name, const Geometry::Vec4 & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec4> & values);
		Uniform(UniformName _name, const Util::Color4f & color);

		//! UNIFORM_INT
		Uniform(UniformName _name, int32_t value);
		Uniform(UniformName _name, const std::vector<int32_t> & values);

		//! UNIFORM_VEC2I
		Uniform(UniformName _name, const Geometry::Vec2i & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec2i> & values);

		//! UNIFORM_VEC3I
		Uniform(UniformName _name, const Geometry::Vec3i & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec3i> & values);

		//! UNIFORM_VEC4I
		Uniform(UniformName _name, const Geometry::Vec4i & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec4i> & values);

		//! UNIFORM_UINT
		Uniform(UniformName _name, uint32_t value);
		Uniform(UniformName _name, const std::vector<uint32_t> & values);

		//! UNIFORM_VEC2UI
		Uniform(UniformName _name, const Geometry::Vec2ui & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec2ui> & values);

		//! UNIFORM_VEC3UI
		Uniform(UniformName _name, const Geometry::Vec3ui & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec3ui> & values);

		//! UNIFORM_VEC4UI
		Uniform(UniformName _name, const Geometry::Vec4ui & value);
		Uniform(UniformName _name, const std::vector<Geometry::Vec4ui> & values);

		/**
		 * Create a uniform containing a matrix.
		 * \note The matrix is transposed and stored in the uniform data.
		 */
		//! UNIFORM_MATRIX_3X3F
		Uniform(UniformName _name, const Geometry::Matrix3x3 & value);
		Uniform(UniformName _name, const std::vector<Geometry::Matrix3x3> & values);

		//! UNIFORM_MATRIX_4X4F
		Uniform(UniformName _name, const Geometry::Matrix4x4 & value);
		Uniform(UniformName _name, const std::vector<Geometry::Matrix4x4> & values);


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
