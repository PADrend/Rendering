/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_FENCE_H_
#define RENDERING_CORE_FENCE_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>

namespace Rendering {

class Fence : public Util::ReferenceCounter<Fence> {
public:
	using Ref = Util::Reference<Fence>;
	static Ref create();
	~Fence();
	Fence(Fence&& o) = default;
	Fence(const Fence& o) = delete;

private:
	Fence();
	bool init();
	
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_FENCE_H_ */