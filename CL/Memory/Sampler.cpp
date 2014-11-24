/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "Sampler.h"

#include "../CLUtils.h"
#include "../Context.h"

#include <Util/Macros.h>

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

Sampler::Sampler() = default;

Sampler::Sampler(Context* context, bool normalizedCoords, AdressingMode_r addressingMode, FilterMode_t filterMode) : context(context){
	cl_addressing_mode aMode;
	switch (addressingMode) {
		case ClampToEdge: aMode = CL_ADDRESS_CLAMP_TO_EDGE; break;
		case Clamp: aMode = CL_ADDRESS_CLAMP; break;
		case Repeat: aMode = CL_ADDRESS_REPEAT; break;
		case MirroredRepeat: aMode = CL_ADDRESS_MIRRORED_REPEAT; break;
		default: aMode = CL_ADDRESS_NONE; break;
	}
	cl_filter_mode fMode;
	switch (addressingMode) {
		case Linear: fMode = CL_FILTER_NEAREST; break;
		default: fMode = CL_FILTER_NEAREST; break;
	}
	cl_int err;
	sampler.reset(new cl::Sampler(*context->_internal(), normalizedCoords ? CL_TRUE : CL_FALSE, aMode, fMode));
	if(err != CL_SUCCESS) {
		WARN("Could not create sampler (" + getErrorString(err) + ").");
		FAIL();
	}
}

Sampler::Sampler(const Sampler& sampler) : context(sampler.context), sampler(new cl::Sampler(*sampler.sampler.get())) { }

//Sampler::Sampler(Sampler&& sampler) = default;

Sampler::~Sampler() = default;

//Sampler& Sampler::operator=(Sampler&&) = default;

Context* Sampler::getContext() const {
	return context.get();
}

Sampler::AdressingMode_r Sampler::getAdressingMode() const {
	switch (sampler->getInfo<CL_SAMPLER_FILTER_MODE>()) {
		case CL_ADDRESS_CLAMP_TO_EDGE:
			return ClampToEdge;
		case CL_ADDRESS_CLAMP:
			return Clamp;
		case CL_ADDRESS_REPEAT:
			return Repeat;
		case CL_ADDRESS_MIRRORED_REPEAT:
			return MirroredRepeat;
	}
	return None;
}

Sampler::FilterMode_t Sampler::getFilterMode() const {
	switch (sampler->getInfo<CL_SAMPLER_FILTER_MODE>()) {
		case CL_FILTER_NEAREST:
			return Nearest;
		case CL_FILTER_LINEAR:
			return Linear;
	}
}

bool Sampler::hasNormalizedCoords() const {
	return sampler->getInfo<CL_SAMPLER_NORMALIZED_COORDS>() == CL_TRUE;
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */
