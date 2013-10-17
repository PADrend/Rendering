# Try to find GLESv2. Once done, this will define:
#
#   GLESV2_FOUND - variable which returns the result of the search
#   GLESV2_INCLUDE_DIRS - list of include directories
#   GLESV2_LIBRARIES - options for the linker

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

find_package(PkgConfig)
pkg_check_modules(PC_GLESV2 QUIET glesv2)

find_path(GLESV2_INCLUDE_DIR
	GLES2/gl2.h
	HINTS ${PC_GLESV2_INCLUDEDIR} ${PC_GLESV2_INCLUDE_DIRS}
)
find_library(GLESV2_LIBRARY
	GLESv2
	HINTS ${PC_GLESV2_LIBDIR} ${PC_GLESV2_LIBRARY_DIRS}
)

set(GLESV2_INCLUDE_DIRS ${GLESV2_INCLUDE_DIR})
set(GLESV2_LIBRARIES ${GLESV2_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLESv2 DEFAULT_MSG
	GLESV2_INCLUDE_DIR
	GLESV2_LIBRARY
)

mark_as_advanced(
	GLESV2_INCLUDE_DIR
	GLESV2_LIBRARY
)
