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
#include "../Shader/ShaderUtils.h"

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
	~Binding();
	Binding(Binding&& o);
	Binding(const Binding& o);
	Binding& operator=(Binding&& o);
	Binding& operator=(const Binding& o);
	
	bool operator==(const Binding& o) const { return buffer == o.buffer && texture == o.texture; }
	bool operator!=(const Binding& o) const { return !(*this == o); }

	bool bind(const BufferObjectRef& obj);
	bool bind(const TextureRef& obj);

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
	~BindingSet();
	BindingSet(BindingSet&& o);
	BindingSet(const BindingSet& o);
	BindingSet& operator=(BindingSet&& o);
	BindingSet& operator=(const BindingSet& o);

	bool operator==(const BindingSet& o) const { return bindings == o.bindings; }
	bool operator!=(const BindingSet& o) const { return !(*this == o); }

	bool bind(const BufferObjectRef& buffer, uint32_t binding=0, uint32_t arrayElement=0);
	bool bind(const TextureRef& texture, uint32_t binding=0, uint32_t arrayElement=0);
	void setArraySize(uint32_t binding, uint32_t arraySize);

	const BindingMap& getBindings() const { return bindings; }
	const Binding& getBinding(uint32_t binding, uint32_t arrayElement=0) const;
	bool hasBinding(uint32_t binding, uint32_t arrayElement=0) const;
	
private:
	BindingMap bindings;
	bool dirty = true;
public:
	void markDirty() { dirty = true; }
	void clearDirty();
	bool isDirty() const;
};

//------------------

class BindingState {
public:
	using BindingSetMap = std::map<uint32_t, BindingSet>;
	BindingState() = default;
	~BindingState();
	BindingState(BindingState&& o);
	BindingState(const BindingState& o);
	BindingState& operator=(BindingState&& o);
	BindingState& operator=(const BindingState& o);

	bool operator==(const BindingState& o) const { return bindingSets == o.bindingSets; }
	bool operator!=(const BindingState& o) const { return !(*this == o); }

	bool bind(const BufferObjectRef& buffer, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	bool bind(const TextureRef& texture, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);

	const Binding& getBinding(uint32_t set, uint32_t binding, uint32_t arrayElement=0) const;
	bool hasBinding(uint32_t set, uint32_t binding, uint32_t arrayElement=0) const;

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
	void clearDirty();
	bool isDirty() const;
};

//------------------

} /* Rendering */

#endif /* end of include guard: RENDERING_RENDERINGCONTEXT_BINDINGSTATE_H_ */