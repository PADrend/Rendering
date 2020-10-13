/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Draw.h"
#include "BufferObject.h"
#include "Mesh/Mesh.h"
#include "Mesh/MeshDataStrategy.h"
#include "Mesh/VertexAttribute.h"
#include "Mesh/VertexAttributeIds.h"
#include "MeshUtils/MeshBuilder.h"
#include "MeshUtils/MeshUtils.h"
#include "MeshUtils/PrimitiveShapes.h"
#include "MeshUtils/PlatonicSolids.h"
#include "MeshUtils/WireShapes.h"
#include "Shader/Shader.h"
#include "Shader/Uniform.h"
#include "Helper.h"
#include "RenderingContext/RenderingContext.h"
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec2.h>
#include <Geometry/Convert.h>
#include <Geometry/Sphere.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <Util/Macros.h>
#include <cstddef>
#include <list>
#include <cstdint>

namespace Rendering {

void drawFullScreenRect(RenderingContext & rc){
	static Geometry::Matrix4x4f projectionMatrix(Geometry::Matrix4x4f::orthographicProjection(-1, 1, -1, 1, -1, 1));
	static Geometry::Matrix4x4f modelViewMatrix;
	static Util::Reference<Mesh> mesh;
	if(mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendTexCoord();
		MeshUtils::MeshBuilder mb(vertexDescription);
		mb.position(Geometry::Vec3f(-1,-1,0)); 	mb.texCoord0(Geometry::Vec2f(0,0));	uint32_t a = mb.addVertex();
		mb.position(Geometry::Vec3f(1,-1,0)); 	mb.texCoord0(Geometry::Vec2f(1,0));	uint32_t b = mb.addVertex();
		mb.position(Geometry::Vec3f(-1,1,0)); 	mb.texCoord0(Geometry::Vec2f(0,1));	uint32_t c = mb.addVertex();
		mb.position(Geometry::Vec3f(1,1,0)); 	mb.texCoord0(Geometry::Vec2f(1,1));	uint32_t d = mb.addVertex();
		mb.addTriangle(a, b, c);
		mb.addTriangle(c, b, d);
		mesh = mb.buildMesh();
	}

	rc.pushMatrix_cameraToClipping();
	rc.setMatrix_cameraToClipping(projectionMatrix);

	rc.pushMatrix_modelToCamera();
	rc.setMatrix_modelToCamera(modelViewMatrix);

	rc.displayMesh(mesh.get());

	rc.popMatrix_modelToCamera();
	rc.popMatrix_cameraToClipping();
}

void drawAbsBox(RenderingContext & renderingContext, const Geometry::Box & box) {
	renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
	drawBox(renderingContext, box);
	renderingContext.popMatrix_modelToCamera();
}

void drawAbsBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawAbsBox(rc, box);
	rc.popMaterial();
}

void drawAbsWireframeBox(RenderingContext & renderingContext, const Geometry::Box & box) {
	renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
	drawWireframeBox(renderingContext, box);
	renderingContext.popMatrix_modelToCamera();
}

void drawAbsWireframeBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawAbsWireframeBox(rc, box);
	rc.popMaterial();
}

void drawBox(RenderingContext & rc, const Geometry::Box & box) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();
		const Geometry::Box unitBox(Geometry::Vec3(-0.5f, -0.5f, -0.5f), Geometry::Vec3(0.5f, 0.5f, 0.5f));
		mesh = MeshUtils::createBox(vertexDescription, unitBox);
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(box.getCenter());
	matrix.scale(box.getExtentX(), box.getExtentY(), box.getExtentZ());
	rc.pushMatrix_modelToCamera();
	rc.multMatrix_modelToCamera(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix_modelToCamera();
}

void drawFastAbsBox(RenderingContext & rc, const Geometry::Box & b){
	drawAbsBox(rc,b);
}

void drawBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawBox(rc, box);
	rc.popMaterial();
}

void drawWireframeBox(RenderingContext & rc, const Geometry::Box & box) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		const Geometry::Box unitBox(Geometry::Vec3(-0.5f, -0.5f, -0.5f), Geometry::Vec3(0.5f, 0.5f, 0.5f));
		mesh = MeshUtils::WireShapes::createWireBox(vertexDescription, unitBox);
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(box.getCenter());
	matrix.scale(box.getExtentX(), box.getExtentY(), box.getExtentZ());
	rc.pushMatrix_modelToCamera();
	rc.multMatrix_modelToCamera(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix_modelToCamera();
}

void drawWireframeBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawWireframeBox(rc, box);
	rc.popMaterial();
}

void drawWireframeSphere(RenderingContext & rc, const Geometry::Sphere & sphere) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		const Geometry::Sphere unitSphere(Geometry::Vec3(0,0,0), 1);
		mesh = MeshUtils::WireShapes::createWireSphere(vertexDescription, unitSphere, 32);
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(sphere.getCenter());
	matrix.scale(sphere.getRadius());
	rc.pushMatrix_modelToCamera();
	rc.multMatrix_modelToCamera(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix_modelToCamera();
}

void drawWireframeSphere(RenderingContext & rc, const Geometry::Sphere & sphere, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawWireframeSphere(rc, sphere);
	rc.popMaterial();
}

void drawQuad(RenderingContext & rc, const Geometry::Vec3 & lowerLeft, const Geometry::Vec3 & lowerRight, const Geometry::Vec3 & upperRight,
				const Geometry::Vec3 & upperLeft) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();
		vertexDescription.appendTexCoord();
		mesh = new Mesh(vertexDescription, 4, 6);

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 0;
		indices[4] = 2;
		indices[5] = 3;
		id.updateIndexRange();
		id.markAsChanged();
	}
	const Geometry::Vec3 edgeA = lowerRight - lowerLeft;
	const Geometry::Vec3 edgeB = upperLeft - lowerLeft;
	Geometry::Vec3 normal = edgeA.cross(edgeB);
	normal.normalize();

	MeshVertexData & vd = mesh->openVertexData();
	float * vertices = reinterpret_cast<float *> (vd.data());
	// Lower left
	*vertices++ = lowerLeft.getX();
	*vertices++ = lowerLeft.getY();
	*vertices++ = lowerLeft.getZ();
	*vertices++ = normal.getX();
	*vertices++ = normal.getY();
	*vertices++ = normal.getZ();
	*vertices++ = 0.0f;
	*vertices++ = 0.0f;
	// Lower right
	*vertices++ = lowerRight.getX();
	*vertices++ = lowerRight.getY();
	*vertices++ = lowerRight.getZ();
	*vertices++ = normal.getX();
	*vertices++ = normal.getY();
	*vertices++ = normal.getZ();
	*vertices++ = 1.0f;
	*vertices++ = 0.0f;
	// Upper right
	*vertices++ = upperRight.getX();
	*vertices++ = upperRight.getY();
	*vertices++ = upperRight.getZ();
	*vertices++ = normal.getX();
	*vertices++ = normal.getY();
	*vertices++ = normal.getZ();
	*vertices++ = 1.0f;
	*vertices++ = 1.0f;
	// Upper left
	*vertices++ = upperLeft.getX();
	*vertices++ = upperLeft.getY();
	*vertices++ = upperLeft.getZ();
	*vertices++ = normal.getX();
	*vertices++ = normal.getY();
	*vertices++ = normal.getZ();
	*vertices++ = 0.0f;
	*vertices++ = 1.0f;
	vd.updateBoundingBox();
	vd.markAsChanged();

	rc.displayMesh(mesh.get());
}

void drawQuad(RenderingContext & rc, const Geometry::Vec3f & lowerLeft, const Geometry::Vec3f & lowerRight, const Geometry::Vec3f & upperRight,
				const Geometry::Vec3f & upperLeft, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawQuad(rc, lowerLeft, lowerRight, upperRight, upperLeft);
	rc.popMaterial();
}

void drawWireframeRect(RenderingContext & rc, const Geometry::Rect & rect) {
	static Util::Reference<Mesh> mesh;
	if(mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition2D();
		Geometry::Rect unitRect(0,0,1,1);
		mesh = MeshUtils::WireShapes::createWireRectangle(vertexDescription, unitRect);
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(rect.getX(), rect.getY(), 0.0f);
	matrix.scale(rect.getWidth(), rect.getHeight(), 1.0f);
	rc.pushMatrix_modelToCamera();
	rc.multMatrix_modelToCamera(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix_modelToCamera();
}

void drawWireframeRect(RenderingContext & rc, const Geometry::Rect & rect, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawWireframeRect(rc, rect);
	rc.popMaterial();
}

void drawRect(RenderingContext & rc, const Geometry::Rect & rect) {
	static Util::Reference<Mesh> mesh;
	if(mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition2D();
		Geometry::Rect unitRect(0,0,1,1);
		mesh = MeshUtils::createRectangle(vertexDescription, unitRect);
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(rect.getX(), rect.getY(), 0.0f);
	matrix.scale(rect.getWidth(), rect.getHeight(), 1.0f);
	rc.pushMatrix_modelToCamera();
	rc.multMatrix_modelToCamera(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix_modelToCamera();
}

void drawRect(RenderingContext & rc, const Geometry::Rect & rect, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawRect(rc, rect);
	rc.popMaterial();
}

void drawWireframeCircle(RenderingContext & rc, const Geometry::Vec2f & center, float radius) {
	static Util::Reference<Mesh> mesh;
	if(mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition2D();
		mesh = MeshUtils::WireShapes::createWireCircle(vertexDescription, 1, 32);
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(center.getX(), center.getY(), 0.0f);
	matrix.scale(radius, radius, 1.0f);
	rc.pushMatrix_modelToCamera();
	rc.multMatrix_modelToCamera(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix_modelToCamera();
}

void drawWireframeCircle(RenderingContext & rc, const Geometry::Vec2f & center, float radius, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawWireframeCircle(rc, center, radius);
	rc.popMaterial();
}

void drawTriangle(RenderingContext & rc, const Geometry::Vec3f & vertexA, const Geometry::Vec3f & vertexB, const Geometry::Vec3f & vertexC) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		mesh = new Mesh(vertexDescription, 3, 3);

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		id.updateIndexRange();
		id.markAsChanged();
	}

	MeshVertexData & vd = mesh->openVertexData();
	float * vertices = reinterpret_cast<float *>(vd.data());
	// First vertex
	*vertices++ = vertexA.getX();
	*vertices++ = vertexA.getY();
	*vertices++ = vertexA.getZ();
	// Second vertex
	*vertices++ = vertexB.getX();
	*vertices++ = vertexB.getY();
	*vertices++ = vertexB.getZ();
	// Third vertex
	*vertices++ = vertexC.getX();
	*vertices++ = vertexC.getY();
	*vertices++ = vertexC.getZ();
	vd.updateBoundingBox();
	vd.markAsChanged();

	rc.displayMesh(mesh.get());
}

void drawVector(RenderingContext & rc, const Geometry::Vec3 & from, const Geometry::Vec3 & to) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		mesh = new Mesh(vertexDescription, 2, 2);
		mesh->setDrawMode(Mesh::DRAW_LINES);

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		indices[0] = 0;
		indices[1] = 1;
		id.updateIndexRange();
		id.markAsChanged();
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
	}

	MeshVertexData & vd = mesh->openVertexData();
	float * vertices = reinterpret_cast<float *> (vd.data());
	*vertices++ = from.getX(); // From
	*vertices++ = from.getY();
	*vertices++ = from.getZ();
	*vertices++ = to.getX(); // To
	*vertices++ = to.getY();
	*vertices++ = to.getZ();
	vd.updateBoundingBox();
	vd.markAsChanged();

	rc.displayMesh(mesh.get());
}
void drawVector(RenderingContext & rc, const Geometry::Vec3 & from, const Geometry::Vec3 & to, const Util::Color4f & color1, const Util::Color4f & color2) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendColorRGBAFloat();
		mesh = new Mesh(vertexDescription, 2, 2);
		mesh->setDrawMode(Mesh::DRAW_LINES);

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		indices[0] = 0;
		indices[1] = 1;
		id.updateIndexRange();
		id.markAsChanged();
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
	}

	MeshVertexData & vd = mesh->openVertexData();
	float * vertices = reinterpret_cast<float *> (vd.data());
	*vertices++ = from.getX(); // From
	*vertices++ = from.getY();
	*vertices++ = from.getZ();
	*vertices++ = color1.r(); // FromColor
	*vertices++ = color1.g();
	*vertices++ = color1.b();
	*vertices++ = color1.a();
	*vertices++ = to.getX(); // To
	*vertices++ = to.getY();
	*vertices++ = to.getZ();
	*vertices++ = color2.r(); // ToColor
	*vertices++ = color2.g();
	*vertices++ = color2.b();
	*vertices++ = color2.a();
	vd.updateBoundingBox();
	vd.markAsChanged();

	rc.displayMesh(mesh.get());
}

void drawVector(RenderingContext & rc, const Geometry::Vec3f & from, const Geometry::Vec3f & to, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawVector(rc, from, to);
	rc.popMaterial();
}

void enable2DMode(RenderingContext & rc,const Geometry::Rect_i & screenRect){
	rc.pushMatrix_cameraToClipping();
	rc.setMatrix_cameraToClipping(Geometry::Matrix4x4f::orthographicProjection(screenRect.getMinX(), screenRect.getMaxX(),screenRect.getMaxY(),screenRect.getMinY(), -1, 1));

	rc.pushMatrix_modelToCamera();
	rc.setMatrix_modelToCamera(Geometry::Matrix4x4f());
}
void enable2DMode(RenderingContext & rc) {
	enable2DMode(rc, Geometry::Rect_i(0,0, rc.getWindowClientArea().getWidth(), rc.getWindowClientArea().getHeight()));
}

void disable2DMode(RenderingContext & rc) {
	rc.popMatrix_modelToCamera();
	rc.popMatrix_cameraToClipping();
}

void enableInstanceBuffer(RenderingContext & rc, BufferObject & instanceBuffer, int32_t location, uint32_t elements) {}

void disableInstanceBuffer(RenderingContext & rc, BufferObject & instanceBuffer, int32_t location, uint32_t elements) {}

void drawInstances(RenderingContext & rc, Mesh* m, uint32_t firstElement, uint32_t elementCount, uint32_t instanceCount) {
	WARN("Instancing is not supported.");
}

}
