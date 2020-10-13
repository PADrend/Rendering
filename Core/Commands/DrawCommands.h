/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDS_DRAWCOMMANDS_H_
#define RENDERING_CORE_COMMANDS_DRAWCOMMANDS_H_

#include "Command.h"
#include <Geometry/Rect.h>
#include <Util/Graphics/Color.h>

namespace Rendering {
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;

class DrawCommand : public Command {
public:
	DrawCommand(uint32_t vertexCount, uint32_t instanceCount=1, uint32_t firstVertex=0, uint32_t firstInstance=0) :
		vertexCount(vertexCount), instanceCount(instanceCount), firstVertex(firstVertex), firstInstance(firstInstance) {}
	bool compile(CompileContext& context) override;
private:
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

//------------------------------------------

class DrawIndexedCommand : public Command {
public:
	DrawIndexedCommand(uint32_t indexCount, uint32_t instanceCount=1, uint32_t firstIndex=0, uint32_t vertexOffset=0, uint32_t firstInstance=0) :
		indexCount(indexCount), instanceCount(instanceCount), firstIndex(firstIndex), vertexOffset(vertexOffset), firstInstance(firstInstance) {}
	bool compile(CompileContext& context) override;
private:
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	uint32_t vertexOffset;
	uint32_t firstInstance;
};

//------------------------------------------

class DrawIndirectCommand : public Command {
public:
	DrawIndirectCommand(const BufferObjectRef& buffer, uint32_t drawCount=0, uint32_t stride=0, size_t offset=0) :
		buffer(buffer), drawCount(drawCount), stride(stride), offset(offset) {}
	~DrawIndirectCommand();
	bool compile(CompileContext& context) override;
private:
	BufferObjectRef buffer;
	uint32_t drawCount;
	uint32_t stride;
	size_t offset;
};

//------------------------------------------

class DrawIndexedIndirectCommand : public Command {
public:
	DrawIndexedIndirectCommand(const BufferObjectRef& buffer, uint32_t drawCount=0, uint32_t stride=0, size_t offset=0) :
		buffer(buffer), drawCount(drawCount), stride(stride), offset(offset) {}
	~DrawIndexedIndirectCommand();
	bool compile(CompileContext& context) override;
private:
	BufferObjectRef buffer;
	uint32_t drawCount;
	uint32_t stride;
	size_t offset;
};

//------------------------------------------

class ClearAttachmentsCommand : public Command {
public:
	ClearAttachmentsCommand(std::vector<Util::Color4f> colors, float depthValue, uint32_t stencilValue, bool clearColor, bool clearDepth, bool clearStencil, const Geometry::Rect_i& rect) :
		colors(colors), depthValue(depthValue), stencilValue(stencilValue), rect(rect), clearColor(clearColor), clearDepth(clearDepth), clearStencil(clearStencil) {}
	~ClearAttachmentsCommand() = default;
	bool compile(CompileContext& context) override;
private:
	std::vector<Util::Color4f> colors;
	float depthValue;
	uint32_t stencilValue;
	Geometry::Rect_i rect;
	bool clearColor;
	bool clearDepth;
	bool clearStencil;
};

//------------------------------------------

} /* Rendering */
#endif /* end of include guard: RENDERING_CORE_COMMANDS_DRAWCOMMANDS_H_ */
