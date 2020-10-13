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
#include <Util/StringIdentifier.h>
#include <Util/Resources/ResourceFormat.h>
#include <Util/Resources/ResourceAccessor.h>

#include <vector>

namespace Rendering {
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class BufferPool;
using BufferPoolRef = Util::Reference<BufferPool>;
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
	static Ref create(const BufferPoolRef& pool, const Util::ResourceFormat& format, uint32_t arraySize=1, bool pushConstant=false);
	static Ref createFromShaderResource(const BufferPoolRef& pool, const ShaderResource& resource);
	~UniformBuffer();
	UniformBuffer(UniformBuffer&& o) = default;
	UniformBuffer(const UniformBuffer& o) = delete;

	[[deprecated]]
	void applyUniform(const Uniform& uniform, uint32_t index=0);

	void writeData(const Util::StringIdentifier& name, const uint8_t* data, size_t size, uint32_t index=0);

	template<typename T>
	void writeValue(const Util::StringIdentifier& name, const T& value, uint32_t index=0);

	template<typename T>
	void writeValues(const Util::StringIdentifier& name, const std::vector<T>& values, uint32_t index=0);

	void flush(const CommandBufferRef& cmd, bool force=false);
	void bind(const CommandBufferRef& cmd, uint32_t binding=0, uint32_t set=0);

	const Util::ResourceFormat& getFormat() const { return accessor->getFormat(); }
	size_t getSize() const { return cache.size(); }
	uint32_t getElementCount() const { return arraySize; }
private:
	explicit UniformBuffer(const BufferPoolRef& pool, uint32_t arraySize, bool pushConstant);
	bool init(const Util::ResourceFormat& format);
	
	Util::ResourceAccessor::Ref accessor;
	BufferPoolRef pool;
	std::deque<BufferObjectRef> buffers;
	std::vector<uint8_t> cache;
	uint32_t arraySize;
	bool pushConstant;
	bool dataHasChanged;
};

//---------------

template<typename T>
void UniformBuffer::writeValue(const Util::StringIdentifier& name, const T& value, uint32_t index) {
	uint32_t location = getFormat().getAttributeLocation(name);
	if(location < getSize() && index < arraySize) {
		accessor->writeValue<T>(index, name, value);
		dataHasChanged = true;
	}
}

//---------------

template<typename T>
void UniformBuffer::writeValues(const Util::StringIdentifier& name, const std::vector<T>& values, uint32_t index) {
	uint32_t location = getFormat().getAttributeLocation(name);
	if(location < getSize() && index < arraySize) {
		accessor->writeValues<T>(index, name, values);
		dataHasChanged = true;
	}
}

} /* Rendering */

#endif /* end of include guard: RENDERING_SHADER_UNIFORMBUFFER_H_ */