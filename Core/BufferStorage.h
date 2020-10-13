/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_BUFFERSTORAGE_H_
#define RENDERING_CORE_BUFFERSTORAGE_H_

#include "Common.h"

#include <Util/ReferenceCounter.h>

#include <vector>
#include <memory>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;

/**
 * @brief Represents a linear array of unformatted data allocated on a device.
 */
class BufferStorage : public Util::ReferenceCounter<BufferStorage> {
public:
	struct Configuration {
		size_t size; //! size (in bytes) of the buffer storage.
		MemoryUsage access = MemoryUsage::CpuToGpu; //! memory access flag
		bool persistent = false; //! if true, the memory of the buffer is persistently mapped to CPU memory 
		ResourceUsage usage = ResourceUsage::General; //! usage flags

		bool operator==(const Configuration& o) const {
			return size == o.size && access == o.access && persistent == o.persistent && usage == o.usage;
		}
	};

	using Ref = Util::Reference<BufferStorage>;
	BufferStorage(const BufferStorage&) = delete;
	BufferStorage(BufferStorage&&) = default;
	~BufferStorage();

	/**
	 * @brief Creates and allocates a new buffer storage.
	 * @param device The device the buffer is allocated on
	 * @param config The configuration parameters of the buffer
	 * @return Smart reference to the buffer storage
	 */
	static Ref create(const DeviceRef& device, const Configuration& config);

	/**
	 * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
	 */
	void flush() const;

	/**
	 * @brief Maps memory if it isn't already mapped to an host visible address
	 * @return Pointer to host visible memory
	 */
	uint8_t* map();

	/**
	 * @brief Unmaps memory from the host visible address
	 */
	void unmap();
	
	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	void upload(const uint8_t* data, size_t size, size_t offset = 0);

	/**
	 * @brief Copies a vector of bytes into the buffer
	 * @param data The data vector to upload
	 * @param offset The offset to start the copying into the mapped data
	 */
	void upload(const std::vector<uint8_t> &data, size_t offset = 0) {
		upload(data.data(), data.size(), offset);
	}

	/**
	 * @return @p true, iff the buffer can be mapped to CPU memory
	 */
	bool isMappable() const { return config.access != MemoryUsage::GpuOnly && config.access != MemoryUsage::Unknown; };

	/**
	 * @return The size of the buffer
	 */
	size_t getSize() const { return config.size; };
	
	/**
	 * @return The configuration the buffer was created with
	 */
	const Configuration& getConfig() const { return config; }

	//! @name Internal
	//! @{
	const DeviceRef& getDevice() const { return device; }
	const BufferHandle& getApiHandle() const { return handle; }
	const AllocationHandle& getAllocation() const { return allocation; }
	//! @}

	//! @name Debugging
	//! @{
	void setDebugName(const std::string& name);
	//! @}
private:
	explicit BufferStorage(const DeviceRef& device, const Configuration& config);
	bool init();

	DeviceRef device;
	Configuration config;
	BufferHandle handle;
	AllocationHandle allocation;
	uint8_t* mappedPtr = nullptr;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_BUFFERSTORAGE_H_ */