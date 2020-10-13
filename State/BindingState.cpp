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

#include <functional>

namespace Rendering {

//------------------

template<class Key, class Value>
static void overwriteMap(std::map<Key,Value>& tgt, const std::map<Key,Value>& src) {
	auto tgtIt = tgt.begin();
	auto srcIt = src.begin();
	while(tgtIt != tgt.end() && srcIt != src.end()) {
		if(srcIt->first < tgtIt->first) {
			// insert objects that are not in the target map
			tgt.insert(tgtIt, *srcIt);
			++srcIt;
		} else if(tgtIt->first == srcIt->first) {
			// overwrite objects in target using copy assignment operator
			tgtIt->second = srcIt->second;
			++tgtIt; ++srcIt;
		} else {
			// erase objects in target map
			tgtIt = tgt.erase(tgtIt);
		}
	}
	// insert/remove remaining objects
	tgt.erase(tgtIt, tgt.end());
	tgt.insert(srcIt, src.end());
}

//------------------

template<class Key, class Value, typename MergeOp=std::function<void(Value&,const Value&)>>
static void mergeMap(std::map<Key,Value>& tgt, const std::map<Key,Value>& src, MergeOp merge) {
	auto tgtIt = tgt.begin();
	auto srcIt = src.begin();
	while(tgtIt != tgt.end() && srcIt != src.end()) {
		if(srcIt->first < tgtIt->first) {
			// insert objects that are not in the target map
			tgt.insert(tgtIt, *srcIt);
			++srcIt;
		} else if(tgtIt->first == srcIt->first) {
			// merge objects in target
			merge(tgtIt->second, srcIt->second);
			++tgtIt; ++srcIt;
		} else {
			// skip
			++tgtIt;
		}
	}
	// insert/remove remaining objects
	tgt.insert(srcIt, src.end());
}

//------------------

template<class Value>
static void mergeArray(std::vector<Value>& tgt, const std::vector<Value>& src) {
	auto tgtIt = tgt.begin();
	auto srcIt = src.begin();
	while(tgtIt != tgt.end() && srcIt != src.end()) {
		if(*srcIt)
			*tgtIt = *srcIt;
		++tgtIt; ++srcIt;
	}
	// insert/remove remaining objects
	tgt.insert(tgt.end(), srcIt, src.end());
}

//------------------

Binding::~Binding() = default;

//------------------

Binding::Binding(Binding&& o) {
	buffer = std::move(o.buffer);
	texture = std::move(o.texture);
	dirty = true;
	o.dirty = true;
}

//------------------

Binding::Binding(const Binding& o) {
	buffer = o.buffer;
	texture = o.texture;
	dirty = true;
}

//------------------

Binding& Binding::operator=(Binding&& o) {
	buffer = std::move(o.buffer);
	texture = std::move(o.texture);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//------------------

Binding& Binding::operator=(const Binding& o) {
	dirty |= (*this != o);
	buffer = o.buffer;
	texture = o.texture;
	return *this;
}

//------------------

bool Binding::bind(const BufferObjectRef& obj) {
	if(texture || buffer != obj)
		markDirty();
	buffer = obj;
	texture = nullptr;
	return dirty;
}

//------------------

bool Binding::bind(const TextureRef& obj) {
	if(buffer || texture != obj)
		markDirty();
	buffer = nullptr;
	texture = obj;
	return dirty;
}

//------------------

BindingSet::~BindingSet() = default;

//------------------

BindingSet::BindingSet(BindingSet&& o) {
	bindings = std::move(o.bindings);
	dirty = true;
	o.dirty = true;
}

//------------------

BindingSet::BindingSet(const BindingSet& o) {
	bindings = o.bindings;
	dirty = true;
}

//------------------

BindingSet& BindingSet::operator=(BindingSet&& o) {
	bindings = std::move(o.bindings);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//------------------

BindingSet& BindingSet::operator=(const BindingSet& o) {
	dirty |= (*this != o);
	overwriteMap(bindings, o.bindings);
	return *this;
}

//------------------

bool BindingSet::bind(const BufferObjectRef& buffer, uint32_t binding, uint32_t arrayElement) {
	auto& bindingArray = bindings[binding];
	if(bindingArray.size() <= arrayElement)
		bindingArray.resize(arrayElement + 1);
	dirty |= bindingArray[arrayElement].bind(buffer);
	return dirty;
}

//------------------

bool BindingSet::bind(const TextureRef& texture, uint32_t binding, uint32_t arrayElement) {
	auto& bindingArray = bindings[binding];
	if(bindingArray.size() <= arrayElement)
		bindingArray.resize(arrayElement + 1);
	dirty |= bindingArray[arrayElement].bind(texture);
	return dirty;
}

//------------------

void BindingSet::merge(const BindingSet& other) {
	dirty |= (*this != other);
	mergeMap(bindings, other.bindings, [](std::vector<Binding>& tgt, const std::vector<Binding>& src) {
		mergeArray(tgt, src);
	});
}

//------------------

const Binding& BindingSet::getBinding(uint32_t binding, uint32_t arrayElement) const {
	static Binding nullBinding{};
	const auto& it = bindings.find(binding);
	if(it == bindings.end())
		return nullBinding;
	return it->second.size() > arrayElement ? it->second[arrayElement] : nullBinding;
}

//------------------

bool BindingSet::hasBinding(uint32_t binding, uint32_t arrayElement) const {
	const auto& it = bindings.find(binding);
	if(it == bindings.end())
		return false;
	return it->second.size() > arrayElement && it->second[arrayElement].isValid();
}

//------------------

void BindingSet::clearDirty() {
	for(auto& entry : bindings)
		for(auto& b : entry.second)
			b.clearDirty();
	dirty = false;
}

//------------------

bool BindingSet::isDirty() const {
	if(dirty) return true;
	for(const auto& entry : bindings)
		for(const auto& b : entry.second)
			if(b.isDirty())
				return true;
	return false;
}

//------------------

BindingState::~BindingState() = default;

//------------------

bool BindingState::bind(const BufferObjectRef& buffer, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	dirty |= bindingSets[set].bind(buffer, binding, arrayElement);
	return dirty;
}

//------------------

bool BindingState::bind(const TextureRef& texture, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	dirty |= bindingSets[set].bind(texture, binding, arrayElement);
	return dirty;
}

//------------------

void BindingState::merge(const BindingState& other) {
	dirty |= (*this != other);
	mergeMap(bindingSets, other.bindingSets, [](BindingSet& tgt, const BindingSet& src) {
		tgt.merge(src);
	});
}

//------------------

BindingState::BindingState(BindingState&& o) {
	bindingSets = std::move(o.bindingSets);
	dirty = o.dirty;
	o.dirty = true;
}

//------------------

BindingState::BindingState(const BindingState& o) {
	bindingSets = o.bindingSets;
	dirty = true;
}

//------------------

BindingState& BindingState::operator=(BindingState&& o) {
	bindingSets = std::move(o.bindingSets);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//------------------

BindingState& BindingState::operator=(const BindingState& o) {
	dirty |= (*this != o);
	overwriteMap(bindingSets, o.bindingSets);
	return *this;
}

//------------------

const Binding& BindingState::getBinding(uint32_t set, uint32_t binding, uint32_t arrayElement) const {
	static Binding nullBinding{};
	const auto it = bindingSets.find(set);
	if(it == bindingSets.end())
		return nullBinding;
	return it->second.getBinding(binding, arrayElement);
}

//------------------

bool BindingState::hasBinding(uint32_t set, uint32_t binding, uint32_t arrayElement) const {
	const auto it = bindingSets.find(set);
	if(it == bindingSets.end())
		return false;
	return it->second.hasBinding(binding, arrayElement);
}

//------------------

void BindingState::clearDirty() {
	for(auto& set : bindingSets)
		set.second.clearDirty();
	dirty = false;
}

//------------------

bool BindingState::isDirty() const {
	if(dirty) return true;
	for(const auto& set : bindingSets)
		if(set.second.isDirty())
			return true;
	return false;
}

//------------------

} /* Rendering */