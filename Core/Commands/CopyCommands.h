/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDS_COPYCOMMANDS_H_
#define RENDERING_CORE_COMMANDS_COPYCOMMANDS_H_

#include "Command.h"
#include <Util/Graphics/Color.h>

namespace Rendering {
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class BufferStorage;
using BufferStorageRef = Util::Reference<BufferStorage>;
class Texture;
using TextureRef = Util::Reference<Texture>;
class ImageStorage;
using ImageStorageRef = Util::Reference<ImageStorage>;
class ImageView;
using ImageViewRef = Util::Reference<ImageView>;

//------------------------------------------

class CopyBufferCommand : public Command {
PROVIDES_TYPE_NAME(CopyBufferCommand)
public:
	CopyBufferCommand(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size, size_t srcOffset=0, size_t tgtOffset=0) : 
		srcBuffer(srcBuffer), tgtBuffer(tgtBuffer), size(size), srcOffset(srcOffset), tgtOffset(tgtOffset) {}
	CopyBufferCommand(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size, size_t srcOffset=0, size_t tgtOffset=0);
	~CopyBufferCommand();
	bool compile(CompileContext& context) override;
private:
	BufferObjectRef srcBuffer;
	BufferObjectRef tgtBuffer;
	size_t size;
	size_t srcOffset;
	size_t tgtOffset;
};

//------------------------------------------

class UpdateBufferCommand : public Command {
PROVIDES_TYPE_NAME(UpdateBufferCommand)
public:
	UpdateBufferCommand(const BufferObjectRef& buffer, const uint8_t* data, size_t size, size_t tgtOffset=0) : 
		srcData(data, data+size), tgtBuffer(buffer), tgtOffset(tgtOffset) {}
	UpdateBufferCommand(const BufferStorageRef& buffer, const uint8_t* data, size_t size, size_t tgtOffset=0);
	~UpdateBufferCommand();
	bool compile(CompileContext& context) override;
private:
	std::vector<uint8_t> srcData;
	BufferObjectRef tgtBuffer;
	size_t tgtOffset;
};

//------------------------------------------

class CopyImageCommand : public Command {
PROVIDES_TYPE_NAME(CopyImageCommand)
public:
	CopyImageCommand(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion) : 
		srcImage(srcImage), tgtImage(tgtImage), srcRegion(srcRegion), tgtRegion(tgtRegion) {}
	~CopyImageCommand();
	bool compile(CompileContext& context) override;
private:
	ImageStorageRef srcImage;
	ImageStorageRef tgtImage;
	ImageRegion srcRegion;
	ImageRegion tgtRegion;
};

//------------------------------------------

class CopyBufferToImageCommand : public Command {
PROVIDES_TYPE_NAME(CopyBufferToImageCommand)
public:
	CopyBufferToImageCommand(const BufferStorageRef& srcBuffer, const ImageStorageRef& tgtImage, size_t srcOffset, const ImageRegion& tgtRegion) : 
		srcBuffer(srcBuffer), tgtImage(tgtImage), srcOffset(srcOffset), tgtRegion(tgtRegion) {}
	~CopyBufferToImageCommand();
	bool compile(CompileContext& context) override;
private:
	BufferStorageRef srcBuffer;
	ImageStorageRef tgtImage;
	size_t srcOffset;
	ImageRegion tgtRegion;
};

//------------------------------------------

class CopyImageToBufferCommand : public Command {
PROVIDES_TYPE_NAME(CopyImageToBufferCommand)
public:
	CopyImageToBufferCommand(const ImageStorageRef& srcImage, const BufferStorageRef& tgtBuffer, const ImageRegion& srcRegion, size_t tgtOffset) : 
		srcImage(srcImage), tgtBuffer(tgtBuffer), srcRegion(srcRegion), tgtOffset(tgtOffset) {}
	~CopyImageToBufferCommand();
	bool compile(CompileContext& context) override;
private:
	ImageStorageRef srcImage;
	BufferStorageRef tgtBuffer;
	ImageRegion srcRegion;
	size_t tgtOffset;
};

//------------------------------------------

class BlitImageCommand : public Command {
PROVIDES_TYPE_NAME(BlitImageCommand)
public:
	BlitImageCommand(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion, ImageFilter filter=ImageFilter::Nearest) : 
		srcImage(srcImage), tgtImage(tgtImage), srcRegion(srcRegion), tgtRegion(tgtRegion), filter(filter) {}
	~BlitImageCommand();
	bool compile(CompileContext& context) override;
private:
	ImageStorageRef srcImage;
	ImageStorageRef tgtImage;
	ImageRegion srcRegion;
	ImageRegion tgtRegion;
	ImageFilter filter;
};

//------------------------------------------

class ClearImageCommand : public Command {
PROVIDES_TYPE_NAME(ClearImageCommand)
public:
	ClearImageCommand(const ImageViewRef& view, const Util::Color4f& color) : view(view), image(nullptr), color(color) {}
	ClearImageCommand(const ImageStorageRef& image, const Util::Color4f& color) : view(nullptr), image(image), color(color) {}
	RENDERINGAPI ClearImageCommand(const TextureRef& texture, const Util::Color4f& color);
	~ClearImageCommand();
	bool compile(CompileContext& context) override;
private:
	ImageViewRef view;
	ImageStorageRef image;
	Util::Color4f color;
};

//------------------------------------------

} /* Rendering */
#endif /* end of include guard: RENDERING_CORE_COMMANDS_COPYCOMMANDS_H_ */
