/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STREAMERMMF_H_
#define RENDERING_STREAMERMMF_H_

#include "AbstractRenderingStreamer.h"
#include <cstdint>

namespace Rendering {
namespace Serialization {

/**

	.mmf (MinSG Mesh Format)
	========================

	Fileformat: binary little endian

	MMF-File ::=    Header (char[4] "mmf"+chr(13) ),
					uint32 version (currently 0x01),
					DataBlock * (one VertexBlock and one IndexBlock),
					EndMarker (uint32 0xFFFFFFFF)

	DataBlock ::=   uint32 dataType,
					uint32 dataSize -- nr of bytes to be jumped to skip the block (not including dataType and blockSize value),
					uint8 data[dataSize]

	DataBlock ::=   VertexBlock

	DataBlock ::=   IndexBlock

	VertexBlock ::= Vertex-dataType (uint32 0x00),
					uint32 dataSize,
					VertexAttributeDescription *,
					EndMarker (uint32 0xFFFFFFFF),
					uint32 vertexCount -- the number of vertices in the following datablock,
					uint8* vertexData -- the vertex data

	VertexAttributeDescription ::=
					uint32 attrId -- one of the constants:
						0x00:POSITION	(attribute name: "sg_Position")
						0x01:NORMAL		(attribute name: "sg_Normal")
						0x02:COLOR		(attribute name: "sg_Color")
						0x06:TEX0		(attribute name: "sg_TexCoord0")
						0x07:TEX1		(attribute name: "sg_TexCoord1")
						0xff:custom attribute MMF_CUSTOM_ATTR_ID
					uint32 numValues -- entrysize of the vertexattribute specified by index,
					uint32 (=GLuint) type -- type of the vertexattribute specified by index,
					uint32 extLength -- length of the extension for future use,
					VertexAttributeExtension* (extensions for the vertex attribute)

	VertexAttributeExtension ::=
					uint32 extension Type
					uint32 dataLength
					uint8 data[dataLength]

	VertexAttributeExtension ::=
					VertexAttributeNameExtension

	VertexAttributeNameExtension ::=
					uint32 extension Type 0x03 ( MMF_VERTEX_ATTR_EXT_NAME )
					uint32 length of name string including padding zeros
					uint8* attrName (filled up with additional zeros until 32bit alignment is reached.


	IndexBlock ::=  Index-dataType (uint32 0x01),
					uint32 dataSize,
					uint32 indexCount -- the number of indices in the following datablock,
					uint32 (=GLuint) indexMode -- the meaning of the indices (GL_TRIANGLES, GL_TRIANGLE_STRIP, ...),
					uint8* indexData -- the index data
*/
class StreamerMMF : public AbstractRenderingStreamer {
	public:
		const static uint32_t MMF_VERSION = 0x01;
		const static uint32_t MMF_HEADER = 0x0d666d6d; // = "mmf "

		const static uint32_t MMF_VERTEX_DATA = 0x00;
		const static uint32_t MMF_INDEX_DATA = 0x01;
		const static uint32_t MMF_END = 0xFFFFFFFF;

		const static uint32_t MMF_CUSTOM_ATTR_ID = 0xFF;
		const static uint32_t MMF_VERTEX_ATTR_EXT_NAME = 0x03;

		StreamerMMF() :
			AbstractRenderingStreamer() {
		}
		virtual ~StreamerMMF() {
		}

		Util::GenericAttributeList * loadGeneric(std::istream & input) override;
		Mesh * loadMesh(std::istream & input) override;
		bool saveMesh(Mesh * mesh, std::ostream & output) override;

		static uint8_t queryCapabilities(const std::string & extension);
		static const char * const fileExtension;

	private:
		struct Reader{
			Reader(std::istream & _in) : in(_in){}
			std::istream & in;
			uint32_t read_uint32();
			void read(uint8_t * data,size_t count);
			void skip(uint32_t size);

		};
		static void readVertexData(Mesh * mesh, Reader & in);
		static void readIndexData(Mesh * mesh, Reader & in);


		static void write(std::ostream & out, uint32_t x);
};

}
}

#endif /* RENDERING_STREAMERMMF_H_ */
