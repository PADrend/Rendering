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
#ifndef VERTEXACCESSOR_H
#define VERTEXACCESSOR_H

#include "VertexAttribute.h"
#include "MeshVertexData.h"
#include "VertexDescription.h"
#include "VertexAttributeIds.h"


#include <Geometry/Vec3.h>
#include <Geometry/Vec2.h>
#include <Util/Resources/AttributeAccessor.h>
#include <Util/ReferenceCounter.h>
#include <Util/References.h>
#include <Util/Graphics/Color.h>

namespace Rendering {

/** @addtogroup mesh_accessor
 * @{
 */
 
/*! Base class of all VertexAttributeAccessor-classes.
	\note A VertexAttributeAccessor only stays valid as long as the referenced MeshVertexData is not altered externally! */
class VertexAttributeAccessor : public Util::ReferenceCounter<VertexAttributeAccessor>{
		MeshVertexData & vData;
		const VertexAttribute attribute;
		const size_t vertexSize;
		uint8_t * const dataPtr;
	protected:

		VertexAttributeAccessor(MeshVertexData & _vData,VertexAttribute _attribute) :
				ReferenceCounter_t(),vData(_vData),attribute(std::move(_attribute)),
				vertexSize(_vData.getVertexDescription().getVertexSize()),
				dataPtr( vData.data() + attribute.getOffset() ) {}

		void assertRange(uint32_t index)const			{	if(index>=vData.getVertexCount()) throwRangeError(index); }
		RENDERINGAPI void assertNumValues(uint32_t index, uint32_t count) const;
	public:
		virtual ~VertexAttributeAccessor() {}

		bool checkRange(uint32_t index)const			{	return index<vData.getVertexCount();	}
		const VertexAttribute & getAttribute()const		{	return attribute;	}

		template<typename number_t>
		number_t * _ptr(uint32_t index)const			{	return reinterpret_cast<number_t*>(dataPtr+index*vertexSize); }
	private:
		RENDERINGAPI void throwRangeError(uint32_t index)const;		
};


// ---------------------------------
// Color

/*! ColorAttributeAccessor ---|> VertexAttributeAccessor
	Abstract accessor for colors.*/
class ColorAttributeAccessor : public VertexAttributeAccessor{
	protected:
		ColorAttributeAccessor(MeshVertexData & _vData,const VertexAttribute & _attribute) :
				VertexAttributeAccessor(_vData,_attribute){}
	public:
		/*! (static factory)
			Create a ColorAttributeAccessor for the given MeshVertexData's attribute having the given name.
			If no Accessor can be created, an std::invalid_argument exception is thrown. */
		RENDERINGAPI static Util::Reference<ColorAttributeAccessor> create(MeshVertexData & _vData,Util::StringIdentifier name=VertexAttributeIds::COLOR);

		virtual ~ColorAttributeAccessor(){}

		virtual Util::Color4f getColor4f(uint32_t index)const = 0;
		virtual Util::Color4ub getColor4ub(uint32_t index)const = 0;
		virtual void setColor(uint32_t index,const Util::Color4f & c) = 0;
		virtual void setColor(uint32_t index,const Util::Color4ub & c) = 0;
};

// ---------------------------------
// Normals

/*! NormalAttributeAccessor ---|> VertexAttributeAccessor
	Abstract accessor for vertex normals (or tangents etc.)*/
class NormalAttributeAccessor : public VertexAttributeAccessor{
	protected:
		NormalAttributeAccessor(MeshVertexData & _vData,const VertexAttribute & _attribute) :
				VertexAttributeAccessor(_vData,_attribute){}
	public:
		/*! (static factory)
			Create a NormalAttributeAccessor for the given MeshVertexData's attribute having the given name.
			If no Accessor can be created, an std::invalid_argument exception is thrown. */
		RENDERINGAPI static Util::Reference<NormalAttributeAccessor> create(MeshVertexData & _vData,Util::StringIdentifier name=VertexAttributeIds::NORMAL);

		virtual ~NormalAttributeAccessor(){}

		virtual Geometry::Vec3 getNormal(uint32_t index)const = 0;
		virtual void setNormal(uint32_t index,const Geometry::Vec3 & vec) = 0;
};

// ---------------------------------
// Position

/*! PositionAttributeAccessor ---|> VertexAttributeAccessor
	Accessor for float vertex positions.
	\note If someday something else than vec3 is used for storing positions, this has to be implemented using new subclasses! */
class PositionAttributeAccessor : public VertexAttributeAccessor{
	protected:
		PositionAttributeAccessor(MeshVertexData & _vData,const VertexAttribute & _attribute) :
				VertexAttributeAccessor(_vData,_attribute){}
	public:
		/*! (static factory)
			Create a PositionAttributeAccessor for the given MeshVertexData's attribute having the given name.
			If no Accessor can be created, an std::invalid_argument exception is thrown. */
		RENDERINGAPI static Util::Reference<PositionAttributeAccessor> create(MeshVertexData & _vData,Util::StringIdentifier name=VertexAttributeIds::POSITION);

		virtual ~PositionAttributeAccessor(){}

		virtual const Geometry::Vec3 getPosition(uint32_t index) const = 0;
		virtual void setPosition(uint32_t index, const Geometry::Vec3 & p) = 0;
};

// ---------------------------------
// TexCoord

/*! TexCoordAttributeAccessor ---|> VertexAttributeAccessor
	Abstract accessor for texture coordinates.
	\note If someday something else than vec2f is used for storing texture coordinates, this has to be implemented using new subclasses! */
class TexCoordAttributeAccessor : public VertexAttributeAccessor{
	protected:
		TexCoordAttributeAccessor(MeshVertexData & _vData,const VertexAttribute & _attribute) :
				VertexAttributeAccessor(_vData,_attribute){}

	public:
		/*! (static factory)
			Create a TexCoordAttributeAccessor for the given MeshVertexData's attribute having the given name.
			If no Accessor can be created, an std::invalid_argument exception is thrown. */
		RENDERINGAPI static Util::Reference<TexCoordAttributeAccessor> create(MeshVertexData & _vData,Util::StringIdentifier name=VertexAttributeIds::TEXCOORD0);

		virtual ~TexCoordAttributeAccessor(){}

		const Geometry::Vec2 getCoordinate(uint32_t index)const	{
			assertRange(index);
			const float * v=_ptr<const float>(index);
			return Geometry::Vec2(v[0],v[1]);
		}

		void setCoordinate(uint32_t index,const Geometry::Vec2 & p){
			assertRange(index);
			float * v=_ptr<float>(index);
			v[0] = p.x() , v[1] = p.y();
		}
};



// ---------------------------------
// Float

/*! FloatAttributeAccessor ---|> VertexAttributeAccessor
	Accessor for generic float vertex attributes. */
class FloatAttributeAccessor : public VertexAttributeAccessor{
	protected:
		FloatAttributeAccessor(MeshVertexData & _vData,const VertexAttribute & _attribute) :
				VertexAttributeAccessor(_vData,_attribute){}
	public:
		/*! (static factory)
			Create a FloatAttributeAccessor for the given MeshVertexData's attribute having the given name.
			If no Accessor can be created, an std::invalid_argument exception is thrown. */
		RENDERINGAPI static Util::Reference<FloatAttributeAccessor> create(MeshVertexData & _vData, Util::StringIdentifier name);

		virtual ~FloatAttributeAccessor(){}

		virtual float getValue(uint32_t index) const = 0;

		virtual void setValue(uint32_t index, float value) = 0;

		virtual const std::vector<float> getValues(uint32_t index) const = 0;

		virtual void setValues(uint32_t index, const float* values, uint32_t count) = 0;
		inline void setValues(uint32_t index, const std::vector<float>& values) {
			setValues(index, values.data(), static_cast<uint32_t>(values.size()));
		}
};

// ---------------------------------
// Unisigned Integer

/*! UIntAttributeAccessor ---|> VertexAttributeAccessor
	Accessor for generic float vertex attributes. */
class UIntAttributeAccessor : public VertexAttributeAccessor{
	protected:
		UIntAttributeAccessor(MeshVertexData & _vData,const VertexAttribute & _attribute) :
				VertexAttributeAccessor(_vData,_attribute){}
	public:
		/*! (static factory)
			Create a UIntAttributeAccessor for the given MeshVertexData's attribute having the given name.
			If no Accessor can be created, an std::invalid_argument exception is thrown. */
		RENDERINGAPI static Util::Reference<UIntAttributeAccessor> create(MeshVertexData & _vData, Util::StringIdentifier name);
		virtual ~UIntAttributeAccessor(){}

		virtual uint32_t getValue(uint32_t index) const = 0;

		virtual void setValue(uint32_t index, uint32_t value) = 0;

		virtual const std::vector<uint32_t> getValues(uint32_t index) const = 0;
		
		virtual void setValues(uint32_t index, const uint32_t* values, uint32_t count) = 0;
		inline void setValues(uint32_t index, const std::vector<uint32_t>& values) {
			setValues(index, values.data(), static_cast<uint32_t>(values.size()));
		}
};

//! @}
}
#endif // VERTEXACCESSOR_H
