/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Pipeline.h"
#include "Device.h"

#include <Util/Macros.h>

namespace Rendering {

//---------------

Pipeline::Ref Pipeline::create(const DeviceRef& device, const Configuration& config) {
	Ref obj = new Pipeline(device, config);
	if(!obj->init()) {
		WARN("Pipeline: Failed to create pipeline.");
		return nullptr;
	}
	return obj;
}

//---------------

Pipeline::~Pipeline() = default;

//---------------

Pipeline::Pipeline(const DeviceRef& device, const Configuration& config) : device(device), config(config) { }

//---------------

bool Pipeline::init() {
	
	return true;
}

//---------------

} /* Rendering */