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
#include "../RenderingContext/BindingState.h"

#include <Util/ReferenceCounter.h>

#include <memory>

namespace Rendering {
class DescriptorPool;
using DescriptorPoolRef = Util::Reference<DescriptorPool>;

//----------------------------------------

class DescriptorSet : public Util::ReferenceCounter<DescriptorSet> {
public:
	using Ref = Util::Reference<DescriptorSet>;
	static Ref create(const DescriptorPoolRef& pool, const BindingSet& bindings);

	~DescriptorSet();
	DescriptorSet(DescriptorSet&& o) = default;
	DescriptorSet(const DescriptorSet& o) = delete;
	DescriptorSet& operator=(DescriptorSet&& o) = default;
	DescriptorSet& operator=(const DescriptorSet& o) = delete;

	bool update(const BindingSet& bindings);

	const DescriptorSetHandle& getApiHandle() const { return handle; }
private:
	friend class DescriptorPool;
	DescriptorSet(const DescriptorPoolRef& pool, const BindingSet& bindings);
	bool init();
	
	const DescriptorPoolRef pool;
	BindingSet bindings;
	DescriptorSetHandle handle;
	DescriptorPoolHandle poolHandle;
	DescriptorSetLayoutHandle layoutHandle;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_DESCRIPTORSET_H_ */