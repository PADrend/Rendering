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
		
		/*! A Texture's type. (Corresponds to 'glTextureType', but the actual value is independent from OpenGL.
			\note Value assignment must never change! (they may be used for serialization)
		*/
		enum class TextureType : std::uint8_t{
			TEXTURE_1D = 0,
			TEXTURE_1D_ARRAY = 1,
			TEXTURE_2D = 2,
			TEXTURE_2D_ARRAY = 3,
			TEXTURE_3D = 4,
			TEXTURE_CUBE_MAP = 5,
			TEXTURE_CUBE_MAP_ARRAY = 6
		};
		
		/***
		 ** Texture::Format
		 **/
		struct Format {
			Format();
			uint32_t sizeX, sizeY, numLayers;		//!< width, height, depth (3d-texture)/num Layers(array texture)
			uint32_t glTextureType;					//!< GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_

			// storage parameters
			int32_t glInternalFormat;				//!< e.g. GL_RGBA8
			uint32_t glFormat;						//!< e.g. GL_RGBA
			bool compressed; 						//!< [=false] Determines if the texture is stored in a compressed format.
			uint32_t compressedImageSize; 			//!< Size of the data in bytes. @see glCompressedTexImage2D

			// sampling parameters
			uint32_t glDataType;					//!< e.g. GL_UNSIGNED_BYTE
			int32_t glWrapS, glWrapT, glWrapR;		//!< e.g. GL_REPEAT

		
			bool linearMinFilter,linearMagFilter;	//! true, true
			
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
