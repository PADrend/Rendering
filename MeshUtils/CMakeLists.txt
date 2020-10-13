#
# This file is part of the MinSG library.
# Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

target_sources(Rendering PRIVATE
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
)

# Install the header files
file(GLOB RENDERING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${RENDERING_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rendering/MeshUtils COMPONENT headers)