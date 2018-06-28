#
# This file is part of the Rendering library.
# Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
cmake_minimum_required(VERSION 2.8.11)
project(Rendering)

set(RENDERING_VERSION_MAJOR 0)
set(RENDERING_VERSION_MINOR 2)
set(RENDERING_VERSION_PATCH 0)
set(RENDERING_VERSION_STRING ${RENDERING_VERSION_MAJOR}.${RENDERING_VERSION_MINOR}.${RENDERING_VERSION_PATCH})

# Set up install directories
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKECONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Rendering)

add_library(Rendering
	CL/Event.cpp
	CL/CLUtils.cpp
	CL/CommandQueue.cpp
	CL/Context.cpp
	CL/Device.cpp
	CL/Kernel.cpp
	CL/Platform.cpp
	CL/Program.cpp
	CL/UserEvent.cpp
	CL/Memory/Buffer.cpp
	CL/Memory/BufferAccessor.cpp
	CL/Memory/Image.cpp
	CL/Memory/Memory.cpp
	CL/Memory/Sampler.cpp
	Mesh/Mesh.cpp
	Mesh/MeshDataStrategy.cpp
	Mesh/MeshIndexData.cpp
	Mesh/MeshVertexData.cpp
	Mesh/VertexAttribute.cpp
	Mesh/VertexAttributeAccessors.cpp
	Mesh/VertexAttributeIds.cpp
	Mesh/VertexDescription.cpp
	MeshUtils/LocalMeshDataHolder.cpp
	MeshUtils/MarchingCubesMeshBuilder.cpp
	MeshUtils/MeshBuilder.cpp
	MeshUtils/MeshUtils.cpp
	MeshUtils/PlatonicSolids.cpp
	MeshUtils/QuadtreeMeshBuilder.cpp
	MeshUtils/QuadtreeMeshBuilderDebug.cpp
	MeshUtils/Simplification.cpp
	MeshUtils/StreamMeshDataStrategy.cpp
	MeshUtils/TriangleAccessor.cpp
	MeshUtils/ConnectivityAccessor.cpp
	RenderingContext/internal/PipelineState.cpp
	RenderingContext/internal/StatusHandler_UBO.cpp
	RenderingContext/RenderingContext.cpp
	RenderingContext/RenderingParameters.cpp
	Serialization/GenericAttributeSerialization.cpp
	Serialization/Serialization.cpp
	Serialization/StreamerMD2.cpp
	Serialization/StreamerMMF.cpp
	Serialization/StreamerMTL.cpp
	Serialization/StreamerMVBO.cpp
	Serialization/StreamerNGC.cpp
	Serialization/StreamerOBJ.cpp
	Serialization/StreamerPKM.cpp
	Serialization/StreamerPLY.cpp
	Serialization/StreamerXYZ.cpp
	Shader/Shader.cpp
	Shader/ShaderObjectInfo.cpp
	Shader/ShaderUtils.cpp
	Shader/Uniform.cpp
	Shader/UniformRegistry.cpp
	Texture/Texture.cpp
	Texture/TextureUtils.cpp
	BufferObject.cpp
	Draw.cpp
	DrawCompound.cpp
	FBO.cpp
	Helper.cpp
	OcclusionQuery.cpp
	PBO.cpp
	QueryObject.cpp
	StatisticsQuery.cpp
	TextRenderer.cpp
	VAO.cpp
)

# Dependency to Geometry
if(NOT TARGET Geometry)
	find_package(Geometry 0.2.0 REQUIRED NO_MODULE)
endif()
target_link_libraries(Rendering LINK_PUBLIC Geometry)

# Dependency to Util
if(NOT TARGET Util)
	find_package(Util 0.2.0 REQUIRED NO_MODULE)
endif()
target_link_libraries(Rendering LINK_PUBLIC Util)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Dependency to an OpenGL implementation
find_package(GLImplementation REQUIRED)
target_compile_definitions(Rendering PRIVATE "${GLIMPLEMENTATION_DEFINITIONS}")
target_include_directories(Rendering PRIVATE ${GLIMPLEMENTATION_INCLUDE_DIRS})
target_link_libraries(Rendering LINK_PRIVATE ${GLIMPLEMENTATION_LIBRARIES})

# Dependency to OpenCL
find_package(OpenCL QUIET)
if(OPENCL_FOUND)
	if( OPENCL_HAS_CPP_BINDINGS )
		target_include_directories(Rendering PRIVATE ${OPENCL_INCLUDE_DIRS})
		target_compile_definitions(Rendering PUBLIC RENDERING_HAS_LIB_OPENCL)
		target_link_libraries(Rendering LINK_PRIVATE ${OPENCL_LIBRARIES})
	else( OPENCL_HAS_CPP_BINDINGS )
	    message(SEND_ERROR "No OpenCL CPP bindings found" )
	endif( OPENCL_HAS_CPP_BINDINGS )
endif()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set_property(TARGET Rendering APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++11 ")
elseif(COMPILER_SUPPORTS_CXX0X)
	set_property(TARGET Rendering APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++0x ")
else()
	message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Set version of library
set_target_properties(Rendering PROPERTIES VERSION ${RENDERING_VERSION_STRING}
                                           SOVERSION ${RENDERING_VERSION_MAJOR})
                                           
add_subdirectory(tests)

# Install the header files
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/CL/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/CL COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/CL/Memory/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/CL/Memory COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Mesh/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/Mesh COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/MeshUtils/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/MeshUtils COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/RenderingContext/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/RenderingContext COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Serialization/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/Serialization COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Shader/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/Shader COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Texture/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/Texture COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering COMPONENT headers)

# Install the library
install(TARGETS Rendering EXPORT LibraryExport
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtimelibraries
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtimelibraries
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT developmentlibraries
)

# Set include directories for users of this library
target_include_directories(Rendering
                           INTERFACE
                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
                           "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")

# Library export
install(EXPORT LibraryExport DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR} FILE RenderingTargets.cmake COMPONENT developmentlibraries)

# Create the configuration files
include(CMakePackageConfigHelpers)
configure_package_config_file(RenderingConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RenderingConfig.cmake
                              INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR}
                              PATH_VARS CMAKE_INSTALL_CMAKECONFIGDIR
                              NO_SET_AND_CHECK_MACRO
                              NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/RenderingConfigVersion.cmake
                                 VERSION ${RENDERING_VERSION_STRING}
                                 COMPATIBILITY SameMajorVersion)

# Install the configuration files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RenderingConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/RenderingConfigVersion.cmake
		DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR} COMPONENT developmentlibraries)

# Packaging
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "librendering${RENDERING_VERSION_MAJOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Rendering library")
set(CPACK_PACKAGE_VENDOR "Benjamin Eikel; Claudius Jaehn; Ralf Petring")
set(CPACK_PACKAGE_CONTACT "Benjamin Eikel <benjamin@eikel.org>")
set(CPACK_PACKAGE_VERSION_MAJOR ${RENDERING_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${RENDERING_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${RENDERING_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "libRendering")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES ON)

include(CPack)
