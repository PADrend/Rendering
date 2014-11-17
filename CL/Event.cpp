/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Event.h"

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

Event::Event() : event(new cl::Event()) {}

void Event::wait() {
	event->wait();
}

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_HAS_LIB_OPENCL */
