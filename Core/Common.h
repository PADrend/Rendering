/*
  This file is part of the Rendering library.
  Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
  
  This library is subject to the terms of the Mozilla Public License, v. 2.0.
  You should have received a copy of the MPL along with this library; see the
  file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_COMMON_H_
#define RENDERING_CORE_COMMON_H_

#include "ApiHandles.h"

namespace Rendering {

enum MemoryUsage {
	Unknown, //! No intended memory usage specified.
	CpuOnly, //! Memory will be mappable on host.
	GpuOnly, //! Memory will be used on device only.
	CpuToGpu, //! Memory that is both mappable on host and preferably fast to access by GPU.
	GpuToCpu //! Memory mappable on host and cached.
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMON_H_ */
