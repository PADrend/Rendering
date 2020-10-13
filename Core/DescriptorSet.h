/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_DESCRIPTORSET_H_
#define RENDERING_CORE_DESCRIPTORSET_H_

#include "Common.h"
#include "../State/BindingState.h"
#include "../Shader/ShaderUtils.h"

#include <Util/ReferenceCounter.h>

#include <vector>

namespace Rendering {
class DescriptorPool;
using DescriptorPoolRef = Util::Reference<DescriptorPool>;

//----------------------------------------

class DescriptorSet : public Util::ReferenceCounter<DescriptorSet> {
public:
	using Ref = Util::Reference<DescriptorSet>;
	static Ref create(const DescriptorPoolRef& pool);

	~DescriptorSet();
	DescriptorSet(DescriptorSet&& o) = default;
	DescriptorSet(const DescriptorSet& o) = delete;

	void update(const BindingSet& bindings);
	const std::vector<uint32_t>& getDynamicOffsets() const { return dynamicOffsets; }

	const ShaderResourceLayoutSet& getLayout() const;
	const DescriptorSetHandle& getApiHandle() const { return handle; }
	const DescriptorSetLayoutHandle& getLayoutHandle() const { return layoutHandle; }
private:
	friend class DescriptorPool;
	DescriptorSet(const DescriptorPoolRef& pool);
	bool init();
	
	const DescriptorPoolRef pool;
	DescriptorSetHandle handle;
	DescriptorPoolHandle poolHandle;
	DescriptorSetLayoutHandle layoutHandle;
	std::vector<uint32_t> dynamicOffsets;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_DESCRIPTORSET_H_ */