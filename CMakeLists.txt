#
# This file is part of the Rendering library.
# Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
# Copyright (C) 2014-2020 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
cmake_minimum_required(VERSION 3.1.0)
project(Rendering VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set up install directories
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKECONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Rendering)

add_library(Rendering SHARED
	Buffer/BufferObject.cpp
	Buffer/BufferPool.cpp
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
	Core/Queue.cpp
	Core/ResourceCache.cpp
	Core/Sampler.cpp
	Core/Swapchain.cpp
	Mesh/Mesh.cpp
	Mesh/MeshDataStrategy.cpp
	Mesh/MeshIndexData.cpp
	Mesh/MeshVertexData.cpp
	Mesh/VertexAccessor.cpp
	Mesh/VertexAttributeAccessors.cpp
	Mesh/VertexAttributeIds.cpp
	MeshUtils/ConnectivityAccessor.cpp
	MeshUtils/LocalMeshDataHolder.cpp
	MeshUtils/MarchingCubesMeshBuilder.cpp
	MeshUtils/MeshBuilder.cpp
	MeshUtils/MeshUtils.cpp
	MeshUtils/PlatonicSolids.cpp
	MeshUtils/PrimitiveShapes.cpp
	MeshUtils/QuadtreeMeshBuilder.cpp
	MeshUtils/QuadtreeMeshBuilderDebug.cpp
	MeshUtils/Simplification.cpp
	MeshUtils/TriangleAccessor.cpp
	MeshUtils/WireShapes.cpp
	RenderingContext/RenderingParameters.cpp
	#Serialization/GenericAttributeSerialization.cpp
	#Serialization/Serialization.cpp
	#Serialization/StreamerMD2.cpp
	#Serialization/StreamerMMF.cpp
	#Serialization/StreamerMTL.cpp
	#Serialization/StreamerMVBO.cpp
	#Serialization/StreamerNGC.cpp
	#Serialization/StreamerOBJ.cpp
	#Serialization/StreamerPKM.cpp
	#Serialization/StreamerPLY.cpp
	#Serialization/StreamerXYZ.cpp
	Shader/Shader.cpp
	Shader/ShaderObjectInfo.cpp
	Shader/ShaderUtils.cpp
	Shader/Uniform.cpp
	Shader/UniformBuffer.cpp
	Shader/UniformRegistry.cpp
	State/BindingState.cpp
	State/PipelineState.cpp
	State/RenderingState.cpp
	State/ShaderLayout.cpp
	Texture/Texture.cpp
	Texture/TextureType.cpp
	#Texture/TextureUtils.cpp
	Draw.cpp
	DrawCompound.cpp
	FBO.cpp
	Helper.cpp
	#OcclusionQuery.cpp
	#PBO.cpp
	#QueryObject.cpp
	RenderingContext.cpp
	#StatisticsQuery.cpp
	#TextRenderer.cpp
)

# Dependency to Geometry
if(NOT TARGET Geometry)
	find_package(Geometry 0.3.0 REQUIRED NO_MODULE)
endif()
target_link_libraries(Rendering LINK_PUBLIC Geometry)

# Dependency to Util
if(NOT TARGET Util)
	find_package(Util 0.3.0 REQUIRED NO_MODULE)
endif()
target_link_libraries(Rendering LINK_PUBLIC Util)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Extern libraries
add_subdirectory(extern)
target_link_libraries(Rendering PRIVATE RenderingExtern)

# Set version of library
set_target_properties(Rendering PROPERTIES VERSION ${Rendering_VERSION}
																					 SOVERSION ${Rendering_VERSION_MAJOR}
																					 LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
add_subdirectory(tests)

# Install the header files
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/Core COMPONENT headers)
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
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/State/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/State COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Texture/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/Texture COMPONENT headers)
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering COMPONENT headers)


# Set data directory to the directory inside the sources
target_compile_definitions(Rendering PRIVATE COMPILE_DEFINITIONS RENDERING_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# Install the data directory and rename it to Rendering
install(DIRECTORY data/
	DESTINATION ${CMAKE_INSTALL_DATADIR}/Rendering COMPONENT data
)

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
																 VERSION ${Rendering_VERSION}
																 COMPATIBILITY SameMajorVersion)

# Install the configuration files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RenderingConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/RenderingConfigVersion.cmake
		DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR} COMPONENT developmentlibraries)

# Packaging
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "librendering${Rendering_VERSION_MAJOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Rendering library")
set(CPACK_PACKAGE_VENDOR "Benjamin Eikel; Claudius Jaehn; Ralf Petring; Sascha Brandt")
set(CPACK_PACKAGE_CONTACT "Benjamin Eikel <benjamin@eikel.org>")
set(CPACK_PACKAGE_VERSION_MAJOR ${Rendering_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${Rendering_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${Rendering_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "libRendering")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES ON)

include(CPack)
