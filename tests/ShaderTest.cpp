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
#include "../Shader/Uniform.h"
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

const std::string vertexShader = R"vs(
	#version 450

	layout(location = 0) in vec3 sg_Position;
	layout(location = 1) in vec4 sg_Color;

	layout(location = 0) out vec3 fragColor;

	layout(push_constant) uniform PushConstants {
		mat4 sg_matrix_modelToCamera;
		int testData;
	} test;

	layout(set=0, binding=0) uniform FrameData {
		mat4 sg_matrix_cameraToWorld;
		vec3 values[2];
	} fd;

	void main() {
		gl_Position = fd.sg_matrix_cameraToWorld * test.sg_matrix_modelToCamera * vec4(sg_Position + fd.values[test.testData], 1.0);
		fragColor = vec3(1);
	}
)vs";

const std::string fragmentShader = R"fs(
	#version 450
	
	layout(set=0, binding=0) uniform FrameData {
		mat4 sg_matrix_cameraToWorld;
		vec3 values[2];
	} fd;

	struct sg_LightSourceParameters {
		vec3 position;
		vec3 direction;
		vec4 ambient, diffuse, specular;
		float constant, linear, quadratic;
		float exponent, cosCutoff;
		int type;
	};

	layout(set=1, binding=1, std140) uniform LightData {
		sg_LightSourceParameters sg_LightSource[8];
	};

	// Currently not addressable by uniforms
	layout(set=1, binding=2, std140) uniform TestData {
		float bar;
		float blub;
	} foo[2];

	layout(location = 0) in vec3 fragColor;
	layout(location = 0) out vec4 outColor;

	void main() {
		outColor = vec4(fragColor, fd.values[1]) + sg_LightSource[0].ambient * foo[0].bar;
	}
)fs";

struct sg_LightSourceParameters {
	Geometry::Vec4 position; // 4*4 = 16
	Geometry::Vec4 direction; // 4*4 = 16
	Geometry::Vec4 ambient, diffuse, specular; // 3*4*4 = 48
	float constant, linear, quadratic; // 3*4 = 12
	float exponent, cosCutoff; // 2*4 = 8
	int32_t type; // 4
	uint32_t pad[2];
}; // 16+16+48+12+8+4 = 104, aligned size: 112

TEST_CASE("ShaderTest", "[ShaderTest]") {
	using namespace Rendering;
	using namespace Util;
	std::cout << std::endl;
	
	REQUIRE(sizeof(sg_LightSourceParameters) == 112);

	auto device = TestUtils::device;
	REQUIRE(device);
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
		std::cout << r.first.toString() << ": " << toString(r.second, true) << std::endl;

	auto& layout = shader->getLayout();
	REQUIRE(layout.hasLayoutSet(0));
	REQUIRE(layout.hasLayoutSet(1));
	REQUIRE(!layout.hasLayoutSet(2));
	REQUIRE(layout.getLayoutSet(0).hasLayout(0));
	REQUIRE(layout.getLayoutSet(0).getLayout(0).type == ShaderResourceType::BufferUniform);
	REQUIRE((layout.getLayoutSet(0).getLayout(0).stages & ShaderStage::Vertex) == ShaderStage::Vertex);
	REQUIRE((layout.getLayoutSet(0).getLayout(0).stages & ShaderStage::Fragment) == ShaderStage::Fragment);
	REQUIRE(!layout.getLayoutSet(0).hasLayout(1));
	REQUIRE(!layout.getLayoutSet(1).hasLayout(0));
	REQUIRE(layout.getLayoutSet(1).hasLayout(1));
	REQUIRE(layout.getLayoutSet(1).getLayout(1).stages == ShaderStage::Fragment);
	REQUIRE(layout.getLayoutSet(1).hasLayout(2));
	REQUIRE(layout.getLayoutSet(1).getLayout(2).stages == ShaderStage::Fragment);

	{
		auto colAttr = shader->getResource({"Vertex_sg_Color"});
		REQUIRE(colAttr);
		REQUIRE(colAttr.name == "sg_Color");
		REQUIRE(colAttr.location == 1);
		REQUIRE(colAttr.vecSize == 4);
	}

	{
		auto lightData = shader->getResource({"LightData"});
		REQUIRE(lightData);
		REQUIRE(lightData.set == 1);
		REQUIRE(lightData.binding == 1);
		REQUIRE(lightData.size == sizeof(sg_LightSourceParameters) * 8 );
	}

	{
		auto uniform = shader->getUniform({"sg_LightSource[2].constant"});
		REQUIRE(!uniform.isNull());
		REQUIRE(uniform.getType() == Uniform::UNIFORM_FLOAT);
		REQUIRE(uniform.getNumValues() == 1);
	}

	{
		auto uniform = shader->getUniform({"sg_LightSource[3].position"});
		REQUIRE(!uniform.isNull());
		REQUIRE(uniform.getType() == Uniform::UNIFORM_FLOAT);
		REQUIRE(uniform.getNumValues() == 3);
	}

	{
		auto uniform = shader->getUniform({"nonsense"});
		REQUIRE(uniform.isNull());
	}

	{
		auto uniform = shader->getUniform({"test.testData"});
		REQUIRE(!uniform.isNull());
		REQUIRE(uniform.getType() == Uniform::UNIFORM_INT);
	}

	{
		auto uniform = shader->getUniform({"fd.values"});
		REQUIRE(!uniform.isNull());
		REQUIRE(uniform.getType() == Uniform::UNIFORM_FLOAT);
		REQUIRE(uniform.getNumValues() == 6);
	}

	/*{ 
		auto uniform = shader->getUniform({"foo[0].bar"});
		REQUIRE(!uniform.isNull());
		REQUIRE(uniform.getType() == Uniform::UNIFORM_FLOAT);
		REQUIRE(uniform.getNumValues() == 1);
	}*/

	device->waitIdle();

}