/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_IMAGESTORAGE_H_
#define RENDERING_CORE_IMAGESTORAGE_H_

#include "Common.h"

#include "../Texture/TextureType.h"

#include <Util/ReferenceCounter.h>


namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;

/**
 * @brief Represents a multidimensional (up to 3) array of formatted data allocated on a device.
 */
class ImageStorage : public Util::ReferenceCounter<ImageStorage> {
public:
	struct Configuration {
		ImageFormat format; //! internal format of the image storage.
		MemoryUsage access = MemoryUsage::CpuToGpu; //! memory access flag
		ResourceUsage usage = ResourceUsage::General; //! usage flags
	};

	using Ref = Util::Reference<ImageStorage>;
	ImageStorage(const ImageStorage&) = delete;
	ImageStorage(ImageStorage&&) = default;
	~ImageStorage() = default;

	/**
	 * @brief Creates and allocates a new image storage.
	 * @param device The device the image is allocated on
	 * @param config The configuration parameters of the image
	 * @return Smart reference to the image storage
	 */
	static Ref create(const DeviceRef& device, const Configuration& config);

	/**
	 * @brief Creates a image storage from an existing image handle.
	 * @param device The device the image is allocated on
	 * @param config The configuration parameters of the image. It has to match the parameters of the image handle (this will not be checked!).
	 * @param handle The image handle. The created image storage takes ownership of the handle.
	 * @return Smart reference to the image storage
	 */
	static Ref createFromHandle(const DeviceRef& device, const Configuration& config, ImageHandle&& handle);

	/**
	 * @return The data size of the image
	 */
	size_t getSize() const { return dataSize; }
	
	/**
	 * @return The format of the image
	 */
	const ImageFormat& getFormat() const { return config.format; };
	
	/**
	 * @return The type of the image (1D, 2D, 3D)
	 */
	TextureType getType() const { return type; }

	/**
	 * @return The configuration the image was created with
	 */
	const Configuration& getConfig() const { return config; }

	//! @name Internal
	//! @{	
	ResourceUsage getLastUsage() const { return lastUsage; }
	void _setLastUsage(ResourceUsage usage) { lastUsage = usage; }

	const ImageHandle& getApiHandle() const { return handle; }
	const AllocationHandle& getAllocation() const { return allocation; }
	//! @}
private:
	explicit ImageStorage(const DeviceRef& device, const Configuration& config);
	bool init();

	const DeviceRef device;
	const Configuration config;
	const TextureType type;
	ImageHandle handle;
	AllocationHandle allocation;
	size_t dataSize = 0;
	ResourceUsage lastUsage = ResourceUsage::Undefined;
};
} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_IMAGESTORAGE_H_ */