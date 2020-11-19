/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_RENDERINGCONTEXT_BINDINGSTATE_H_
#define RENDERING_RENDERINGCONTEXT_BINDINGSTATE_H_

#include "../Core/Common.h"

#include <Util/ReferenceCounter.h>

#include <unordered_map>
#include <map>
#include <vector>

namespace Rendering {
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class Texture;
using TextureRef = Util::Reference<Texture>;

//------------------

class Binding {
HAS_DIRTY_FLAG
public:
	Binding() = default;
	RENDERINGAPI ~Binding();
	RENDERINGAPI Binding(Binding&& o);
	RENDERINGAPI Binding(const Binding& o);
	RENDERINGAPI Binding& operator=(Binding&& o);
	RENDERINGAPI Binding& operator=(const Binding& o);
	
	bool operator==(const Binding& o) const { return buffer == o.buffer && texture == o.texture; }
	bool operator!=(const Binding& o) const { return !(*this == o); }
	operator bool() const { return texture || buffer; }

	RENDERINGAPI bool bind(const BufferObjectRef& obj);
	RENDERINGAPI bool bind(const TextureRef& obj);

	const BufferObjectRef& getBuffer() const { return buffer; }
	const TextureRef& getTexture() const { return texture; }
	bool isValid() const { return texture || buffer; }
private:
	BufferObjectRef buffer;
	TextureRef texture;
};

//------------------

class BindingSet {
public:
	using BindingMap = std::map<uint32_t, std::vector<Binding>>;
	BindingSet() = default;
	RENDERINGAPI ~BindingSet();
	RENDERINGAPI BindingSet(BindingSet&& o);
	RENDERINGAPI BindingSet(const BindingSet& o);
	RENDERINGAPI BindingSet& operator=(BindingSet&& o);
	RENDERINGAPI BindingSet& operator=(const BindingSet& o);

	bool operator==(const BindingSet& o) const { return bindings == o.bindings; }
	bool operator!=(const BindingSet& o) const { return !(*this == o); }

	RENDERINGAPI bool bind(const BufferObjectRef& buffer, uint32_t binding=0, uint32_t arrayElement=0);
	RENDERINGAPI bool bind(const TextureRef& texture, uint32_t binding=0, uint32_t arrayElement=0);
	RENDERINGAPI void setArraySize(uint32_t binding, uint32_t arraySize);
	RENDERINGAPI void merge(const BindingSet& other, bool overwriteExisting=true);

	const BindingMap& getBindings() const { return bindings; }
	const Binding& getBinding(uint32_t binding, uint32_t arrayElement=0) const;
	bool hasBinding(uint32_t binding, uint32_t arrayElement=0) const;
	
private:
	BindingMap bindings;
	bool dirty = true;
public:
	void markDirty() { dirty = true; }
	RENDERINGAPI void clearDirty();
	RENDERINGAPI bool isDirty() const;
};

//------------------

class BindingState {
public:
	using BindingSetMap = std::map<uint32_t, BindingSet>;
	BindingState() = default;
	RENDERINGAPI ~BindingState();
	RENDERINGAPI BindingState(BindingState&& o);
	RENDERINGAPI BindingState(const BindingState& o);
	RENDERINGAPI BindingState& operator=(BindingState&& o);
	RENDERINGAPI BindingState& operator=(const BindingState& o);

	bool operator==(const BindingState& o) const { return bindingSets == o.bindingSets; }
	bool operator!=(const BindingState& o) const { return !(*this == o); }

	RENDERINGAPI bool bind(const BufferObjectRef& buffer, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	RENDERINGAPI bool bind(const TextureRef& texture, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	RENDERINGAPI void merge(const BindingState& other, bool overwriteExisting=true);

	RENDERINGAPI const Binding& getBinding(uint32_t set, uint32_t binding, uint32_t arrayElement=0) const;
	RENDERINGAPI bool hasBinding(uint32_t set, uint32_t binding, uint32_t arrayElement=0) const;

	const BindingSetMap& getBindingSets() { return bindingSets; }
	const BindingSet& getBindingSet(uint32_t set) { return bindingSets.at(set); }
	bool hasBindingSet(uint32_t set) const { return bindingSets.find(set) != bindingSets.end(); }

	void reset(){
		bindingSets.clear();
		dirty = true;
	}
private:
	BindingSetMap bindingSets;
	bool dirty = true;
public:
	void markDirty() { dirty = true; }
	RENDERINGAPI void clearDirty();
	RENDERINGAPI bool isDirty() const;
};

//------------------

} /* Rendering */

#endif /* end of include guard: RENDERING_RENDERINGCONTEXT_BINDINGSTATE_H_ */