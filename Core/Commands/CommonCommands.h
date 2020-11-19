/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDS_COMMONCOMMANDS_H_
#define RENDERING_CORE_COMMANDS_COMMONCOMMANDS_H_

#include "Command.h"
#include "../../State/ShaderLayout.h"
#include <Util/Graphics/Color.h>

namespace Rendering {
class CommandBuffer;
using CommandBufferRef = Util::Reference<CommandBuffer>;
class ImageStorage;
using ImageStorageRef = Util::Reference<ImageStorage>;
class ImageView;
using ImageViewRef = Util::Reference<ImageView>;
class Texture;
using TextureRef = Util::Reference<Texture>;
class FBO;
using FBORef = Util::Reference<FBO>;

//------------------------------------------

class ExecuteCommandBufferCommand : public Command {
PROVIDES_TYPE_NAME(ExecuteCommandBufferCommand)
public:
	ExecuteCommandBufferCommand(const CommandBufferRef& buffer) : buffer(buffer) {}
	~ExecuteCommandBufferCommand();
	bool compile(CompileContext& context) override;
private:
	CommandBufferRef buffer;
};

//------------------------------------------

class BeginRenderPassCommand : public Command {
PROVIDES_TYPE_NAME(BeginRenderPassCommand)
public:
	RENDERINGAPI BeginRenderPassCommand(const FBORef& fbo, std::vector<Util::Color4f> colors, float depthValue, uint32_t stencilValue, bool clearColor, bool clearDepth, bool clearStencil);
	~BeginRenderPassCommand();
	bool compile(CompileContext& context) override;
private:
	FBORef fbo;
	std::vector<Util::Color4f> colors;
	float depthValue;
	uint32_t stencilValue;
	bool clearColor;
	bool clearDepth;
	bool clearStencil;
	FramebufferHandle framebuffer;
	RenderPassHandle renderPass;
};

//------------------------------------------

class EndRenderPassCommand : public Command {
PROVIDES_TYPE_NAME(EndRenderPassCommand)
public:
	EndRenderPassCommand(const FBORef& fbo) : fbo(fbo) {}
	~EndRenderPassCommand() = default;
	bool compile(CompileContext& context) override;
private:
	FBORef fbo;
};

//------------------------------------------

class PrepareForPresentCommand : public Command {
PROVIDES_TYPE_NAME(PrepareForPresentCommand)
public:
	PrepareForPresentCommand() {}
	~PrepareForPresentCommand() = default;
	bool compile(CompileContext& context) override;
};

//------------------------------------------

class PushConstantCommand : public Command {
PROVIDES_TYPE_NAME(PushConstantCommand)
public:
	PushConstantCommand(const uint8_t* data, size_t size, size_t offset, ShaderLayout layout) :
		constantData(data, data+size), offset(offset), layout(layout) { }
	~PushConstantCommand();
	bool compile(CompileContext& context) override;
private:
	std::vector<uint8_t> constantData;
	size_t offset;
	ShaderLayout layout;
	PipelineLayoutHandle pipelineLayout;
};

//------------------------------------------

class ImageBarrierCommand : public Command {
PROVIDES_TYPE_NAME(ImageBarrierCommand)
public:
	RENDERINGAPI ImageBarrierCommand(const TextureRef& texture, ResourceUsage newUsage);
	ImageBarrierCommand(const ImageViewRef& view, ResourceUsage newUsage) : view(view), image(nullptr), newUsage(newUsage) {}
	ImageBarrierCommand(const ImageStorageRef& image, ResourceUsage newUsage) : view(nullptr), image(image), newUsage(newUsage) {}
	~ImageBarrierCommand();
	bool compile(CompileContext& context) override;
private:
	ImageViewRef view;
	ImageStorageRef image;
	ResourceUsage newUsage;
};

//------------------------------------------

class DebugMarkerCommand : public Command {
PROVIDES_TYPE_NAME(DebugMarkerCommand)
public:
	enum Mode {
		Begin,
		End,
		Insert,
	};
	DebugMarkerCommand(const std::string& name, const Util::Color4f& color, Mode mode=Mode::Insert) : name(name), color(color), mode(mode) {}
	~DebugMarkerCommand() = default;
	bool compile(CompileContext& context) override;
private:
	std::string name;
	Util::Color4f color;
	Mode mode;
};

//------------------------------------------

} /* Rendering */
#endif /* end of include guard: RENDERING_CORE_COMMANDS_COMMONCOMMANDS_H_ */
