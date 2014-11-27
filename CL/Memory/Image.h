/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_IMAGE_H_
#define RENDERING_CL_IMAGE_H_

#include "Memory.h"

#include <Rendering/Texture/TextureType.h>

namespace Util {
class Bitmap;
}

namespace Rendering {
class Texture;
namespace CL {
class Context;
class Buffer;

enum class ImageType : std::uint8_t {
	IMAGE_1D = 0,
	IMAGE_1D_ARRAY = 1,
	IMAGE_2D = 2,
	IMAGE_2D_ARRAY = 3,
	IMAGE_3D = 4,
	IMAGE_1D_BUFFER = 5,
	IMAGE_GL = 6,
};

enum class ChannelOrder : std::uint8_t {
	A,
	R,
	RG,
	RA,
	RGB,
	RGBA,
	BGRA,
	ARGB,
	INTENSITY,
	LUMINANCE,
	Rx,
	RGx,
	RGBx,
};

enum class ChannelType : std::uint8_t {
	SNORM_INT8,
	SNORM_INT16,
	UNORM_INT8,
	UNORM_INT16,
	UNORM_SHORT_565,
	UNORM_SHORT_555,
	UNORM_INT_101010,
	SIGNED_INT8,
	SIGNED_INT16,
	SIGNED_INT32,
	UNSIGNED_INT8,
	UNSIGNED_INT16,
	UNSIGNED_INT32,
	HALF_FLOAT,
	FLOAT,
};

struct PixelFormatCL {
	ChannelOrder channelOrder;	//!< Specifies the number of channels and the channel layout i.e. the memory layout in which channels are stored in the image.
	ChannelType channelType; 	//!< Describes the size of the channel data type.
};

class Image : public Memory {
public:
	struct Format {
		Format();
		size_t width, height, numLayers; //!< width, height, depth (3d-texture)/num Layers(array texture)
		ImageType type; 			 //!< The image type (TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, ...)

		PixelFormatCL pixelFormat;

		/**
		 * The scan-line pitch in bytes.
		 * This must be 0 if host_ptr is NULL and can be either 0 or ≥ image_width * size of element in bytes if host_ptr is not NULL.
		 * If host_ptr is not NULL and image_row_pitch = 0, image_row_pitch is calculated as image_width * size of element in bytes.
		 * If image_row_pitch is not 0, it must be a multiple of the image element size in bytes.
		 */
		uint32_t rowPitch;
		/**
		 * The size in bytes of each 2D slice in the 3D image or the size in bytes of each image in a 1D or 2D image array.
		 * This must be 0 if host_ptr is NULL.
		 * If host_ptr is not NULL, image_slice_pitch can be either 0 or ≥ image_row_pitch * image_height for a 2D image array
		 * or 3D image and can be either 0 or ≥ image_row_pitch for a 1D image array. If host_ptr is not NULL and image_slice_pitch = 0,
		 * image_slice_pitch is calculated as image_row_pitch * image_height for a 2D image array or 3D image and image_row_pitch for a 1D image array.
		 * If image_slice_pitch is not 0, it must be a multiple of the image_row_pitch.
		 */
		uint32_t slicePitch;
	};

	Image(Context* context, Format format, ReadWrite_t readWrite, HostPtr_t hostPtrUsage = HostPtr_t::None, void* hostPtr = nullptr, ReadWrite_t hostReadWrite = ReadWrite_t::ReadWrite);
	Image(Context* context, Format format, ReadWrite_t readWrite, Buffer* buffer);
	Image(Context* context, ReadWrite_t readWrite, TextureType target, uint32_t glHandle, uint32_t mipLevel = 0);
	Image(Context* context, ReadWrite_t readWrite, Texture* texture, uint32_t mipLevel = 0);
	Image(Context* context, ReadWrite_t readWrite, Util::Bitmap* bitmap, HostPtr_t hostPtrUsage = HostPtr_t::Use, ReadWrite_t hostReadWrite = ReadWrite_t::ReadWrite);
	Image(const Image& image);

	PixelFormatCL getPixelFormat() const;
	size_t getElementSize() const;
	size_t getRowPitch() const;
	size_t getSlicePitch() const;
	size_t getWidth() const;
	size_t getHeight() const;
	size_t getDepth() const;
	uint32_t getNumMipLevels() const;
	uint32_t getNumSamples() const;
	uint32_t getGLTextureTarget() const;
	uint32_t getMipmapLevel() const;
	ImageType getType() const { return type; };
private:
	ImageType type;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_IMAGE_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
