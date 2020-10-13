/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include <catch2/catch.hpp>

#include <Geometry/Box.h>
#include <Geometry/Sphere.h>
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
#include "../MeshUtils/PlatonicSolids.h"
#include "../MeshUtils/MeshBuilder.h"
#include "../State/RenderingState.h"
#include "../Texture/Texture.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <cstdint>
#include <iostream>

TEST_CASE("RenderingContext", "[RenderingContextTest]") {
	using namespace Rendering;
	std::cout << std::endl;
	
	auto device = TestUtils::device;
	REQUIRE(device);
	RenderingContext context(device);

	auto shader = context.getFallbackShader();
	REQUIRE(shader->init());
	REQUIRE(shader->isUniform({"sg_matrix_modelToCamera"}));
	REQUIRE(shader->isUniform({"sg_lightCount"}));
	REQUIRE(shader->isUniform({"sg_Light[0].intensity"}));

	// --------------------------------------------
	// meshes

	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	vd.appendTexCoord();

	Mesh::Ref mesh1 = MeshUtils::createBox(vd, {-0.5,0.5,-0.5,0.5,-0.5,0.5});
	REQUIRE(mesh1);
	mesh1->setDataStrategy(SimpleMeshDataStrategy::getDynamicVertexStrategy());
	mesh1->_getVertexData().upload();
	mesh1->_getVertexData().getBuffer()->getBuffer()->setDebugName("Vertex Buffer 1");
	mesh1->_getIndexData().upload();
	mesh1->_getIndexData().getBuffer()->getBuffer()->setDebugName("Index Buffer 1");

	Mesh::Ref mesh2 = MeshUtils::PlatonicSolids::createOctahedron(vd);
	REQUIRE(mesh2);
	mesh2->setDataStrategy(SimpleMeshDataStrategy::getStaticDrawReleaseLocalStrategy());
	mesh2->_getVertexData().upload();
	mesh2->_getVertexData().getBuffer()->getBuffer()->setDebugName("Vertex Buffer 2");
	mesh2->_getIndexData().upload();
	mesh2->_getIndexData().getBuffer()->getBuffer()->setDebugName("Index Buffer 2");

	// --------------------------------------------
	// matrices

	auto projection = Geometry::Matrix4x4::perspectiveProjection( Geometry::Angle::deg(60), 1, 0.1, 10 );
	context.setMatrix_cameraToClipping(projection);

	Geometry::SRT camera;
	camera.setTranslation({1.5,1.5,1.5});
	camera.setRotation({1,1,1},{0,1,0});
	context.setMatrix_cameraToWorld(Geometry::Matrix4x4{camera});
	context.setMatrix_modelToCamera(context.getMatrix_worldToCamera());

	Geometry::Matrix4x4 mat;

	// --------------------------------------------
	// materials

	MaterialData material1{};
	material1.setDiffuse({1,0,0});
	material1.setAmbient(material1.getDiffuse() * 0.1);
	material1.setSpecular({0.5,0.5,0.5,0});

	MaterialData material2{};
	material2.setDiffuse({0,1,0});
	material2.setAmbient(material2.getDiffuse() * 0.1);
	material1.setSpecular({1,1,1,1});

	// --------------------------------------------
	// light
	LightData light{};
	light.setIntensity({30,30,30});
	Geometry::Matrix4x4 lightMat;

	{
		REQUIRE(context.getRenderingState().getLights().getLightCount() == 0);
		auto lightId = context.enableLight(light);
		REQUIRE(lightId > 0);
		REQUIRE(context.getRenderingState().getLights().getLightCount() == 1);
		context.disableLight(lightId);
		REQUIRE(context.getRenderingState().getLights().getLightCount() == 0);
	}
	
	// --------------------------------------------
	// draw

	bool running = true;
	for(uint_fast32_t round = 0; round < 10000000 && running; ++round) {

		context.clearScreen({0,0,0,1});

		light.setPosition(lightMat.transformPosition({2,1,2}));
		auto lightId = context.enableLight(light);
		
		context.pushAndSetMatrix_modelToCamera(context.getMatrix_worldToCamera());
		context.pushAndSetMaterial(material1);
		context.displayMesh(mesh1);
		context.popMaterial();

		context.setMatrix_modelToCamera(context.getMatrix_worldToCamera() * mat);
		context.pushAndSetMaterial(material2);
		context.displayMesh(mesh2);
		context.popMaterial();
		context.popMatrix_modelToCamera();

		context.disableLight(lightId);

		context.present();
		
		mat.rotate_deg(0.1, {0,1,0});
		lightMat.rotate_deg(-0.05, {0,1,0});
		for(auto& e : TestUtils::window->fetchEvents()) {
			if(e.type == Util::UI::EVENT_KEYBOARD || e.type == Util::UI::EVENT_QUIT)
				running = false;
		}
	}
	device->waitIdle();
	
}