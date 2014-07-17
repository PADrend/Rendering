/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef TEXTURE_H
#define TEXTURE_H

#include "TextureType.h"
#include "PixelFormatGL.h"

#include <Util/ReferenceCounter.h>
#include <Util/References.h>
#include <Util/IO/FileName.h>
#include <cstdint>
#include <memory>

namespace Util {
class Bitmap;
}
namespace Rendering {
class RenderingContext;
class BufferObject;

/***
 ** Texture
 **
 **	Coordinates:
 **
 ** (0,sizeY-1,numLayers-1)             (sizeX,sizeY,numLayers-1)
 **               +---------------+  
 **               |               |  
 **               .     ...       .
 **               .               .
 **               |               |  
 ** (0,0,1)       +---------------+ (sizeX-1,0,1)
 ** (0,sizeY-1,0) +---------------+ (sizeX-1,sizeY-1,0)
 **               |      /\       |  
 **               |     /  \      |
 **               |      ||       |
 **               |      ||       |
 **               +---------------+
 ** (0,0,0)                  (sizeX,0,0)
 **
 ** \note the coordinates are different to the ones used in Util::Bitmap
 **/
class Texture: public Util::ReferenceCounter<Texture>	{
	public:
		
		/***
		 ** Texture::Format
		 **/
		struct Format {
			Format();
			uint32_t sizeX, sizeY, numLayers;		//!< width, height, depth (3d-texture)/num Layers(array texture)
			uint32_t glTextureType;					//!< GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_

			uint32_t compressedImageSize; 			//!< Size of the data in bytes. @see glCompressedTexImage2D
			int32_t glWrapS, glWrapT, glWrapR;		//!< e.g. GL_REPEAT
			PixelFormatGL pixelFormat;
		
			bool linearMinFilter,linearMagFilter;	//! true, true
			
			uint32_t getPixelSize() const;
			uint32_t getDataSize() const 	{	return pixelFormat.compressed ? compressedImageSize : getPixelSize() * sizeX * sizeY * numLayers;}
			uint32_t getRowSize() const		{	return pixelFormat.compressed ? 0 : getPixelSize() * sizeX;	}
		};
		// ---------------------------------------

		Texture(Format format);
		~Texture();

		uint32_t getDataSize() const						{	return format.getDataSize();	}
		const Format & getFormat() const					{	return format;	}
		uint32_t getGLTextureType() const					{	return format.glTextureType;	}
		uint32_t getGLId() const							{	return glId;	}
		uint32_t getNumLayers() const						{	return format.numLayers;	}
		uint32_t getHeight() const							{	return format.sizeY;	}
		uint32_t getWidth() const							{	return format.sizeX;	}
		TextureType getTextureType() const					{	return tType;	}
		bool getUseLinearMinFilter() const					{	return format.linearMinFilter;	}
		bool getUseLinearMagFilter() const					{	return format.linearMagFilter;	}

		void _createGLID(RenderingContext & context);
		void _uploadGLTexture(RenderingContext & context);
		void downloadGLTexture(RenderingContext & context);
		void removeGLData();

		void allocateLocalData();

		/*! Returns a pointer to the local data.
			\note if the texture has no local data, it is downloaded automatically */
		uint8_t * openLocalData(RenderingContext & context);

		uint8_t * getLocalData();
		const uint8_t * getLocalData() const;
		
		void dataChanged()									{	dataHasChanged = true;	}

		//! (internal) uploads the texture if necessary; returns the glId or 0 if the texture is invalid.
		uint32_t _prepareForBinding(RenderingContext & context){
			if(!glId || dataHasChanged)
				_uploadGLTexture(context);
			if(mipmapCreationIsPlanned)
				createMipmaps(context);
			return glId;
		}

		bool isGLTextureValid()const;
		bool isGLTextureResident()const;

		Util::Bitmap* getLocalBitmap()const					{	return localBitmap.get();	}

	/*!	@name Mipmaps */
	// @{
		void planMipmapCreation()							{	mipmapCreationIsPlanned = true;	}
		void createMipmaps(RenderingContext & context);
		bool getHasMipmaps() const							{	return hasMipmaps;	}
	// @}
		
			
	/*!	@name BufferObject (tType == TEXTURE_BUFFER)  */
	// @{
		public:
			BufferObject* getBufferObject()const			{	return bufferObject.get();	}
		private:
			std::unique_ptr<BufferObject> bufferObject;		// if type is bufferObject
	// @}

	/*!	@name Filename */
	// @{
		public:
			const Util::FileName & getFileName() const		{	return fileName;	}
			void setFileName(const Util::FileName & f)		{	fileName=f;			}

		private:
			Util::FileName fileName;
	// @}

	private:
		TextureType tType;
		uint32_t glId;
		const Format format;
		bool dataHasChanged;
		bool hasMipmaps;
		bool mipmapCreationIsPlanned;
		const uint32_t _pixelDataSize; // initialized automatically

		Util::Reference<Util::Bitmap> localBitmap;
};


}


#endif // TEXTURE_H
