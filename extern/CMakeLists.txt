#
# This file is part of the Rendering library.
# Copyright (C) 2018-2022 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

add_library(RenderingExtern INTERFACE)
include(FetchContent)

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
# DDS/KTX Reader

FetchContent_Declare(
  ddsktx
  GIT_REPOSITORY https://github.com/septag/dds-ktx.git
  GIT_TAG        v1.1
)

FetchContent_GetProperties(ddsktx)
if(NOT ddsktx_POPULATED)
  message(STATUS "Fetching dds-ktx...")
  FetchContent_Populate(ddsktx)
endif()
target_compile_definitions(RenderingExtern INTERFACE RENDERING_HAVE_LIB_DDS)
target_include_directories(RenderingExtern INTERFACE ${ddsktx_SOURCE_DIR})


# ------------------------------------------------------------------------------
# Vulkan-Headers

#FetchContent_Declare(
#  vulkan-headers
#  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
#  GIT_TAG        v1.3.207
#)
#
#FetchContent_GetProperties(vulkan-headers)
#if(NOT vulkan-headers_POPULATED)
#  message(STATUS "Fetching vulkan-headers...")
#  FetchContent_Populate(vulkan-headers)
#endif()
#set(Vulkan_INCLUDE_DIR ${vulkan-headers_SOURCE_DIR}/include CACHE STRING "" FORCE)
#target_include_directories(RenderingExtern INTERFACE ${Vulkan_INCLUDE_DIR})

# ------------------------------------------------------------------------------
# spirv-headers (required by spirv-tools)

#FetchContent_Declare(
#  spirv-headers
#  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
#  GIT_TAG        6a55fade62dec6a406a5a721148f88a2211cbefa # revision used by spirv-tools
#  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/spirv-headers"
#)
#
#FetchContent_GetProperties(spirv-headers)
#if(NOT spirv-headers_POPULATED)
#  message(STATUS "Fetching spirv-headers...")
#  set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
#  set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
#  FetchContent_Populate(spirv-headers)
#  #set(SPIRV-Headers_SOURCE_DIR "${spirv-headers_SOURCE_DIR}" CACHE STRING "" FORCE)
#  add_subdirectory(${spirv-headers_SOURCE_DIR} ${spirv-headers_BINARY_DIR})
#endif()
#target_link_libraries(RenderingExtern INTERFACE SPIRV-Headers)

# ------------------------------------------------------------------------------
# spirv-tools (used for optimizing spir-v shaders)

#FetchContent_Declare(
#  spirv-tools
#  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
#  GIT_TAG        v2022.1
#  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/spirv-tools"
#)
#
#FetchContent_GetProperties(spirv-tools)
#if(NOT spirv-tools_POPULATED)
#  message(STATUS "Fetching spirv-tools...")
#  FetchContent_Populate(spirv-tools)
#  set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
#  set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
#  set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
#  set(SPIRV_CHECK_CONTEXT OFF CACHE BOOL "" FORCE)
#  add_subdirectory(${spirv-tools_SOURCE_DIR} ${spirv-tools_BINARY_DIR})
#endif()
#target_link_libraries(RenderingExtern INTERFACE SPIRV-Tools-link)
#set_target_properties(SPIRV-Tools-link PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# glslang (for compiling shaders to spir-v)

FetchContent_Declare(
  glslang
  GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
  GIT_TAG        11.13.0
  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/glslang"
)

FetchContent_GetProperties(glslang)
if(NOT glslang_POPULATED)
  message(STATUS "Fetching glslang...")
  FetchContent_Populate(glslang)
  set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
  set(ENABLE_SPVREMAPPER ON CACHE BOOL "" FORCE)
  set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
  set(ENABLE_GLSLANG_JS OFF CACHE BOOL "" FORCE)
  add_subdirectory(${glslang_SOURCE_DIR} ${glslang_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
target_link_libraries(RenderingExtern INTERFACE SPIRV)

# ------------------------------------------------------------------------------
# SPIRV-Reflect (for shader reflection)

FetchContent_Declare(
  spirv-reflect
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
  GIT_TAG        sdk-1.3.236.0
  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/spirv-reflect"
)

FetchContent_GetProperties(spirv-reflect)
if(NOT spirv-reflect_POPULATED)
  message(STATUS "Fetching spirv-reflect...")
  FetchContent_Populate(spirv-reflect)
  set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_STRIPPER OFF CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_ENABLE_ASSERTS OFF CACHE BOOL "" FORCE)
  add_subdirectory(${spirv-reflect_SOURCE_DIR} ${spirv-reflect_BINARY_DIR})
endif()
target_link_libraries(RenderingExtern INTERFACE spirv-reflect-static)
set_target_properties(spirv-reflect-static PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# spirv_cross (for shader reflection)

#FetchContent_Declare(
#  spirv-cross
#  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
#  GIT_TAG        2021-01-15
#  SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/spirv-cross"
#)
#
#FetchContent_GetProperties(spirv-cross)
#if(NOT spirv-cross_POPULATED)
#  message(STATUS "Fetching spirv-cross...")
#  FetchContent_Populate(spirv-cross)
#  set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
#  set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
#  set(SPIRV_CROSS_FORCE_STL_TYPES OFF CACHE BOOL "" FORCE)
#  set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
#  set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
#  set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
#  add_subdirectory(${spirv-cross_SOURCE_DIR} ${spirv-cross_BINARY_DIR})
#endif()
#target_link_libraries(RenderingExtern INTERFACE spirv-cross-core)
#set_target_properties(spirv-cross-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# nvrhi (NVIDIA Rendering Hardware Interface)

FetchContent_Declare(
  nvrhi
  GIT_REPOSITORY https://github.com/NVIDIAGameWorks/nvrhi.git
  GIT_TAG        198b2d082963e916f0a00fb3350b44a46a1d2bfa
)

FetchContent_GetProperties(nvrhi)
if(NOT nvrhi_POPULATED)
  message(STATUS "Fetching nvrhi...")
  FetchContent_Populate(nvrhi)
  set(NVRHI_INSTALL ON CACHE BOOL "" FORCE)
  set(NVRHI_WITH_SHADER_COMPILER ON CACHE BOOL "" FORCE)
  set(NVRHI_WITH_VULKAN ON CACHE BOOL "" FORCE)
  set(NVRHI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  add_subdirectory(${nvrhi_SOURCE_DIR} ${nvrhi_BINARY_DIR})
endif()
target_link_libraries(RenderingExtern INTERFACE nvrhi_vk)
target_link_libraries(Rendering PUBLIC nvrhi) # want to make nvrhi publicly available

# ------------------------------------------------------------------------------

set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS} CACHE INTERNAL "" FORCE)
set(CMAKE_BUILD_TYPE ${OLD_CMAKE_BUILD_TYPE} CACHE INTERNAL "" FORCE)