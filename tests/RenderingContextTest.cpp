/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include <catch2/catch.hpp>

#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Geometry/Angle.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/SRT.h>

#include "../RenderingContext.h"
#include "../DrawCompound.h"
#include "../FBO.h"
#include "../Core/Device.h"
#include "../Core/BufferStorage.h"
#include "../Core/CommandBuffer.h"
#include "../Shader/Shader.h"
#include "../Shader/Uniform.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/MeshDataStrategy.h"
#include "../Mesh/VertexDescription.h"
#include "../MeshUtils/PrimitiveShapes.h"
#include "../MeshUtils/MeshBuilder.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <cstdint>
#include <iostream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>

const std::string vertexShader = R"vs(
	#version 450

	layout(location = 0) in vec3 sg_Position;
	layout(location = 1) in vec4 sg_Color;

	layout(location = 0) out vec3 fragColor;

	layout(push_constant) uniform PushConstants {
		mat4 sg_matrix_modelToClipping;
	};

	void main() {
		gl_Position = sg_matrix_modelToClipping * vec4(sg_Position, 1.0);
		gl_Position.y = -gl_Position.y; // Vulkan uses right hand NDC
		fragColor = sg_Color.rgb;
	}
)vs";

const std::string fragmentShader = R"fs(
	#version 450

	layout(location = 0) in vec3 fragColor;
	layout(location = 0) out vec4 outColor;

	struct sg_MaterialParameters {
		vec4 ambient, diffuse, specular, emission;
		float shininess;
	};

	layout(set=0, binding=0) uniform MaterialData {
		sg_MaterialParameters sg_Material;
	};

	void main() {
		outColor = sg_Material.diffuse;
	}
)fs";

TEST_CASE("RenderingContext", "[RenderingContextTest]") {
	using namespace Rendering;
	std::cout << std::endl;
	
	auto device = TestUtils::device;
	REQUIRE(device);
	RenderingContext context(device);

	// --------------------------------------------
	// input

	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendColorRGBAFloat();

	Mesh::Ref mesh = MeshUtils::createBox(vd, {-0.5,0.5,-0.5,0.5,-0.5,0.5});
	REQUIRE(mesh);
	mesh->setDataStrategy(SimpleMeshDataStrategy::getDynamicVertexStrategy());
	mesh->_getVertexData().upload();
	mesh->_getVertexData().getBuffer()->getBuffer()->setDebugName("Vertex Buffer");
	mesh->_getIndexData().upload();
	mesh->_getIndexData().getBuffer()->getBuffer()->setDebugName("Index Buffer");
	
	// compile shaders
	auto shader = Shader::createShader(device, vertexShader, fragmentShader);
	REQUIRE(shader->init());

	context.setShader(shader);
	REQUIRE(context.isShaderEnabled(shader));
	REQUIRE(!shader->getUniform({"sg_matrix_modelToClipping"}).isNull());

	auto projection = Geometry::Matrix4x4::perspectiveProjection( Geometry::Angle::deg(60), 1, 0.1, 10 );
	context.setMatrix_cameraToClipping(projection);

	Geometry::SRT camera;
	camera.setTranslation({1,1,1});
	camera.setRotation({1,1,1},{0,1,0});
	context.setMatrix_cameraToWorld(Geometry::Matrix4x4{camera});
	context.setMatrix_modelToCamera(context.getMatrix_worldToCamera());

	Geometry::Matrix4x4 mat;
	mat.rotate_deg(45, {1,0,0});
	mat.rotate_deg(45, {0,0,1});

	// --------------------------------------------
	// draw

	bool running = true;
	for(uint_fast32_t round = 0; round < 1000 && running; ++round) {

		context.clearScreen({0,0,0,1});
		
		context.pushAndSetMatrix_modelToCamera(context.getMatrix_worldToCamera());
		context.pushAndSetColorMaterial({1,0,0,1});
		context.displayMesh(mesh);
		context.popMaterial();

		context.setMatrix_modelToCamera(context.getMatrix_worldToCamera() * mat);
		context.pushAndSetColorMaterial({0,1,0,1});
		context.displayMesh(mesh);
		context.popMaterial();
		context.popMatrix_modelToCamera();

		context.present();
		
		mat.rotate_deg(0.1, {0,1,0});
		for(auto& e : TestUtils::window->fetchEvents()) {
			if(e.type == Util::UI::EVENT_KEYBOARD || e.type == Util::UI::EVENT_QUIT)
				running = false;
		}
	}
	device->waitIdle();
	
}