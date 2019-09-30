#
# This file is part of the Rendering library.
# Copyright (C) 2018-2019 Sascha Brandt <sascha@brandt.graphics>
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

# ------------------------------------------------------------------------------
# glew

find_package(OpenGL QUIET)
if(OPENGL_FOUND)
  FetchContent_Declare(
    glew
    URL http://downloads.sourceforge.net/project/glew/glew/2.1.0/glew-2.1.0.tgz
  	URL_MD5 "b2ab12331033ddfaa50dc39345343980"
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
  target_compile_definitions(RenderingExtern INTERFACE LIB_GLEW)
  target_link_libraries(RenderingExtern INTERFACE glew)
  target_include_directories(RenderingExtern INTERFACE ${glew_SOURCE_DIR})
endif()
