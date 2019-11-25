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
#include <Geometry/Matrix4x4.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Draw.h>
#include <Rendering/Helper.h>
#include <Rendering/Shader/ShaderUtils.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Util/Timer.h>
#include <Util/Graphics/Color.h>
#include <cstdint>
#include <iostream>

TEST_CASE("DrawTest_testBox", "[DrawTest]") {
	using namespace Rendering;
	std::cout << std::endl;

	const Geometry::Box boxA(Geometry::Vec3f(2.0f, 2.0f, 2.0f), 3.0f);
	const Geometry::Box boxB(Geometry::Vec3f(-5.0f, -5.0f, -5.0f), 1.0f);
	const Geometry::Box boxC(Geometry::Vec3f(17.0f, 17.0f, 17.0f), 12.0f);

	RenderingContext context;
	context.setImmediateMode(false);
	Rendering::disableGLErrorChecking();
	auto shader = ShaderUtils::createDefaultShader();
	context.pushAndSetShader(shader.get());

	Util::Timer drawFastBoxTimer;
	Util::Timer drawBoxTimer;
	for(uint_fast32_t round = 0; round < 1000; ++round) {
		drawFastBoxTimer.resume();
		context.applyChanges();
		for(uint_fast32_t box = 0; box < 1000; ++box) {
			drawFastAbsBox(context, boxA);
			drawFastAbsBox(context, boxB);
			drawFastAbsBox(context, boxC);
		}
		drawFastBoxTimer.stop();
		drawBoxTimer.resume();
		for(uint_fast32_t box = 0; box < 1000; ++box) {
			drawAbsBox(context, boxA);
			drawAbsBox(context, boxB);
			drawAbsBox(context, boxC);
		}
		drawBoxTimer.stop();
	}
	context.popShader();
	std::cout << "drawFastAbsBox: " << drawFastBoxTimer.getSeconds() << " s" << std::endl;
	std::cout << "drawAbsBox: " << drawBoxTimer.getSeconds() << " s" << std::endl;
}

TEST_CASE("DrawTest_testTriangle", "[DrawTest]") {
	using namespace Rendering;	
	RenderingContext context;	
	
	auto shader = ShaderUtils::createDefaultShader();
	context.pushAndSetShader(shader.get());
	
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	MeshUtils::MeshBuilder mb(vd);
	mb.position(Geometry::Vec3f(-1,-1,0)); mb.color(Util::Color4f(1,0,0)); mb.addVertex();
	mb.position(Geometry::Vec3f(1,-1,0)); mb.color(Util::Color4f(0,1,0)); mb.addVertex();
	mb.position(Geometry::Vec3f(0,1,0)); mb.color(Util::Color4f(0,0,1)); mb.addVertex();
	mb.addTriangle(0, 1, 2);
	Util::Reference<Mesh> mesh = mb.buildMesh();
		
	for(uint_fast32_t round = 0; round < 10000; ++round) {
		context.clearScreen({0,0,0});
		context.applyChanges();
				
		context.displayMesh(mesh.get());
		
		TestUtils::window->swapBuffers();
	}

	context.popShader();
}