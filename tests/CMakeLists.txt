#
# This file is part of the Rendering library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
cmake_minimum_required(VERSION 2.8.11)

option(RENDERING_BUILD_TESTS "Defines if CppUnit tests for the Rendering library are built.")
if(RENDERING_BUILD_TESTS)
	add_executable(RenderingTest 
		BufferObjectTest.cpp
		OpenCLTest.cpp
		DrawTest.cpp
		RenderingTestMain.cpp
	)

	target_link_libraries(RenderingTest LINK_PRIVATE Rendering)

# Dependency to OpenCL
find_package(OpenCL QUIET)
if(OPENCL_FOUND)
	target_link_libraries(RenderingTest LINK_PRIVATE ${OPENCL_LIBRARIES})
endif()

	# Dependency to CppUnit
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)
	find_package(CppUnit REQUIRED)
	target_include_directories(RenderingTest PRIVATE ${CPPUNIT_INCLUDE_DIRS})
	target_link_libraries(RenderingTest LINK_PRIVATE ${CPPUNIT_LIBRARIES})

	if(COMPILER_SUPPORTS_CXX11)
		set_property(TARGET RenderingTest APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++11 ")
	elseif(COMPILER_SUPPORTS_CXX0X)
		set_property(TARGET RenderingTest APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++0x ")
	endif()

	install(TARGETS RenderingTest
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT tests
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tests
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tests
	)

	enable_testing()
	add_test(NAME RenderingUnitTests COMMAND RenderingTest)
endif()
