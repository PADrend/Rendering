/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDS_BINDCOMMANDS_H_
#define RENDERING_CORE_COMMANDS_BINDCOMMANDS_H_

#include "Command.h"
#include "../../State/BindingState.h"
#include "../../State/ShaderLayout.h"
#include "../../State/PipelineState.h"

namespace Rendering {
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class DescriptorSet;
using DescriptorSetRef = Util::Reference<DescriptorSet>;

//------------------------------------------

class BindPipelineCommand : public Command {
PROVIDES_TYPE_NAME(BindPipelineCommand)
public:
	BindPipelineCommand(const PipelineState& pipeline, const PipelineHandle& parent=nullptr) : pipeline(pipeline), parentHandle(parent) {}
	~BindPipelineCommand();
	bool compile(CompileContext& context) override;
private:
	PipelineState pipeline;
	PipelineHandle parentHandle;
	PipelineHandle pipelineHandle;
};

//------------------------------------------

class BindSetCommand : public Command {
PROVIDES_TYPE_NAME(BindSetCommand)
public:
	BindSetCommand(uint32_t set, const BindingSet& bindingSet, const ShaderLayout& layout, PipelineType bindingPoint) :
		set(set), bindingSet(bindingSet), layout(layout), bindingPoint(bindingPoint) {}
	~BindSetCommand();
	bool compile(CompileContext& context) override;
private:
	uint32_t set;
	BindingSet bindingSet;
	ShaderLayout layout;
	PipelineType bindingPoint;
	DescriptorSetRef descriptorSet;
	PipelineLayoutHandle pipelineLayout;
};

//------------------------------------------

class BindVertexBuffersCommand : public Command {
PROVIDES_TYPE_NAME(BindVertexBuffersCommand)
public:
	BindVertexBuffersCommand(uint32_t firstBinding, const std::vector<BufferObjectRef>& buffers) :
		firstBinding(firstBinding), buffers(buffers) {}
	~BindVertexBuffersCommand();
	bool compile(CompileContext& context) override;
private:
	uint32_t firstBinding;
	std::vector<BufferObjectRef> buffers;
};

//------------------------------------------

class BindIndexBufferCommand : public Command {
PROVIDES_TYPE_NAME(BindIndexBufferCommand)
public:
	BindIndexBufferCommand(const BufferObjectRef& buffer) : buffer(buffer) {}
	~BindIndexBufferCommand();
	bool compile(CompileContext& context) override;
private:
	BufferObjectRef buffer;
};

//------------------------------------------

} /* Rendering */
#endif /* end of include guard: RENDERING_CORE_COMMANDS_BINDCOMMANDS_H_ */
