/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_SAMPLER_H_
#define RENDERING_CORE_SAMPLER_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;

class Sampler : public Util::ReferenceCounter<Sampler> {
public:

	using Ref = Util::Reference<Sampler>;
	struct Configuration {
		ImageFilter magFilter = ImageFilter::Linear; //! value specifying the magnification filter to apply to lookups.
		ImageFilter minFilter = ImageFilter::Linear; //! value specifying the minification filter to apply to lookups.
		ImageFilter mipmapMode = ImageFilter::Linear; //! value specifying the mipmap filter to apply to lookups.
		ImageAddressMode addressModeU = ImageAddressMode::Repeat; //! value specifying the addressing mode for outside [0..1] range for U coordinate.
		ImageAddressMode addressModeV = ImageAddressMode::Repeat; //! value specifying the addressing mode for outside [0..1] range for V coordinate.
		ImageAddressMode addressModeW = ImageAddressMode::Repeat; //! value specifying the addressing mode for outside [0..1] range for W coordinate.
		float minLod = -1000.0f; //! value used to clamp the computed LOD value.
		float maxLod = 1000.0f; //! value used to clamp the computed LOD value.
		float mipLodBias = 0.0f; //! the bias to be added to mipmap LOD (level-of-detail) calculation.
		uint32_t maxAnisotropy = 1; //! the anisotropy value clamp used by the sampler when @p anisotropyEnable is @p true.
		ComparisonFunc compareOp = ComparisonFunc::Disabled; //! value specifying the comparison function to apply to fetched data before filtering.
	};

	static Ref create(const DeviceRef& device, const Configuration& config);
	~Sampler() = default;
	Sampler(Sampler&& o) = default;
	Sampler(const Sampler& o) = delete;
	Sampler& operator=(Sampler&& o) = default;
	Sampler& operator=(const Sampler& o) = default;

	const Configuration& getConfig() const { return config; }
	const SamplerHandle& getApiHandle() const { return handle; }
private:
	Sampler(const DeviceRef& device, const Configuration& config);
	bool init();
	DeviceRef device;
	Configuration config;
	SamplerHandle handle;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_SAMPLER_H_ */