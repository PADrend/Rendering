/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_COMMANDS_COMMAND_H_
#define RENDERING_CORE_COMMANDS_COMMAND_H_

#include "../Common.h"

#include <Util/References.h>

#include <memory>

namespace Rendering {
class DescriptorPool;
using DescriptorPoolRef = Util::Reference<DescriptorPool>;
class ResourceCache;
using ResourceCacheRef = Util::Reference<ResourceCache>;
class Device;
using DeviceRef = Util::Reference<Device>;

struct CompileContext {
	DeviceRef device;
	ResourceCacheRef resourceCache;
	DescriptorPoolRef descriptorPool;
	CommandBufferHandle cmd;
};

class Command {
public:
	using Ptr = std::unique_ptr<Command>;
	virtual ~Command() = default;

	virtual bool compile(CompileContext& context) { return false; };
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMANDS_COMMAND_H_ */