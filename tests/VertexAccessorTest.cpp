/*
 This file is part of the Rendering library.
 Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "VertexAccessorTest.h"
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAccessor.h>

#include <Util/Timer.h>
#include <Util/References.h>

#include <cppunit/TestAssert.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>
CPPUNIT_TEST_SUITE_REGISTRATION(VertexAccessorTest);

using namespace Rendering;

void VertexAccessorTest::compareSpeed() {
  std::cout << std::endl;  
	const float coordinateRange = 1000.0f;
	std::uniform_real_distribution<float> coordinateDist(-coordinateRange, coordinateRange);
	std::default_random_engine engine(0);
	const float epsilon = (1.0f / coordinateRange);
  
  // build mesh
  VertexDescription vd;
  vd.appendPosition3D();
  vd.appendNormalFloat();
  vd.appendColorRGBAFloat();
  
  MeshVertexData vData;
  vData.allocate(10000, vd);
  Util::Timer t;
  
  std::vector<Geometry::Vec3> positions;
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions.emplace_back(coordinateDist(engine), coordinateDist(engine), coordinateDist(engine));
  }
  
  {
    t.reset();
    auto acc = PositionAttributeAccessor::create(vData);
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i]);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      CPPUNIT_ASSERT(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    std::cout << "PositionAttributeAccessor: " << t.getMilliseconds() << " ms" << std::endl;    
  }
  
  {
    t.reset();
    Util::Reference<VertexAccessor> acc = new VertexAccessor(vData);
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i]);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      CPPUNIT_ASSERT(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    std::cout << "VertexAccessor: " << t.getMilliseconds() << " ms" << std::endl;    
  }
}