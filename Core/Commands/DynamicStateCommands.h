/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDS_DYNAMICSTATECOMMANDS_H_
#define RENDERING_CORE_COMMANDS_DYNAMICSTATECOMMANDS_H_

#include "Command.h"
#include "../../State/PipelineState.h"

namespace Rendering {

//------------------------------------------

class DynamicScissorCommand : public Command {
public:
	DynamicScissorCommand(const std::vector<Geometry::Rect_i>& scissors, uint32_t firstScissor=0) : firstScissor(firstScissor), scissors(scissors) {}
	~DynamicScissorCommand() = default;
	bool compile(CompileContext& context) override;
private:
	uint32_t firstScissor;
	std::vector<Geometry::Rect_i> scissors;
};

//------------------------------------------

} /* Rendering */
#endif /* end of include guard: RENDERING_CORE_COMMANDS_DYNAMICSTATECOMMANDS_H_ */
