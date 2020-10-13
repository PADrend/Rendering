/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_SHADER_UNIFORMBUFFER_H_
#define RENDERING_SHADER_UNIFORMBUFFER_H_

#include <Util/ReferenceCounter.h>

#include <vector>

namespace Util {
class ResourceFormat;
class ResourceAccessor;
using ResourceAccessorRef = Util::Reference<ResourceAccessor>;
} /* Util */

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class CommandBuffer;
using CommandBufferRef = Util::Reference<CommandBuffer>;
class Uniform;
struct ShaderResource;

/**
 * UniformBuffer
 * @ingroup Shader
 */
class UniformBuffer : public Util::ReferenceCounter<UniformBuffer> {
public:
	using Ref = Util::Reference<UniformBuffer>;
	static Ref create(const DeviceRef& device, const Util::ResourceFormat& format, uint32_t arraySize=1, bool pushConstant=false);
	static Ref createFromShaderResource(const DeviceRef& device, const ShaderResource& resource);
	~UniformBuffer();
	UniformBuffer(UniformBuffer&& o) = default;
	UniformBuffer(const UniformBuffer& o) = delete;

	void applyUniform(const Uniform& uniform);

	void flush(const CommandBufferRef& cmd, bool force=false);
	void bind(const CommandBufferRef& cmd, uint32_t binding=0, uint32_t set=0);

	const Util::ResourceFormat& getFormat() const;
private:
	explicit UniformBuffer(bool pushConstant);
	bool init(const DeviceRef& device, const Util::ResourceFormat& format, uint32_t arraySize);
	
	Util::ResourceAccessorRef accessor;
	BufferObjectRef buffer;
	std::vector<uint8_t> cache;
	uint32_t arraySize;
	bool pushConstant;
	bool dataHasChanged;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_SHADER_UNIFORMBUFFER_H_ */