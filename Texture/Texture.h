/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef TEXTURE_H
#define TEXTURE_H

#include "TextureType.h"
#include "PixelFormatGL.h"
#include "../Core/Common.h"
#include "../RenderingContext/RenderingParameters.h"

#include <Util/Graphics/Color.h>
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
class Device;
using DeviceRef = Util::Reference<Device>;
class ImageStorage;
using ImageStorageRef = Util::Reference<ImageStorage>;
class ImageView;
using ImageViewRef = Util::Reference<ImageView>;
class Sampler;
using SamplerRef = Util::Reference<Sampler>;

//! @addtogroup rendering_resources
//! @{
//! @defgroup texture Texture
//! @}
 
/**
	Texture

	Coordinates:
	@verbatim

	(0,sizeY-1,numLayers-1)             (sizeX,sizeY,numLayers-1)
								+---------------+  
								|               |  
								.     ...       .
								.               .
								|               |  
	(0,0,1)       +---------------+ (sizeX-1,0,1)
	(0,sizeY-1,0) +---------------+ (sizeX-1,sizeY-1,0)
								|      /\       |  
								|     /  \      |
								|      ||       |
								|      ||       |
								+---------------+
	(0,0,0)                  (sizeX,0,0)

	@endverbatim
	\note the coordinates are different to the ones used in Util::Bitmap
	@ingroup texture
*/
class Texture : public Util::ReferenceCounter<Texture>	{
public:
	using Ref = Util::Reference<Texture>;
	using Format = ImageFormat;
	
	static Ref create(const DeviceRef& device, const Format& format, const SamplerRef& sampler = nullptr);
	static Ref create(const DeviceRef& device, const ImageStorageRef& image, const SamplerRef& sampler = nullptr);
	static Ref create(const DeviceRef& device, const ImageViewRef& image, const SamplerRef& sampler = nullptr);

	~Texture();

	uint32_t getDataSize() const { return 0; }
	const Format & getFormat() const { return format; }
	uint32_t getNumLayers() const { return format.layers > 1 ? format.layers : format.extent.z(); }
	uint32_t getHeight() const { return format.extent.y(); }
	uint32_t getWidth() const { return format.extent.x(); }
	TextureType getTextureType() const { return tType; }
	bool isValid() const { return imageView; }

	/*!	@name Image data manipulation */
	// @{

	void allocateLocalData();

	/*! Returns a pointer to the local data.
		\note if the texture has no local data, it is downloaded automatically */
	uint8_t * openLocalData(RenderingContext & context);

	uint8_t * getLocalData();
	const uint8_t * getLocalData() const;
	
	void dataChanged() { dataHasChanged = true; }

	Util::Bitmap* getLocalBitmap() const { return localBitmap.get(); }
	
	void upload();
	// @}

	/*!	@name Mipmaps */
	// @{
	void planMipmapCreation() { mipmapCreationIsPlanned = true; }
	void createMipmaps(RenderingContext & context);
	bool getHasMipmaps() const { return hasMipmaps; }
	// @}
	
	/*!	@name Filename */
	// @{
	const Util::FileName & getFileName() const { return fileName; }
	void setFileName(const Util::FileName & f) { fileName=f; }
	// @}
	
	/*!	@name Internal */
	// @{
	const ImageViewRef& getImageView() const { return imageView; }
	const ImageStorageRef& getImage() const;
	const SamplerRef& getSampler() const { return sampler; }
	ResourceUsage getLastUsage() const;
	// @}
	
	/*!	@name Deprecated */
	// @{
	[[deprecated]]
	Texture(Format format);
	[[deprecated]]
	bool getUseLinearMinFilter() const;
	[[deprecated]]
	bool getUseLinearMagFilter() const;
	[[deprecated]]
	uint32_t _prepareForBinding(RenderingContext & context);
	[[deprecated]]
	bool isGLTextureValid()const { return isValid(); }
	[[deprecated]]
	bool isGLTextureResident()const { return isValid(); }
	[[deprecated]]
	uint32_t getGLTextureType() const { return 0; }
	[[deprecated]]
	uint32_t getGLId() const { return glId; }
	[[deprecated]]
	void _createGLID(RenderingContext & context);
	[[deprecated]]
	void _uploadGLTexture(RenderingContext & context, int level=0);
	[[deprecated]]
	void downloadGLTexture(RenderingContext & context);
	[[deprecated]]
	void removeGLData();
	[[deprecated]]
	void clearGLData(const Util::Color4f& color={});
	[[deprecated]]
	void _setGLId(uint32_t glId);
	[[deprecated]]
	void enableComparision(RenderingContext & context, Comparison::function_t func);
	[[deprecated]]
	BufferObject* getBufferObject() const { return nullptr; }
	// @}
private:
	Texture(const DeviceRef& device, const Format& format, const SamplerRef& sampler);

	DeviceRef device;
	const Format format;

	TextureType tType;
	uint32_t glId = 0;
	bool dataHasChanged = true;
	bool hasMipmaps = false;
	bool mipmapCreationIsPlanned = false;

	Util::FileName fileName;
	Util::Reference<Util::Bitmap> localBitmap;
	ImageViewRef imageView;
	SamplerRef sampler;
};


}


#endif // TEXTURE_H
