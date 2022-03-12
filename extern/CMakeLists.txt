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
# nvrhi (NVIDIA Rendering Hardware Interface)

FetchContent_Declare(
  nvrhi
  GIT_REPOSITORY https://github.com/NVIDIAGameWorks/nvrhi.git
  GIT_TAG        12f3e12eb29b681e99a53af7347c60c0733a6ef9
)

FetchContent_GetProperties(nvrhi)
if(NOT nvrhi_POPULATED)
  message(STATUS "Fetching nvrhi...")
  FetchContent_Populate(nvrhi)
  add_subdirectory(${nvrhi_SOURCE_DIR} ${nvrhi_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
target_link_libraries(RenderingExtern INTERFACE nvrhi)

# ------------------------------------------------------------------------------
# OpenGL & glew

option(RENDERING_BACKEND_GLES "Defines wether GLESv2 is used as rendering backend." OFF)

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW) # prefer GLVND over legacy GL libraries
endif()

if(RENDERING_BACKEND_GLES)
  find_package(GLESv2 REQUIRED)
  target_compile_definitions(RenderingExtern INTERFACE "LIB_GLESv2")
  target_include_directories(RenderingExtern INTERFACE ${GLESV2_INCLUDE_DIRS})
  target_link_libraries(RenderingExtern INTERFACE ${GLESV2_LIBRARIES})
  set(GLIMPLEMENTATION_FOUND ${GLESV2_FOUND})
else()
  find_package(OpenGL REQUIRED)
  target_compile_definitions(RenderingExtern INTERFACE "LIB_GL")
  target_include_directories(RenderingExtern INTERFACE ${OPENGL_INCLUDE_DIR})
  target_link_libraries(RenderingExtern INTERFACE ${OPENGL_LIBRARIES})
  set(GLIMPLEMENTATION_FOUND ${OPENGL_FOUND})
endif()

if(GLIMPLEMENTATION_FOUND)  
  FetchContent_Declare(
    glew
    URL http://downloads.sourceforge.net/project/glew/glew/2.2.0/glew-2.2.0.tgz
  	URL_MD5 "3579164bccaef09e36c0af7f4fd5c7c7"
  )

  FetchContent_GetProperties(glew)
  if(NOT glew_POPULATED)
    message(STATUS "Fetching glew...")
    FetchContent_Populate(glew)
    # hide options in cmake-gui
    set(BUILD_UTILS OFF CACHE INTERNAL "" FORCE)
    set(GLEW_REGAL OFF CACHE INTERNAL "" FORCE)
    set(GLEW_OSMESA OFF CACHE INTERNAL "" FORCE)
    set(BUILD_FRAMEWORK OFF CACHE INTERNAL "" FORCE)
    add_subdirectory(${glew_SOURCE_DIR}/build/cmake ${glew_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  set_target_properties(glew_s PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(RenderingExtern INTERFACE LIB_GLEW GLEW_NO_GLU)
  target_link_libraries(RenderingExtern INTERFACE glew_s)
  target_include_directories(RenderingExtern INTERFACE ${glew_SOURCE_DIR}/include)
endif()
