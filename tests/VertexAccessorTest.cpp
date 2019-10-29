/*
 This file is part of the Rendering library.
 Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <catch2/catch.hpp>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAccessor.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>

#include <Util/Timer.h>
#include <Util/References.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

using namespace Rendering;

TEST_CASE("VertexAccessorTest_compareSpeed", "[VertexAccessorTest]") {
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
      REQUIRE(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    std::cout << "PositionAttributeAccessor: " << t.getMilliseconds() << " ms" << std::endl;    
  }
  
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions[i]  = {coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)};
  }
  
  {
    t.reset();
    auto acc = PositionAttributeAccessor::create(vData);
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i]);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      REQUIRE(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    vData.upload();
    std::cout << "PositionAttributeAccessor (+upload): " << t.getMilliseconds() << " ms" << std::endl;
    vData.removeGlBuffer();
  }
  
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions[i]  = {coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)};
  }
  
  {
    t.reset();
    auto acc = VertexAccessor::create(vData);
    REQUIRE(acc.isNotNull());
    REQUIRE(acc->getDataSize() == vData.dataSize());
    REQUIRE(acc->getElementCount() == vData.getVertexCount());
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i]);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      REQUIRE(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    std::cout << "VertexAccessor (local): " << t.getMilliseconds() << " ms" << std::endl;    
  }
  
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions[i]  = {coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)};
  }
  
  {
    t.reset();
    auto acc = VertexAccessor::create(vData);
    REQUIRE(acc.isNotNull());
    REQUIRE(acc->getDataSize() == vData.dataSize());
    REQUIRE(acc->getElementCount() == vData.getVertexCount());
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i]);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      REQUIRE(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    vData.upload();
    std::cout << "VertexAccessor (local+upload): " << t.getMilliseconds() << " ms" << std::endl;
    vData.removeGlBuffer(); 
  }
  
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions[i]  = {coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)};
  }
  
  {
    vData.upload();
    vData.releaseLocalData();
    t.reset();
    auto acc = VertexAccessor::create(vData);
    REQUIRE(acc.isNotNull());
    REQUIRE(acc->getDataSize() == vData.getVertexCount() * vd.getVertexSize());
    REQUIRE(acc->getElementCount() == vData.getVertexCount());
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i]);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      REQUIRE(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    std::cout << "VertexAccessor (GPU;static): " << t.getMilliseconds() << " ms" << std::endl;    
  }
  
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions[i]  = {coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)};
  }
  
  {
    vData.download();
    vData.removeGlBuffer();
    vData.upload(BufferObject::USAGE_DYNAMIC_DRAW);
    vData.releaseLocalData();
    t.reset();
    auto acc = VertexAccessor::create(vData);    
    REQUIRE(acc.isNotNull());
    REQUIRE(acc->getDataSize() == vData.getVertexCount() * vd.getVertexSize());
    REQUIRE(acc->getElementCount() == vData.getVertexCount());
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i]);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      REQUIRE(acc->getPosition(i).distance(positions[i]) < epsilon);
    }
    std::cout << "VertexAccessor (GPU;dynamic): " << t.getMilliseconds() << " ms" << std::endl;    
  }
  
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions[i]  = {coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)};
  }
  
  {
    vData.upload();
    vData.releaseLocalData();
    t.reset();
    auto acc = VertexAccessor::create(vData);
    REQUIRE(acc.isNotNull());
    REQUIRE(acc->getDataSize() == vData.getVertexCount() * vd.getVertexSize());
    REQUIRE(acc->getElementCount() == vData.getVertexCount());
    uint16_t posLoc = acc->getFormat().getAttributeLocation(VertexAttributeIds::POSITION);
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i], posLoc);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      REQUIRE(acc->getPosition(i, posLoc).distance(positions[i]) < epsilon);
    }
    std::cout << "VertexAccessor (GPU;static;location): " << t.getMilliseconds() << " ms" << std::endl;    
  }
  
  for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
    positions[i]  = {coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)};
  }
  
  {
    vData.download();
    vData.removeGlBuffer();
    vData.upload(BufferObject::USAGE_DYNAMIC_DRAW);
    vData.releaseLocalData();
    t.reset();
    auto acc = VertexAccessor::create(vData);    
    REQUIRE(acc.isNotNull());
    REQUIRE(acc->getDataSize() == vData.getVertexCount() * vd.getVertexSize());
    REQUIRE(acc->getElementCount() == vData.getVertexCount());
    uint16_t posLoc = acc->getFormat().getAttributeLocation(VertexAttributeIds::POSITION);
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      acc->setPosition(i, positions[i], posLoc);
    }
    for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
      REQUIRE(acc->getPosition(i, posLoc).distance(positions[i]) < epsilon);
    }
    std::cout << "VertexAccessor (GPU;dynamic:location): " << t.getMilliseconds() << " ms" << std::endl;    
  }
}