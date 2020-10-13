/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_PIPELINE_H_
#define RENDERING_CORE_PIPELINE_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;

class Pipeline : public Util::ReferenceCounter<Pipeline> {
public:
	struct Configuration {

	};
	using Ref = Util::Reference<Pipeline>;
	static Ref create(const DeviceRef& device, const Configuration& config);
	Pipeline(Pipeline &&) = default;
	Pipeline(const Pipeline &) = delete;
	~Pipeline();

	const PipelineHandle& getApiHandle() const { return handle; }
private:
	Pipeline(const DeviceRef& device, const Configuration& config);
	bool init();
	
	DeviceRef device;
	Configuration config;
	PipelineHandle handle;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_PIPELINE_H_ */