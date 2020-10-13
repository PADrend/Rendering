#
# This file is part of the Rendering library.
# Copyright (C) 2018-2020 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

add_library(RenderingExtern INTERFACE)

if(${CMAKE_VERSION} VERSION_LESS "3.11.0") 
  message("Please consider to switch to CMake 3.11.0")
  include(FetchContentFallback)
else()
  include(FetchContent)
endif()

if(POLICY CMP0048)
  set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
endif()

# store state of BUILD_SHARED_LIBS to later restore
set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# store state of CMAKE_BUILD_TYPE to later restore
set(OLD_CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)

# Dependency to pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
if(TARGET Threads::Threads)
	target_link_libraries(RenderingExtern INTERFACE ${CMAKE_THREAD_LIBS_INIT})
endif()

# ------------------------------------------------------------------------------
# Vulkan-Headers

FetchContent_Declare(
  vulkan-headers
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
  GIT_TAG        v1.1.130
)

FetchContent_GetProperties(vulkan-headers)
if(NOT vulkan-headers_POPULATED)
  message(STATUS "Fetching vulkan-headers...")
  FetchContent_Populate(vulkan-headers)
endif()
set(Vulkan_INCLUDE_DIR ${vulkan-headers_SOURCE_DIR}/include CACHE STRING "" FORCE)

# ------------------------------------------------------------------------------
# Vulkan


find_package(Vulkan)
if(Vulkan_FOUND)
  target_include_directories(RenderingExtern INTERFACE ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(RenderingExtern INTERFACE ${Vulkan_LIBRARIES})
endif()

# ------------------------------------------------------------------------------
# spirv-headers (required by shaderc)

FetchContent_Declare(
  spirv-headers
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
  GIT_TAG        c4f8f65792d4bf2657ca751904c511bbcf2ac77b
  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/spirv-headers"
)

FetchContent_GetProperties(spirv-headers)
if(NOT spirv-headers_POPULATED)
  message(STATUS "Fetching spirv-headers...")
  FetchContent_Populate(spirv-headers)
  set(SPIRV-Headers_SOURCE_DIR "${spirv-headers_SOURCE_DIR}" CACHE STRING "" FORCE)
  set(SHADERC_SPIRV_HEADERS_DIR "${spirv-headers_SOURCE_DIR}" CACHE STRING "" FORCE)
endif()

# ------------------------------------------------------------------------------
# spirv-tools (required by shaderc)

FetchContent_Declare(
  spirv-tools
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
  GIT_TAG        0125b28ed4214d1860696f22d230dbfc965c6c2c
  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/spirv-tools"
)

FetchContent_GetProperties(spirv-tools)
if(NOT spirv-tools_POPULATED)
  message(STATUS "Fetching spirv-tools...")
  FetchContent_Populate(spirv-tools)
  set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
  set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
  set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
  #add_subdirectory(${spirv-tools_SOURCE_DIR} ${spirv-tools_BINARY_DIR})
endif()
#target_link_libraries(RenderingExtern INTERFACE SPIRV-Tools-link)
#set_target_properties(SPIRV-Tools-link PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# glslang (required by shaderc)

FetchContent_Declare(
  glslang
  GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
  GIT_TAG        f88e5824d2cfca5edc58c7c2101ec9a4ec36afac
  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/glslang"
)

FetchContent_GetProperties(glslang)
if(NOT glslang_POPULATED)
  message(STATUS "Fetching glslang...")
  FetchContent_Populate(glslang)
endif()

# ------------------------------------------------------------------------------
# spirv_cross (required by shaderc_spvc)

FetchContent_Declare(
  spirv-cross
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
  GIT_TAG        fce83b7e8b0f6599efd4481992b2eb30f69f21de
  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/spirv-cross"
)

FetchContent_GetProperties(spirv-cross)
if(NOT spirv-cross_POPULATED)
  message(STATUS "Fetching spirv-cross...")
  FetchContent_Populate(spirv-cross)
  set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_FORCE_STL_TYPES OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
  add_subdirectory(${spirv-cross_SOURCE_DIR} ${spirv-cross_BINARY_DIR})
endif()
target_link_libraries(RenderingExtern INTERFACE spirv-cross-core)
set_target_properties(spirv-cross-core PROPERTIES POSITION_INDEPENDENT_CODE ON)


# ------------------------------------------------------------------------------
# shaderc (glsl compiler required by vulkan)

FetchContent_Declare(
  shaderc
  GIT_REPOSITORY https://github.com/google/shaderc.git
  GIT_TAG        v2019.0
)

FetchContent_GetProperties(shaderc)
if(NOT shaderc_POPULATED)
  message(STATUS "Fetching shaderc...")
  FetchContent_Populate(shaderc)
  #set(SHADERC_ENABLE_SPVC ON CACHE BOOL "" FORCE)
  set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
  set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
  set(SHADERC_THIRD_PARTY_ROOT_DIR "${FETCHCONTENT_BASE_DIR}" CACHE STRING "" FORCE)
  add_subdirectory(${shaderc_SOURCE_DIR} ${shaderc_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
target_link_libraries(RenderingExtern INTERFACE shaderc_shared)
#target_link_libraries(RenderingExtern INTERFACE shaderc_spvc)
#set_target_properties(shaderc PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# Vulkan memory allocator

FetchContent_Declare(
  vma
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        v2.3.0
)

FetchContent_GetProperties(vma)
if(NOT vma_POPULATED)
  message(STATUS "Fetching vma...")
  FetchContent_Populate(vma)
endif()
target_include_directories(RenderingExtern INTERFACE ${vma_SOURCE_DIR}/src)

# ------------------------------------------------------------------------------

set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS} CACHE INTERNAL "" FORCE)
set(CMAKE_BUILD_TYPE ${OLD_CMAKE_BUILD_TYPE} CACHE INTERNAL "" FORCE)