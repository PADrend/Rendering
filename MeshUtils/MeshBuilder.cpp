/*
 This file is part of the Rendering library.
 Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
 Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
 Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
 Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshBuilder.h"
#include "PrimitiveShapes.h"
#include "MeshUtils.h"
#include "../Mesh/VertexAccessor.h"
#include "../Mesh/Mesh.h"

#include <Geometry/Matrix4x4.h>

#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Sphere.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelAccessor.h>

#include <cmath>
#include <map>

namespace Rendering {
namespace MeshUtils {
	
static uint32_t nextPowerOfTwo(uint32_t n) {
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

// -----------------------------------------------------------------------------

MeshBuilder::MeshBuilder() {
	description.appendPosition3D();
	description.appendNormalFloat();
	description.appendColorRGBAFloat();
	description.appendTexCoord();
	vData.allocate(1, description);
	iData.allocate(1);
	currentVertex.allocate(1, description);
	acc = VertexAccessor::create(currentVertex);
	acc->setColor(0, Util::Color4f{1,1,1,1}); // Default color WHITE
}

MeshBuilder::MeshBuilder(VertexDescription _description) : description(std::move(_description)) {
	vData.allocate(1, description);
	iData.allocate(1);
	currentVertex.allocate(1, description);
	acc = VertexAccessor::create(currentVertex);
	acc->setColor(0, Util::Color4f{1,1,1,1}); // Default color WHITE
}

MeshBuilder::~MeshBuilder() = default;

void MeshBuilder::position(const Geometry::Vec2 & v, const Util::StringIdentifier& attr) {
	if(transMat) {
		const auto transV = transMat->transformPosition(v.getX(), v.getY(), 0);
		acc->setPosition(0, transV, attr);
	} else {
		acc->setTexCoord(0, v, attr);
	}
}

void MeshBuilder::position(const Geometry::Vec3f & v, const Util::StringIdentifier& attr) {
	acc->setPosition(0, transMat ? transMat->transformPosition(v) : v, attr);
}

void MeshBuilder::position(const Geometry::Vec4 & v, const Util::StringIdentifier& attr) {
	acc->setVec4(0, transMat ? (*transMat) * v : v, attr);
}

void MeshBuilder::normal(const Geometry::Vec3f & n, const Util::StringIdentifier& attr) {
	acc->setNormal(0, transMat ? transMat->transformDirection(n) : n, attr);
}

void MeshBuilder::normal(const Geometry::Vec3b & n, const Util::StringIdentifier& attr) {
	normal( Geometry::Vec3f(n) );
}

void MeshBuilder::normal(const Geometry::Vec4f & n, const Util::StringIdentifier& attr) {
	acc->setVec4(0, transMat ? (*transMat) * n : n, attr);
}

void MeshBuilder::color(const Util::Color4f & c, const Util::StringIdentifier& attr) {
	acc->setColor(0, c, attr);
}

void MeshBuilder::color(const Util::Color4ub & c, const Util::StringIdentifier& attr) {
	acc->setColor(0, c, attr);
}

void MeshBuilder::texCoord0(const Geometry::Vec2 & uv, const Util::StringIdentifier& attr) {
	acc->setTexCoord(0, uv, attr);
}

void MeshBuilder::values(const std::vector<float> & v, const Util::StringIdentifier& attr) {
	acc->writeValues(0, attr, v);
}

void MeshBuilder::values(const std::vector<uint32_t> & v, const Util::StringIdentifier& attr) {
	acc->writeValues(0, attr, v);
}

void MeshBuilder::value(float v, const Util::StringIdentifier& attr) {
	acc->writeValue(0, attr, v);
}

void MeshBuilder::value(uint32_t v, const Util::StringIdentifier& attr) {
	acc->writeValue(0, attr, v);
}

uint32_t MeshBuilder::addVertex() {
	if(vSize >= vData.getVertexCount())
		vData.allocate(vData.getVertexCount()*2, description);
		
	std::copy(currentVertex.data(), currentVertex.data() + description.getVertexSize(), vData.data() + vSize * description.getVertexSize());
	return vSize++;
}

void MeshBuilder::addIndex(uint32_t idx) {
	if(iSize >= iData.getIndexCount())
		iData.allocate(iData.getIndexCount()*2);
	iData[iSize++] = idx;
}

Mesh* MeshBuilder::buildMesh() {
	if(isEmpty()) {
		std::cerr << "Empty Mesh..? (MeshBuilder::buildMesh)\n";
		return nullptr;
	}
	
	vData.allocate(vSize, description);
	vData.updateBoundingBox();
  
  if(iSize > 0) {
  	iData.allocate(iSize);
  	iData.updateIndexRange();
  }

	auto m = new Mesh(iData, vData);
	if(iSize == 0)
		m->setUseIndexData(false);

	return m;
}

void MeshBuilder::addQuad(uint32_t idx0, uint32_t idx1,	uint32_t idx2, uint32_t idx3) {
	// 0-1
	// |/|
	// 3-2
	addIndex(idx0);
	addIndex(idx1);
	addIndex(idx3);
	addIndex(idx1);
	addIndex(idx2);
	addIndex(idx3);
}

void MeshBuilder::addTriangle(uint32_t idx0, uint32_t idx1,	uint32_t idx2) {
	addIndex(idx0);
	addIndex(idx1);
	addIndex(idx2);
}

void MeshBuilder::addMesh(Mesh* mesh) {
	if(iSize + mesh->getIndexCount() > iData.getIndexCount())
		iData.allocate(nextPowerOfTwo(iSize + mesh->getIndexCount()));
	if(vSize + mesh->getVertexCount() > vData.getVertexCount())
		vData.allocate(nextPowerOfTwo(vSize + mesh->getVertexCount()), description);
	
	const auto& id = mesh->openIndexData();
	const auto& vd = mesh->openVertexData();
	
	if(mesh->getIndexCount() > 0) {
		std::copy(id.data(), id.data() + id.getIndexCount(), iData.data() + iSize);	
		if(vSize > 0) {
			for(uint32_t i=0; i<mesh->getIndexCount(); ++i) {
				iData[iSize+i] += vSize;
			}
		}
	}
		
	if(description == mesh->getVertexDescription()) {
		std::copy(vd.data(), vd.data() + vd.dataSize(), vData.data() + vSize*description.getVertexSize());
	} else {
		std::unique_ptr<MeshVertexData> newVd(MeshUtils::convertVertices(vd, description));
		std::copy(newVd->data(), newVd->data() + newVd->dataSize(), vData.data() + vSize*description.getVertexSize());
	}
	
	if(transMat) {
		auto va = VertexAccessor::create(vData);
		for(uint32_t i=0; i<mesh->getVertexCount(); ++i) {
			va->setPosition(i+vSize, transMat->transformPosition(va->getPosition(i+vSize)));
			va->setNormal(i+vSize, transMat->transformDirection(va->getNormal(i+vSize)));
		}
	}
	
	iSize += mesh->getIndexCount();
	vSize += mesh->getVertexCount();
}

Geometry::Matrix4x4 MeshBuilder::getTransformation() const {
	return !transMat ? Geometry::Matrix4x4() : *transMat;
}
void MeshBuilder::setTransformation(const Geometry::Matrix4x4 & m){
	if(m.isIdentity()) {
		transMat.reset();
	} else {
		transMat.reset(new Geometry::Matrix4x4(m));
	}
}
void MeshBuilder::setTransformation(const Geometry::SRT & s) {
	setTransformation(Geometry::Matrix4x4(s));
}

void MeshBuilder::transform(const Geometry::Matrix4x4 & m) {
	setTransformation(getTransformation() * m);
}

}
}
