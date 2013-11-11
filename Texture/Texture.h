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
 ** (0,height)             (width,height)
 **       +---------------+
 **       |      /\       |
 **       |     /  \      |
 **       |      ||       |
 **       |      ||       |
 **       +---------------+
 ** (0,0)                  (width,0)
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
			uint32_t width;
			uint32_t height;
			int32_t border;
			uint32_t glTextureType;
			int32_t glInternalFormat;
			uint32_t glFormat;
			uint32_t glDataType;
			int32_t wrapS, wrapT, wrapR;
			int32_t magFilter, minFilter;
			bool compressed; //!< Determines if the texture is stored in a compressed format.
			uint32_t imageSize; //!< Size of the data in bytes. @see glCompressedTexImage2D
			uint32_t getPixelSize() const;
			uint32_t getDataSize() const {
				if(compressed) {
					return imageSize;
				} else {
					return getPixelSize() * width * height;
				}
			}
			uint32_t getRowSize() const {
				if(compressed) {
					return 0;
				} else {
					return getPixelSize() * width;
				}
			}
		};
		// ---------------------------------------

		Texture(const Format & format);
		~Texture();

		Texture * clone() const;

		uint32_t getGLId() const {
			return glId;
		}
		const Format & getFormat() const {
			return format;
		}
		uint32_t getWidth() const {
			return format.width;
		}
		uint32_t getHeight() const {
			return format.height;
		}
		uint32_t getDataSize() const {
			return format.getDataSize();
		}

		bool createGLID(RenderingContext & context);
		bool uploadGLTexture(RenderingContext & context);
		bool downloadGLTexture(RenderingContext & context);
		void removeGLData();

		void allocateLocalData();
		void removeLocalData();

		/*! Returns a pointer to the local data.
			\note if the texture has no local data, it is downloaded automatically */
		uint8_t * openLocalData(RenderingContext & context);

		uint8_t * getLocalData();
		const uint8_t * getLocalData() const;
		void dataChanged() 							{	dataHasChanged=true;	}

		void _enable(RenderingContext & context);
		void _disable();
		bool isGLTextureValid()const;
		bool isGLTextureResident()const;

		const Util::Reference<Util::Bitmap> & getLocalBitmap() const {
			return localBitmap;
		}

		/*!	@name Filename */
		// @{
		public:
			const Util::FileName & getFileName() const				{	return fileName;	}
			void setFileName(const Util::FileName & f)				{	fileName=f;			}

		private:
			Util::FileName fileName;
		// @}

	/*!	@name Mipmapping */
	// @{
	public:
		//! Generate mipmaps of the texture and set the filtering flags to use these mipmaps.
		bool isMipmappingActive() const;
	// @}

	private:
		uint32_t glId;
		const Format format;
		bool dataHasChanged;

		const uint32_t _pixelDataSize; // initialized automatically

		Util::Reference<Util::Bitmap> localBitmap; //!< local data storage
};


}


#endif // TEXTURE_H
