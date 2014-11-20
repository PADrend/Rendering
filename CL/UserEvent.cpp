/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "UserEvent.h"

#include "Context.h"

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

UserEvent::UserEvent(Context* context) : Event(new cl::UserEvent(*context->_internal())) {
}

UserEvent::~UserEvent() {
	setStatus(CL_COMPLETE);
}

UserEvent::UserEvent(const UserEvent& event) : Event(new cl::UserEvent(*static_cast<cl::UserEvent*>(event.event.get()))) { }
UserEvent::UserEvent(UserEvent&& event) = default;
UserEvent& UserEvent::operator=(UserEvent&&) = default;

void UserEvent::setStatus(int32_t status) {
	if(status > 0)
		status = 0;
	static_cast<cl::UserEvent*>(event.get())->setStatus(status);
}

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_HAS_LIB_OPENCL */
