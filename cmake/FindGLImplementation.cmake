# Try to find OpenGL or OpenGL ES 2.x. Once done, this will define:
#
#   GLIMPLEMENTATION_FOUND - variable which returns the result of the search
#   GLIMPLEMENTATION_INCLUDE_DIRS - list of include directories
#   GLIMPLEMENTATION_LIBRARIES - options for the linker
#   GLIMPLEMENTATION_DEFINITIONS - definitions for the preprocessor

#=============================================================================
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

set(OpenGL_GL_PREFERENCE GLVND CACHE INTERNAL "" FORCE)
find_package(OpenGL QUIET)
find_package(GLESv2 QUIET)

# Do not use pkg-config here.

if(OPENGL_FOUND)
	set(GLIMPLEMENTATION_DEFINITIONS "LIB_GL")
	set(GLIMPLEMENTATION_INCLUDE_DIR ${OPENGL_INCLUDE_DIR})
	set(GLIMPLEMENTATION_LIBRARY ${OPENGL_LIBRARIES})
elseif(GLESV2_FOUND)
	set(GLIMPLEMENTATION_DEFINITIONS "LIB_GLESv2")
	set(GLIMPLEMENTATION_INCLUDE_DIR ${GLESV2_INCLUDE_DIRS})
	set(GLIMPLEMENTATION_LIBRARY ${GLESV2_LIBRARIES})
else()
	message(FATAL_ERROR "Neither OpenGL nor OpenGL ES 2.x were found.")
endif()

set(GLIMPLEMENTATION_INCLUDE_DIRS ${GLIMPLEMENTATION_INCLUDE_DIR})
set(GLIMPLEMENTATION_LIBRARIES ${GLIMPLEMENTATION_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLImplementation DEFAULT_MSG
	GLIMPLEMENTATION_INCLUDE_DIR
	GLIMPLEMENTATION_LIBRARY
)

mark_as_advanced(
	GLIMPLEMENTATION_INCLUDE_DIR
	GLIMPLEMENTATION_LIBRARY
)
