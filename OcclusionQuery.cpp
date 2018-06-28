/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "OcclusionQuery.h"
#include "RenderingContext/RenderingParameters.h"
#include "RenderingContext/RenderingContext.h"
#include "GLHeader.h"
#include <cstdint>

namespace Rendering {

void OcclusionQuery::enableTestMode(RenderingContext & renderingContext) {
	renderingContext.pushAndSetColorBuffer(ColorBufferParameters(false, false, false, false));
	renderingContext.pushAndSetDepthBuffer(
		DepthBufferParameters(
			renderingContext.getDepthBufferParameters().isTestEnabled(),
			false,
			renderingContext.getDepthBufferParameters().getFunction()
		)
	);
	renderingContext.applyChanges();
}

void OcclusionQuery::disableTestMode(RenderingContext & renderingContext) {
	renderingContext.popDepthBuffer();
	renderingContext.popColorBuffer();
}


OcclusionQuery::OcclusionQuery() :
	QueryObject(static_cast<uint32_t>(GL_ANY_SAMPLES_PASSED)) {
}

}
