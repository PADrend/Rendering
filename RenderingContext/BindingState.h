/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_BINDING_STATE_H_
#define RENDERING_BINDING_STATE_H_

#include "RenderingParameters.h"
#include "../Memory/BufferObject.h"
#include "../Memory/BufferView.h"
#include "../Texture/Texture.h"
#include "../Helper.h"
#include <Util/References.h>
#include <Geometry/Rect.h>
#include <cstdint>
#include <bitset>
#include <unordered_map>

namespace Rendering {

//! (internal) Used to track binding states of buffers & textures
class BindingState {
// -------------------------------
public:
	struct StateDiff_t {
		std::bitset<128> ssbos;
		std::bitset<128> ubos;
		std::bitset<8> acbos;
		std::bitset<4> tfbos;
		std::bitset<256> textures;
	};
	StateDiff_t makeDiff(const BindingState& other, bool forced=false) const;
	void apply(const StateDiff_t& diff);

// ------

//!	@name Buffers
//	@{
struct BufferBinding {
	BufferBinding() {}
	BufferBinding(uint32_t target, uint32_t location) : target(target), location(location) {}
	BufferBinding(Util::Reference<BufferView> _buffer, uint32_t target, uint32_t location) :
			buffer(_buffer), target(target), location(location), offset(buffer.isNotNull() ? buffer->getOffset() : 0), size(buffer.isNotNull() ? buffer->getSize() : 0) {}
	Util::Reference<BufferView> buffer;
	union {
		struct { uint32_t target; uint32_t location; };
		uint64_t _key;
	};
	size_t offset;
	size_t size;	
	bool operator!=(const BufferBinding& o) const {
		return buffer != o.buffer || _key != o._key || offset != o.offset || size != o.size;
	}
};
private:
	std::unordered_map<uint64_t, BufferBinding> buffers;
	uint32_t bufferCheckNumber = 0;
public:
	void bindBuffer(uint32_t target, uint32_t location, Util::Reference<BufferView> buffer) {
		if(getMaxBufferBindings(target) <= location) return;
		++bufferCheckNumber;
		BufferBinding binding(std::move(buffer), target, location);
		buffers[binding._key] = std::move(binding);
	}
	const BufferBinding & getBufferBinding(uint32_t target, uint32_t location) const {
		static const BufferBinding nullBinding;
		BufferBinding tmp(target, location);
		const auto it = buffers.find(tmp._key);
		return it == buffers.end() ? nullBinding : it->second;
	}
	const BufferBinding & getBufferBinding(uint64_t key) const {
		static const BufferBinding nullBinding;
		const auto it = buffers.find(key);
		return it == buffers.end() ? nullBinding : it->second;
	}
//	@}

//!	@name Textures
//	@{
private:
	std::unordered_map<uint8_t, Util::Reference<Texture>> textures;
	uint32_t texturesCheckNumber = 0;

public:	
	void bindTexture(uint8_t unit, Util::Reference<Texture> texture) {
		if(getMaxTextureBindings() <= unit) return;
		++texturesCheckNumber;
		textures[unit] = std::move(texture);
	}
	const Util::Reference<Texture> & getTexture(uint8_t unit) const {
		static const Util::Reference<Texture> nullTexture;
		const auto it = textures.find(unit);
		return it == textures.end() ? nullTexture : it->second;
	}
//	@}
};

}

#endif /* RENDERING_BINDING_STATE_H_ */
