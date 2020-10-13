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
class ImageView;
using ImageViewRef = Util::Reference<ImageView>;

//------------------

class Binding {
public:
	void bindBuffer(const BufferObjectRef& buffer, uint32_t arrayElement=0);
	void bindTexture(const TextureRef& texture, uint32_t arrayElement=0);
	void bindInputImage(const ImageViewRef& view, uint32_t arrayElement=0);

	bool isDirty() const { return dirty; }
	void clearDirty();

	const std::vector<BufferObjectRef>& getBuffers() const { return buffers; }
	const std::vector<TextureRef>& getTextures() const { return textures; }
	const std::vector<ImageViewRef>& getInputImages() const { return views; }
private:
	std::vector<BufferObjectRef> buffers;
	std::vector<TextureRef> textures;
	std::vector<ImageViewRef> views;
	bool dirty = true;
};

//------------------

class BindingSet {
public:
	using BindingMap = std::map<uint32_t, Binding>;

	void bindBuffer(const BufferObjectRef& buffer, uint32_t binding=0, uint32_t arrayElement=0);
	void bindTexture(const TextureRef& texture, uint32_t binding=0, uint32_t arrayElement=0);
	void bindInputImage(const ImageViewRef& view, uint32_t binding=0, uint32_t arrayElement=0);

	bool isDirty() const { return dirty; }
	void clearDirty();
	void clearDirty(uint32_t binding);

	const BindingMap& getBindings() const { return bindings; }
private:
	BindingMap bindings;
	bool dirty = true;
};

//------------------

class BindingState : public Util::ReferenceCounter<BindingState> {
public:
	using Ref = Util::Reference<BindingState>;

	void bindBuffer(const BufferObjectRef& buffer, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindTexture(const TextureRef& texture, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindInputImage(const ImageViewRef& view, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);

	void reset();
	bool isDirty() const { return dirty; }
	void clearDirty();
	void clearDirty(uint32_t set);

	const std::unordered_map<uint32_t, BindingSet>& getBindingSets() const { return bindingSets; }
private:
	std::unordered_map<uint32_t, BindingSet> bindingSets;
	bool dirty = true;
};

//------------------

} /* Rendering */

#endif /* end of include guard: RENDERING_RENDERINGCONTEXT_BINDINGSTATE_H_ */