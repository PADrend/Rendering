/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BindingState.h"
#include "../Texture/Texture.h"
#include "../BufferObject.h"
#include "../Core/ImageView.h"

namespace Rendering {

//------------------

void Binding::bindBuffer(const BufferObjectRef& buffer, uint32_t arrayElement) {
	if(buffers.size() <= arrayElement)
		buffers.resize(arrayElement + 1);
	buffers[arrayElement] = buffer;
	textures.clear();
	views.clear();
	dirty = true;
}

//------------------

void Binding::bindTexture(const TextureRef& texture, uint32_t arrayElement) {
	if(textures.size() <= arrayElement)
		textures.resize(arrayElement + 1);
	buffers.clear();
	textures[arrayElement] = texture;
	views.clear();
	dirty = true;
}

//------------------

void Binding::bindInputImage(const ImageViewRef& view, uint32_t arrayElement) {
	if(views.size() <= arrayElement)
		views.resize(arrayElement + 1);
	buffers.clear();
	textures.clear();
	views[arrayElement] = view;
	dirty = true;
}

//------------------

void Binding::clearDirty() {
	dirty = false;
}

//------------------

void BindingSet::bindBuffer(const BufferObjectRef& buffer, uint32_t binding, uint32_t arrayElement) {
	bindings[binding].bindBuffer(buffer, arrayElement);
	dirty = true;
}

//------------------

void BindingSet::bindTexture(const TextureRef& texture, uint32_t binding, uint32_t arrayElement) {
	bindings[binding].bindTexture(texture, arrayElement);
	dirty = true;
}

//------------------

void BindingSet::bindInputImage(const ImageViewRef& view, uint32_t binding, uint32_t arrayElement) {
	bindings[binding].bindInputImage(view, arrayElement);
	dirty = true;
}

//------------------

void BindingSet::clearDirty() {
	dirty = false;
}

//------------------

void BindingSet::clearDirty(uint32_t binding) {
	bindings[binding].clearDirty();
}

//------------------

void BindingState::bindBuffer(const BufferObjectRef& buffer, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	bindingSets[set].bindBuffer(buffer, binding, arrayElement);
	dirty = true;
}

//------------------

void BindingState::bindTexture(const TextureRef& texture, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	bindingSets[set].bindTexture(texture, binding, arrayElement);
	dirty = true;
}

//------------------

void BindingState::bindInputImage(const ImageViewRef& view, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	bindingSets[set].bindInputImage(view, binding, arrayElement);
	dirty = true;
}

//------------------

BufferObjectRef BindingState::getBoundBuffer(uint32_t set, uint32_t binding, uint32_t arrayElement) {
	auto sIt = bindingSets.find(set);
	if(sIt == bindingSets.end())
		return nullptr;
	auto bIt = sIt->second.getBindings().find(binding);
	if(bIt == sIt->second.getBindings().end())
		return nullptr;
	if(bIt->second.getBuffers().size() <= arrayElement)
		return nullptr;
	return bIt->second.getBuffers().at(arrayElement);
}

//------------------

TextureRef BindingState::getBoundTexture(uint32_t set, uint32_t binding, uint32_t arrayElement) {
	auto sIt = bindingSets.find(set);
	if(sIt == bindingSets.end())
		return nullptr;
	auto bIt = sIt->second.getBindings().find(binding);
	if(bIt == sIt->second.getBindings().end())
		return nullptr;
	if(bIt->second.getTextures().size() <= arrayElement)
		return nullptr;
	return bIt->second.getTextures().at(arrayElement);
}

//------------------

ImageViewRef BindingState::getBoundInputImage(uint32_t set, uint32_t binding, uint32_t arrayElement) {
	auto sIt = bindingSets.find(set);
	if(sIt == bindingSets.end())
		return nullptr;
	auto bIt = sIt->second.getBindings().find(binding);
	if(bIt == sIt->second.getBindings().end())
		return nullptr;
	if(bIt->second.getInputImages().size() <= arrayElement)
		return nullptr;
	return bIt->second.getInputImages().at(arrayElement);
}

//------------------

void BindingState::reset() {
	bindingSets.clear();
	dirty = true;
}

//------------------

void BindingState::clearDirty() {
	dirty = false;
}

//------------------

void BindingState::clearDirty(uint32_t set) {
	bindingSets[set].clearDirty();
}

//------------------

BindingState::BindingState(BindingState&& o) : dirty(true) {
	bindingSets = std::move(o.bindingSets);
	dirty = true;
	o.dirty = true;
}

//------------------

BindingState::BindingState(const BindingState& o) : dirty(true) {
	for(auto& s : o.bindingSets) {
		for(auto& b : s.second.getBindings()) {
			for(uint32_t i = 0; i < b.second.getBuffers().size(); ++i)
				bindBuffer(b.second.getBuffers()[i], s.first, b.first, i);
			for(uint32_t i = 0; i < b.second.getTextures().size(); ++i)
				bindTexture(b.second.getTextures()[i], s.first, b.first, i);
			for(uint32_t i = 0; i < b.second.getInputImages().size(); ++i)
				bindInputImage(b.second.getInputImages()[i], s.first, b.first, i);
		}
	}
}

//------------------

BindingState& BindingState::operator=(BindingState&& o) {
	bindingSets = std::move(o.bindingSets);
	dirty = true;
	o.dirty = true;
}

//------------------

BindingState& BindingState::operator=(const BindingState& o) {
	for(auto& s : o.bindingSets) {
		for(auto& b : s.second.getBindings()) {
			for(uint32_t i = 0; i < b.second.getBuffers().size(); ++i)
				bindBuffer(b.second.getBuffers()[i], s.first, b.first, i);
			for(uint32_t i = 0; i < b.second.getTextures().size(); ++i)
				bindTexture(b.second.getTextures()[i], s.first, b.first, i);
			for(uint32_t i = 0; i < b.second.getInputImages().size(); ++i)
				bindInputImage(b.second.getInputImages()[i], s.first, b.first, i);
		}
	}
	dirty = true;
}

//------------------

} /* Rendering */