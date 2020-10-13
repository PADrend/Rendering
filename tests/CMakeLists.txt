#
# This file is part of the Rendering library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
# Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
cmake_minimum_required(VERSION 3.1.0)
project(RenderingTest)

option(RENDERING_BUILD_TESTS "Defines if CppUnit tests for the Rendering library are built.")
if(RENDERING_BUILD_TESTS)
	add_executable(RenderingTest 
		BufferObjectTest.cpp
		DeviceTest.cpp
		FramebufferTest.cpp
		DrawTest.cpp
		RenderingContextTest.cpp
		RenderingTestMain.cpp
		ShaderTest.cpp
		#StatisticsQueryTest.cpp
		#VertexAccessorTest.cpp
	)

	target_link_libraries(RenderingTest LINK_PRIVATE Rendering)
	set_target_properties(RenderingTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)
	
	target_link_libraries(RenderingTest PRIVATE RenderingExtern)
	
	# ------------------------------------------------------------------------------
	# Catch2
	if(${CMAKE_VERSION} VERSION_LESS "3.11.0") 
		include(FetchContentFallback)
	else()
		include(FetchContent)
	endif()
	
	FetchContent_Declare(
		catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG        v2.9.1
	)

	FetchContent_GetProperties(catch2)
	if(NOT catch2_POPULATED)
		message(STATUS "Fetching Catch2...")
		FetchContent_Populate(catch2)
		# hide options in cmake-gui
		set(CATCH_USE_VALGRIND OFF CACHE INTERNAL "" FORCE)
		set(CATCH_BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
		set(CATCH_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
		set(CATCH_BUILD_EXTRA_TESTS OFF CACHE INTERNAL "" FORCE)
		set(CATCH_ENABLE_COVERAGE OFF CACHE INTERNAL "" FORCE)
		set(CATCH_ENABLE_WERROR ON CACHE INTERNAL "" FORCE)
		set(CATCH_INSTALL_DOCS OFF CACHE INTERNAL "" FORCE)
		set(CATCH_INSTALL_HELPERS OFF CACHE INTERNAL "" FORCE)
		add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR} EXCLUDE_FROM_ALL)
	endif()
	target_link_libraries(RenderingTest PRIVATE Catch2::Catch2)	
	
	# ------------------------------------------------------------------------------
	
	install(TARGETS RenderingTest
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT tests
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tests
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tests
	)

	enable_testing()
	add_test(NAME BufferObjectTest COMMAND RenderingTest [BufferObjectTest])
	add_test(NAME DrawTest COMMAND RenderingTest [DrawTest])
	add_test(NAME StatisticsQueryTest COMMAND RenderingTest [StatisticsQueryTest])
	add_test(NAME VertexAccessorTest COMMAND RenderingTest [VertexAccessorTest])
endif()
