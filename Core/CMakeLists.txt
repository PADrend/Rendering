#
# This file is part of the MinSG library.
# Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

target_sources(Rendering PRIVATE
	Core/internal/VkFramebuffer.cpp
	Core/internal/VkPipeline.cpp
	Core/internal/VkUtils.cpp
	Core/ApiHandles.cpp
	Core/BufferStorage.cpp
	Core/CommandBuffer.cpp
	Core/Common.cpp
	Core/DescriptorPool.cpp
	Core/Device.cpp
	Core/Fence.cpp
	Core/ImageStorage.cpp
	Core/ImageView.cpp
	Core/QueryPool.cpp
	Core/Queue.cpp
	Core/ResourceCache.cpp
	Core/Sampler.cpp
	Core/Swapchain.cpp
)

add_subdirectory(Commands)

# Install the header files
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/Core COMPONENT headers)