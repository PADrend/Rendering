/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDBUFFER_H_
#define RENDERING_CORE_COMMANDBUFFER_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>

#include <vector>
#include <deque>
#include <functional>

namespace Rendering {
class CommandPool;

//-------------------------------------------------------

class CommandBuffer : public Util::ReferenceCounter<CommandBuffer> {
public:
	using Ref = Util::Reference<CommandBuffer>;
	
	~CommandBuffer();
	
	bool ready();
	void reset();
	bool isPrimary() const { return primary; }
	const CommandBufferHandle& getApiHandle() const { return handle; };
private:
	friend class CommandPool;
	explicit CommandBuffer(CommandPool* pool, bool primary=true);
	bool init();

	Util::WeakPointer<CommandPool> pool;
	CommandBufferHandle handle;
	bool primary;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMANDBUFFER_H_ */
