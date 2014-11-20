/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Image.h"

#include "Buffer.h"
#include "../CLUtils.h"
#include "../Context.h"

#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/GLHeader.h>

#include <Util/Macros.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelFormat.h>
#include <Util/TypeConstant.h>

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.hpp>

#define WARN_AND_FAIL(msg) WARN(msg); FAIL();

namespace Rendering {
namespace CL {

cl::ImageFormat pixelFormatToImageFormat(PixelFormatCL pixelFormat) {
	cl::ImageFormat format;
	switch(pixelFormat.channelOrder) {
	case ChannelOrder::A: 			format.image_channel_order = CL_A; 			break;
	case ChannelOrder::R: 			format.image_channel_order = CL_R; 			break;
	case ChannelOrder::RG: 			format.image_channel_order = CL_RG; 		break;
	case ChannelOrder::RA: 			format.image_channel_order = CL_RA; 		break;
	case ChannelOrder::RGB: 		format.image_channel_order = CL_RGB; 		break;
	case ChannelOrder::RGBA: 		format.image_channel_order = CL_RGBA; 		break;
	case ChannelOrder::BGRA: 		format.image_channel_order = CL_BGRA; 		break;
	case ChannelOrder::ARGB:		format.image_channel_order = CL_ARGB; 		break;
	case ChannelOrder::INTENSITY:	format.image_channel_order = CL_INTENSITY;	break;
	case ChannelOrder::LUMINANCE:	format.image_channel_order = CL_LUMINANCE;	break;
	case ChannelOrder::Rx: 			format.image_channel_order = CL_Rx;			break;
	case ChannelOrder::RGx: 		format.image_channel_order = CL_RGx; 		break;
	case ChannelOrder::RGBx: 		format.image_channel_order = CL_RGBx; 		break;
	}
	switch(pixelFormat.channelType) {
	case ChannelType::SNORM_INT8:		format.image_channel_data_type = CL_SNORM_INT8; 		break;
	case ChannelType::SNORM_INT16:		format.image_channel_data_type = CL_SNORM_INT16; 		break;
	case ChannelType::UNORM_INT8:		format.image_channel_data_type = CL_UNORM_INT8; 		break;
	case ChannelType::UNORM_INT16:		format.image_channel_data_type = CL_UNORM_INT16; 		break;
	case ChannelType::UNORM_SHORT_565:	format.image_channel_data_type = CL_UNORM_SHORT_565; 	break;
	case ChannelType::UNORM_SHORT_555:	format.image_channel_data_type = CL_UNORM_SHORT_555;	break;
	case ChannelType::UNORM_INT_101010:	format.image_channel_data_type = CL_UNORM_INT_101010;	break;
	case ChannelType::SIGNED_INT8:		format.image_channel_data_type = CL_SIGNED_INT8; 		break;
	case ChannelType::SIGNED_INT16:		format.image_channel_data_type = CL_SIGNED_INT16; 		break;
	case ChannelType::SIGNED_INT32:		format.image_channel_data_type = CL_SIGNED_INT32; 		break;
	case ChannelType::UNSIGNED_INT8:	format.image_channel_data_type = CL_UNSIGNED_INT8; 		break;
	case ChannelType::UNSIGNED_INT16:	format.image_channel_data_type = CL_UNSIGNED_INT16; 	break;
	case ChannelType::UNSIGNED_INT32:	format.image_channel_data_type = CL_UNSIGNED_INT32; 	break;
	case ChannelType::HALF_FLOAT:		format.image_channel_data_type = CL_HALF_FLOAT; 		break;
	case ChannelType::FLOAT:			format.image_channel_data_type = CL_FLOAT; 				break;
	}
	return format;
}

PixelFormatCL imageFormatToPixelFormat(cl_image_format imageFormat) {
	PixelFormatCL format;
	switch(imageFormat.image_channel_order) {
	case CL_A: 			format.channelOrder = ChannelOrder::A; 			break;
	case CL_R: 			format.channelOrder = ChannelOrder::R; 			break;
	case CL_RG: 		format.channelOrder = ChannelOrder::RG; 		break;
	case CL_RA: 		format.channelOrder = ChannelOrder::RA; 		break;
	case CL_RGB: 		format.channelOrder = ChannelOrder::RGB; 		break;
	case CL_RGBA: 		format.channelOrder = ChannelOrder::RGBA; 		break;
	case CL_BGRA: 		format.channelOrder = ChannelOrder::BGRA; 		break;
	case CL_ARGB:		format.channelOrder = ChannelOrder::ARGB; 		break;
	case CL_INTENSITY:	format.channelOrder = ChannelOrder::INTENSITY;	break;
	case CL_LUMINANCE:	format.channelOrder = ChannelOrder::LUMINANCE;	break;
	case CL_Rx: 		format.channelOrder = ChannelOrder::Rx;			break;
	case CL_RGx: 		format.channelOrder = ChannelOrder::RGx; 		break;
	case CL_RGBx: 		format.channelOrder = ChannelOrder::RGBx; 		break;
	}
	switch(imageFormat.image_channel_data_type) {
	case CL_SNORM_INT8:			format.channelType = ChannelType::SNORM_INT8; 		break;
	case CL_SNORM_INT16:		format.channelType = ChannelType::SNORM_INT16; 		break;
	case CL_UNORM_INT8:			format.channelType = ChannelType::UNORM_INT8; 		break;
	case CL_UNORM_INT16:		format.channelType = ChannelType::UNORM_INT16; 		break;
	case CL_UNORM_SHORT_565:	format.channelType = ChannelType::UNORM_SHORT_565; 	break;
	case CL_UNORM_SHORT_555:	format.channelType = ChannelType::UNORM_SHORT_555;	break;
	case CL_UNORM_INT_101010:	format.channelType = ChannelType::UNORM_INT_101010;	break;
	case CL_SIGNED_INT8:		format.channelType = ChannelType::SIGNED_INT8; 		break;
	case CL_SIGNED_INT16:		format.channelType = ChannelType::SIGNED_INT16; 	break;
	case CL_SIGNED_INT32:		format.channelType = ChannelType::SIGNED_INT32; 	break;
	case CL_UNSIGNED_INT8:		format.channelType = ChannelType::UNSIGNED_INT8; 	break;
	case CL_UNSIGNED_INT16:		format.channelType = ChannelType::UNSIGNED_INT16; 	break;
	case CL_UNSIGNED_INT32:		format.channelType = ChannelType::UNSIGNED_INT32; 	break;
	case CL_HALF_FLOAT:			format.channelType = ChannelType::HALF_FLOAT; 		break;
	case CL_FLOAT:				format.channelType = ChannelType::FLOAT; 			break;
	}
	return format;
}

cl::ImageFormat bitmapFormatToImageFormat(Util::PixelFormat pixelFormat) {
	using namespace Util;
	cl::ImageFormat format{0,0};
	switch(pixelFormat.getNumComponents()) {
	case 1:
		if(pixelFormat.getByteOffset_a() != PixelFormat::NONE)
			format.image_channel_order = CL_A;
		else if(pixelFormat.getByteOffset_r() != PixelFormat::NONE)
			format.image_channel_order = CL_R;
		break;
	case 2:
		if(pixelFormat.getByteOffset_r() < pixelFormat.getByteOffset_a() && pixelFormat.getByteOffset_a() != PixelFormat::NONE)
			format.image_channel_order = CL_RA;
		else if(pixelFormat.getByteOffset_r() < pixelFormat.getByteOffset_g() && pixelFormat.getByteOffset_g() != PixelFormat::NONE)
			format.image_channel_order = CL_RG;
		break;
	case 3:
		if(pixelFormat.getByteOffset_r() < pixelFormat.getByteOffset_g()
				&& pixelFormat.getByteOffset_g() < pixelFormat.getByteOffset_b()
				&& pixelFormat.getByteOffset_b() != PixelFormat::NONE)
			format.image_channel_order = CL_RGB;
		break;
	case 4:
		if(pixelFormat.getByteOffset_r() < pixelFormat.getByteOffset_g()
				&& pixelFormat.getByteOffset_g() < pixelFormat.getByteOffset_b()
				&& pixelFormat.getByteOffset_b() < pixelFormat.getByteOffset_a())
			format.image_channel_order = CL_RGBA;
		else if(pixelFormat.getByteOffset_a() < pixelFormat.getByteOffset_r()
				&& pixelFormat.getByteOffset_r() < pixelFormat.getByteOffset_g()
				&& pixelFormat.getByteOffset_g() < pixelFormat.getByteOffset_b())
			format.image_channel_order = CL_ARGB;
		else if(pixelFormat.getByteOffset_b() < pixelFormat.getByteOffset_g()
				&& pixelFormat.getByteOffset_g() < pixelFormat.getByteOffset_r()
				&& pixelFormat.getByteOffset_r() < pixelFormat.getByteOffset_a())
			format.image_channel_order = CL_BGRA;
		break;
	}

	switch(pixelFormat.getValueType()) {
	case TypeConstant::UINT8:  	format.image_channel_data_type = CL_UNORM_INT8; 		break;
	case TypeConstant::UINT16:	format.image_channel_data_type = CL_UNORM_INT16; 		break;
	case TypeConstant::UINT32:	format.image_channel_data_type = CL_UNSIGNED_INT32; 	break;
	case TypeConstant::INT8:	format.image_channel_data_type = CL_SNORM_INT8; 		break;
	case TypeConstant::INT16:	format.image_channel_data_type = CL_SNORM_INT16; 		break;
	case TypeConstant::INT32:  	format.image_channel_data_type = CL_SIGNED_INT32; 		break;
	case TypeConstant::FLOAT:	format.image_channel_data_type = CL_FLOAT; 				break;
	case TypeConstant::UINT64:	// unsupported
	case TypeConstant::INT64:	// unsupported
	case TypeConstant::DOUBLE:	// unsupported
		break;
	}

	return format;
}

cl::Image* createImageBuffer(Context* context, Image::Format format, Memory::ReadWrite_t readWrite, Memory::HostPtr_t hostPtrUsage, void* hostPtr, Memory::ReadWrite_t hostReadWrite) {
	cl_mem_flags flags = convertToCLFlags(readWrite, hostPtrUsage, hostReadWrite);
	cl::ImageFormat cl_format = pixelFormatToImageFormat(format.pixelFormat);
	cl_int err;
	cl::Image* image = nullptr;

	switch (format.type) {
		case ImageType::IMAGE_1D:
			image = new cl::Image1D(*context->_internal(), flags, cl_format, format.width, hostPtr, &err);
			break;
		case ImageType::IMAGE_1D_ARRAY:
			image = new cl::Image1DArray(*context->_internal(), flags, cl_format, format.numLayers, format.width, format.rowPitch, hostPtr, &err);
			break;
		case ImageType::IMAGE_2D:
			image = new cl::Image2D(*context->_internal(), flags, cl_format, format.width, format.height, format.rowPitch, hostPtr, &err);
			break;
		case ImageType::IMAGE_2D_ARRAY:
			image = new cl::Image2DArray(*context->_internal(), flags, cl_format, format.numLayers, format.width, format.height, format.rowPitch, format.slicePitch, hostPtr, &err);
			break;
		case ImageType::IMAGE_3D:
			image = new cl::Image3D(*context->_internal(), flags, cl_format,format.width, format.height, format.numLayers, format.rowPitch, format.slicePitch, hostPtr, &err);
			break;
		default:
			WARN("Could not create image (Unsupported image format).");
			FAIL();
	}

	if(err != CL_SUCCESS) {
		WARN("Could not create image (" + getErrorString(err) + ").");
		FAIL();
	}
	return image;
}

cl::Image* createImageBufferFromBitmap(Context* context, Memory::ReadWrite_t readWrite, Util::Bitmap* bitmap, Memory::HostPtr_t hostPtrUsage, Memory::ReadWrite_t hostReadWrite) {
	cl_mem_flags flags = convertToCLFlags(readWrite, hostPtrUsage, hostReadWrite);
	cl::ImageFormat cl_format = bitmapFormatToImageFormat(bitmap->getPixelFormat());
	if(cl_format.image_channel_data_type == 0 || cl_format.image_channel_order == 0) {
		WARN("Could not create image from bitmap (Unsupported image format).");
		FAIL();
	}

	cl_int err;
	cl::Image* image = new cl::Image2D(*context->_internal(), flags, cl_format, bitmap->getWidth(), bitmap->getHeight(), 0, bitmap->data(), &err);

	if(err != CL_SUCCESS) {
		WARN("Could not create image from bitmap (" + getErrorString(err) + ").");
		FAIL();
	}
	return image;
}

cl::Image* copyImageBuffer(const Image& source) {
	switch (source.getType()) {
		case ImageType::IMAGE_1D:
			return new cl::Image1D(*source._internal<cl::Image1D>());
		case ImageType::IMAGE_1D_ARRAY:
			return new cl::Image1DArray(*source._internal<cl::Image1DArray>());
		case ImageType::IMAGE_2D:
			return new cl::Image2D(*source._internal<cl::Image2D>());
		case ImageType::IMAGE_2D_ARRAY:
			return new cl::Image2DArray(*source._internal<cl::Image2DArray>());
		case ImageType::IMAGE_3D:
			return new cl::Image3D(*source._internal<cl::Image3D>());
		case ImageType::IMAGE_1D_BUFFER:
			return new cl::Image1DBuffer(*source._internal<cl::Image1DBuffer>());
		case ImageType::IMAGE_GL:
			#if !defined(CL_VERSION_1_2) || defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
				uint32_t target = source.getGLTextureTarget();
				if(target == TextureUtils::textureTypeToGLTextureType(TextureType::TEXTURE_2D)) {
					return new cl::Image2D(*source._internal<cl::Image2D>());
				} else if(target == TextureUtils::textureTypeToGLTextureType(TextureType::TEXTURE_3D)) {
					return new cl::Image3D(*source._internal<cl::Image3D>());
				} else {
					WARN_AND_FAIL("Unsupported texture target (OpenCL 1.1 only supports GL_TEXTURE_2D and GL_TEXTURE_3D).");
				}
			#else
				return new cl::ImageGL(*source._internal<cl::ImageGL>());
			#endif
	}
	return nullptr;
}

Image::Format::Format() :
		width(0), height(0), numLayers(1), type(ImageType::IMAGE_2D),
		pixelFormat({ChannelOrder::RGBA, ChannelType::UNSIGNED_INT8}),
		rowPitch(0), slicePitch(0) { }

Image::Image(Context* context, Format format, ReadWrite_t readWrite, HostPtr_t hostPtrUsage, void* hostPtr, ReadWrite_t hostReadWrite) :
		Memory(createImageBuffer(context, std::move(format), readWrite, hostPtrUsage, hostPtr, hostReadWrite)), type(format.type) { }

Image::Image(Context* context, Format format, ReadWrite_t readWrite, Buffer* buffer) : type(ImageType::IMAGE_1D_BUFFER) {
	cl_mem_flags flags = convertToCLFlags(readWrite, None, ReadWrite);
	cl::ImageFormat cl_format = pixelFormatToImageFormat(format.pixelFormat);
	cl_int err;
	mem.reset(new cl::Image1DBuffer(*context->_internal(), flags, cl_format, format.width, *buffer->_internal<cl::Buffer>(), &err));
	if(err != CL_SUCCESS) {
		WARN("Could not create Image from texture (" + getErrorString(err) + ").");
		FAIL();
	}
}

Image::Image(Context* context, ReadWrite_t readWrite, TextureType target, uint32_t glHandle, uint32_t mipLevel) : type(ImageType::IMAGE_GL) {
	cl_mem_flags flags = convertToCLFlags(readWrite, None, ReadWrite);
	uint32_t gl_target = TextureUtils::textureTypeToGLTextureType(target);
	cl_int err;
#if !defined(CL_VERSION_1_2) || defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
	if(target == TextureType::TEXTURE_2D) {
//		mem.reset(new cl::Image2DGL(*context->_internal(), flags, TextureUtils::textureTypeToGLTextureType(target), mipLevel, glHandle, &err));
		// Workaround to allow using Image2DGL with the opencl 1.2 headers
		cl_mem cl_obj = ::clCreateFromGLTexture2D((*context->_internal())(), flags, gl_target, mipLevel, glHandle, &err);
		mem.reset(new cl::Image2D(cl_obj));
	} else if(target == TextureType::TEXTURE_3D) {
//		mem.reset(new cl::Image3DGL(*context->_internal(), flags, TextureUtils::textureTypeToGLTextureType(target), mipLevel, glHandle, &err));
		// Workaround to allow using Image3DGL with the opencl 1.2 headers
		cl_mem cl_obj = ::clCreateFromGLTexture3D((*context->_internal())(), flags, gl_target, mipLevel, glHandle, &err);
		mem.reset(new cl::Image3D(cl_obj));
	} else {
		WARN_AND_FAIL("Unsupported texture target (OpenCL 1.1 only supports GL_TEXTURE_2D and GL_TEXTURE_3D).");
	}
#else
	mem.reset(new cl::ImageGL(*context->_internal(), flags, gl_target, mipLevel, glHandle, &err));
#endif
	if(err != CL_SUCCESS) {
		WARN_AND_FAIL("Could not create Image from texture (" + getErrorString(err) + ").");
	}
}

Image::Image(Context* context, ReadWrite_t readWrite, Texture* texture, uint32_t mipLevel) :
		Image(context, readWrite, texture->getTextureType(), texture->getGLId(), mipLevel) { }

Image::Image(Context* context, ReadWrite_t readWrite, Util::Bitmap* bitmap, HostPtr_t hostPtrUsage /*= Use*/, ReadWrite_t hostReadWrite /*= ReadWrite*/) :
		Memory(createImageBufferFromBitmap(context, readWrite, bitmap, hostPtrUsage, hostReadWrite)), type(ImageType::IMAGE_2D) { }

Image::Image(const Image& image) :
		Memory(copyImageBuffer(image)), type(image.type) { }

PixelFormatCL Image::getPixelFormat() const {
	cl_image_format format = _internal<cl::Image>()->getImageInfo<CL_IMAGE_FORMAT>();
	return imageFormatToPixelFormat(format);
}

size_t Image::getElementSize() const {
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_ELEMENT_SIZE>();
}

size_t Image::getRowPitch() const {
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_ROW_PITCH>();
}

size_t Image::getSlicePitch() const {
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_SLICE_PITCH>();
}

size_t Image::getWidth() const {
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_WIDTH>();
}

size_t Image::getHeight() const {
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_HEIGHT>();
}

size_t Image::getDepth() const {
	if(type == ImageType::IMAGE_1D_ARRAY || type == ImageType::IMAGE_3D) {
		size_t out;
		_internal<cl::Image>()->getImageInfo(CL_IMAGE_ARRAY_SIZE, &out);
		return out;
	} else if(type == ImageType::IMAGE_GL) {
		GLenum target = getGLTextureTarget();
		if(target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D) {
			return _internal<cl::Image>()->getImageInfo<CL_IMAGE_DEPTH>();
		}
	}
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_DEPTH>();
}

uint32_t Image::getNumMipLevels() const {
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_ELEMENT_SIZE>();
}

uint32_t Image::getNumSamples() const {
	return _internal<cl::Image>()->getImageInfo<CL_IMAGE_ELEMENT_SIZE>();
}

uint32_t Image::getGLTextureTarget() const {
	if(type != ImageType::IMAGE_GL)
		return 0;
	GLenum out;
	::clGetGLTextureInfo((*mem.get())(), CL_GL_TEXTURE_TARGET, sizeof(out), &out, nullptr);
	return out;
}

uint32_t Image::getMipmapLevel() const {
	if(type != ImageType::IMAGE_GL)
		return 0;
	GLint out;
	::clGetGLTextureInfo((*mem.get())(), CL_GL_MIPMAP_LEVEL, sizeof(out), &out, nullptr);
	return out;
}

} /* namespace CL */
} /* namespace Rendering */

