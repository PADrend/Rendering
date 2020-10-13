/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Fence.h"

namespace Rendering {

//---------------

Fence::Ref Fence::create() {
	Ref obj = new Fence();
	if(!obj->init()) {
		return nullptr;
	}
	return obj;
}

//---------------

Fence::~Fence() = default;

//---------------

Fence::Fence() { }

//---------------

bool Fence::init() {
	
	return true;
}

//---------------

} /* Rendering */