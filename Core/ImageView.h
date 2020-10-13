/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_IMAGEVIEW_H_
#define RENDERING_CORE_IMAGEVIEW_H_

#include "Common.h"
#include "../Texture/TextureType.h"

#include <Util/ReferenceCounter.h>

#include <Geometry/Vec3.h>

namespace Rendering {
class ImageStorage;
using ImageStorageRef = Util::Reference<ImageStorage>;

class ImageView : public Util::ReferenceCounter<ImageView> {
public:
	struct Configuration {
		TextureType type;
		uint32_t baseMipLevel = 0;
		uint32_t mipLevelCount = 1;
		uint32_t baseLayer = 0;
		uint32_t layerCount = 1;
	};
	using Ref = Util::Reference<ImageView>;
	static Ref create(const ImageStorageRef& image, const Configuration& config);
	ImageView(ImageView &&) = default;
	ImageView(const ImageView &) = delete;
	~ImageView();

	const ImageStorageRef& getImage() const { return image; }
	const Configuration& getConfig() const { return config; }
	TextureType getType() const { return config.type; }
	uint32_t getMipLevel() const { return config.baseMipLevel; }
	uint32_t getMipLevelCount() const { return config.mipLevelCount; }
	uint32_t getLayer() const { return config.baseLayer; }
	uint32_t getLayerCount() const { return config.layerCount; }

	const ImageViewHandle& getApiHandle() const { return handle; }
private:
	ImageView(const ImageStorageRef& image, const Configuration& config);
	bool init();
	
	const ImageStorageRef image;
	const Configuration config;
	ImageViewHandle handle;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_IMAGEVIEW_H_ */