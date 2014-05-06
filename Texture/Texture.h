/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef TEXTURE_H
#define TEXTURE_H

#include <Util/ReferenceCounter.h>
#include <Util/References.h>
#include <Util/IO/FileName.h>
#include <cstdint>

namespace Util {
class Bitmap;
}
namespace Rendering {
class RenderingContext;

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
			uint32_t glTextureType;					//!< GL_TEXTURE_??

			// storage parameters
			int32_t glInternalFormat;				//!< e.g. GL_RGBA8
			uint32_t glFormat;						//!< e.g. GL_RGBA
			bool compressed; 						//!< [=false] Determines if the texture is stored in a compressed format.
			uint32_t compressedImageSize; 			//!< Size of the data in bytes. @see glCompressedTexImage2D

			// sampling parameters
			uint32_t glDataType;					//!< e.g. GL_UNSIGNED_BYTE
			int32_t glWrapS, glWrapT, glWrapR;		//!< e.g. GL_REPEAT
			int32_t glMagFilter, glMinFilter;		//!< e.g. GL_LINEAR

			// memory control
			bool autoCreateMipmaps;					//!< [=true]
			
			uint32_t getPixelSize() const;
			uint32_t getDataSize() const 	{	return compressed ? compressedImageSize : getPixelSize() * sizeX * sizeY * numLayers;}
			uint32_t getRowSize() const		{	return compressed ? 0 : getPixelSize() * sizeX;	}
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
		void createMipMaps(RenderingContext & context);

		//! (internal) uploads the texture if necessary; returns the glId or 0 if the texture is invalid.
		uint32_t _prepareForBinding(RenderingContext & context){
			if(!glId || dataHasChanged)
				_uploadGLTexture(context);
			return glId;
		}

		bool isGLTextureValid()const;
		bool isGLTextureResident()const;

		Util::Bitmap* getLocalBitmap()const					{	return localBitmap.get();	}

		/*!	@name Filename */
		// @{
		public:
			const Util::FileName & getFileName() const		{	return fileName;	}
			void setFileName(const Util::FileName & f)		{	fileName=f;			}

		private:
			Util::FileName fileName;
		// @}

	private:
		uint32_t glId;
		const Format format;
		bool dataHasChanged;
		const uint32_t _pixelDataSize; // initialized automatically

		Util::Reference<Util::Bitmap> localBitmap;
};


}


#endif // TEXTURE_H
