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
#include <Geometry/Vec4.h>
#include <Geometry/Angle.h>
#include <Geometry/Matrix4x4.h>

#include "../RenderingContext.h"
#include "../Core/Device.h"
#include "../Shader/Shader.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexDescription.h"
#include "../MeshUtils/PrimitiveShapes.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <Util/StringUtils.h>
#include <cstdint>
#include <iostream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <vulkan/vulkan.hpp>

const std::string vertexShader = R"vs(
	#version 450

	layout(location = 0) in vec3 sg_Position;
	layout(location = 1) in vec4 sg_Color;

	layout(location = 0) out vec3 fragColor;

	layout(push_constant) uniform PushConstants {
		mat4 sg_matrix_modelToCamera;
	} test;

	layout(set=0, binding=0) uniform FrameData {
		mat4 sg_matrix_cameraToWorld;
	} fd;

	void main() {
		gl_Position = fd.sg_matrix_cameraToWorld * test.sg_matrix_modelToCamera * vec4(sg_Position, 1.0);
		fragColor = vec3(1);
	}
)vs";

const std::string fragmentShader = R"fs(
	#version 450
	
	struct sg_LightSourceParameters {
		int type;
		vec3 position;
		vec3 direction;
		vec4 ambient, diffuse, specular;
		float constant, linear, quadratic;
		float exponent, cosCutoff;
	};

	layout(set=1, binding=1, std140) uniform LightData {
		sg_LightSourceParameters sg_LightSource[8];
	};

	layout(location = 0) in vec3 fragColor;
	layout(location = 0) out vec4 outColor;

	void main() {
		outColor = vec4(fragColor, 1.0) + sg_LightSource[0].ambient;
	}
)fs";

struct sg_LightSourceParameters {
	int32_t type;	// 4
	Geometry::Vec3 position; // 3*4 = 12
	Geometry::Vec3 direction; // 3*4 = 12
	Geometry::Vec4 ambient, diffuse, specular; // 3*4*4 = 48
	float constant, linear, quadratic; // 3*4 = 12
	float exponent, cosCutoff; // 2*4 = 8
}; // 4+12+12+48+12+8 = 96

TEST_CASE("ShaderTest", "[ShaderTest]") {
	using namespace Rendering;
	using namespace Util;
	std::cout << std::endl;
	
	REQUIRE(sizeof(sg_LightSourceParameters) == 96);

	auto device = TestUtils::device;
	REQUIRE(device);
	vk::Device vkDevice(device->getApiHandle());
	REQUIRE(vkDevice);
	RenderingContext context(device);

	
	// --------------------------------------------
	// input

	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendColorRGBAByte();
	Mesh::Ref mesh = MeshUtils::createBox(vd, {});
	REQUIRE(mesh);
	
	// compile shaders
	auto shader = Shader::createShader(device, vertexShader, fragmentShader);
	REQUIRE(shader->init());
	REQUIRE(shader->getVertexAttributeLocation({"sg_Position"}) == 0);
	REQUIRE(shader->getVertexAttributeLocation({"sg_Color"}) == 1);
	REQUIRE(shader->getVertexAttributeLocation({"something"}) == -1);

	for(auto& r : shader->getResources())
		std::cout << toString(r.second) << std::endl;


	auto posAttr = shader->getResource({"Vertex_sg_Color"});
	REQUIRE(posAttr);
	REQUIRE(posAttr.name == "sg_Color");
	REQUIRE(posAttr.location == 1);
	REQUIRE(posAttr.vecSize == 4);

	auto lightData = shader->getResource({"LightData"});
	REQUIRE(lightData);
	REQUIRE(lightData.set == 1);
	REQUIRE(lightData.binding == 1);
	REQUIRE(lightData.size == sizeof(sg_LightSourceParameters) );

}