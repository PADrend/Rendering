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
#include "Mesh/Mesh.h"
#include "Mesh/MeshDataStrategy.h"
#include "Mesh/VertexAttributeIds.h"
#include "MeshUtils/MeshBuilder.h"
#include "MeshUtils/MeshUtils.h"
#include "MeshUtils/PlatonicSolids.h"
#include "GLHeader.h"
#include "Helper.h"
#include "RenderingContext/RenderingContext.h"
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec3.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <cstddef>
#include <list>
#include <cstdint>

namespace Rendering {

void drawFullScreenRect(RenderingContext & rc){
	GET_GL_ERROR();

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

	rc.pushProjectionMatrix();
	rc.setProjectionMatrix(projectionMatrix);

	rc.pushMatrix();
	rc.setMatrix(modelViewMatrix);

	rc.displayMesh(mesh.get());

	rc.popMatrix();
	rc.popProjectionMatrix();

	GET_GL_ERROR();
}

void drawAbsBox(RenderingContext & rc, const Geometry::Box & box) {
	rc.pushMatrix();
	rc.resetMatrix();
	drawBox(rc, box);
	rc.popMatrix();
}

void drawAbsBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawAbsBox(rc, box);
	rc.popMaterial();
}

void drawAbsWireframeBox(RenderingContext & rc, const Geometry::Box & box) {
	rc.pushMatrix();
	rc.resetMatrix();
	drawWireframeBox(rc, box);
	rc.popMatrix();
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
		mesh = MeshUtils::MeshBuilder::createBox(vertexDescription, unitBox);
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(box.getCenter());
	matrix.scale(box.getExtentX(), box.getExtentY(), box.getExtentZ());
	rc.pushMatrix();
	rc.multMatrix(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix();
}

void drawFastAbsBox(RenderingContext & rc, const Geometry::Box & b){
	#ifdef LIB_GL
	rc.pushAndSetShader(nullptr);

//  Too slow:
//	rc.pushMatrix();
//	rc.resetMatrix();
//	rc.applyChanges();

	glPushMatrix();
	glLoadTransposeMatrixf( rc.getCameraMatrix().getData());

	static const unsigned int indices[]={
		1,3,2,0,
		5,7,3,1,
		4,6,7,5,
		0,2,6,4,
		7,6,2,3,
		4,5,1,0
	};
	float corners[8][3] = {{b.getMaxX(), b.getMaxY(), b.getMaxZ()},
					{b.getMinX(), b.getMaxY(), b.getMaxZ()},
					{b.getMaxX(), b.getMinY(), b.getMaxZ()},
					{b.getMinX(), b.getMinY(), b.getMaxZ()},
					{b.getMaxX(), b.getMaxY(), b.getMinZ()},
					{b.getMinX(), b.getMaxY(), b.getMinZ()},
					{b.getMaxX(), b.getMinY(), b.getMinZ()},
					{b.getMinX(), b.getMinY(), b.getMinZ()}};

	glBegin(GL_QUADS);
	for(auto & index : indices){
		glVertex3fv(corners[index]);
	}
	glEnd();

	glPopMatrix();
//	rc.popMatrix();
	rc.popShader();
	#else
	drawAbsBox(rc,b);
	#endif
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
		mesh = new Mesh(vertexDescription, 8, 16);
		mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());
		mesh->setDrawMode(Mesh::DRAW_LINE_STRIP);

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		/*
		 *  Corners:
		 *     6---------7
		 *    /|        /|
		 *   / |       / |
		 *  2---------3  |
		 *  |  |      |  |
		 *  |  4------|--5
		 *  | /       | /
		 *  |/        |/
		 *  0---------1
		 */
		indices[0] = 0;
		indices[1] = 2;
		indices[2] = 3;
		indices[3] = 1;
		indices[4] = 5;
		indices[5] = 7;
		indices[6] = 6;
		indices[7] = 4;
		indices[8] = 0;
		indices[9] = 1;
		indices[10] = 3;
		indices[11] = 7;
		indices[12] = 5;
		indices[13] = 4;
		indices[14] = 6;
		indices[15] = 2;
		id.updateIndexRange();
		id.markAsChanged();
	}

	MeshVertexData & vd = mesh->openVertexData();
	float * vertices = reinterpret_cast<float *>(vd.data());
	for (uint_fast8_t c = 0; c < 8; ++c) {
		const Geometry::Vec3 & corner = box.getCorner(static_cast<Geometry::corner_t> (c));
		*vertices++ = corner.getX();
		*vertices++ = corner.getY();
		*vertices++ = corner.getZ();
	}
	vd._setBoundingBox(box);
	vd.markAsChanged();

	rc.displayMesh(mesh.get());
}

void drawWireframeBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawWireframeBox(rc, box);
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
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition2D();
		mesh = new Mesh(vertexDescription, 4, 4);
		mesh->setDrawMode(Mesh::DRAW_LINE_LOOP);

		MeshVertexData & vd = mesh->openVertexData();
		float * vertices = reinterpret_cast<float *> (vd.data());
		*vertices++ = 0.0f; // Bottom left
		*vertices++ = 0.0f;
		*vertices++ = 1.0f; // Bottom right
		*vertices++ = 0.0f;
		*vertices++ = 1.0f; // Top right
		*vertices++ = 1.0f;
		*vertices++ = 0.0f; // Top left
		*vertices++ = 1.0f;
		vd.updateBoundingBox();
		vd.markAsChanged();

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 3;
		id.updateIndexRange();
		id.markAsChanged();
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(rect.getX(), rect.getY(), 0.0f);
	matrix.scale(rect.getWidth(), rect.getHeight(), 1.0f);
	rc.pushMatrix();
	rc.multMatrix(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix();
}

void drawWireframeRect(RenderingContext & rc, const Geometry::Rect & rect, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawWireframeRect(rc, rect);
	rc.popMaterial();
}

void drawRect(RenderingContext & rc, const Geometry::Rect & rect) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition2D();
		mesh = new Mesh(vertexDescription, 4, 6);
		mesh->setDrawMode(Mesh::DRAW_TRIANGLES);

		MeshVertexData & vd = mesh->openVertexData();
		float * vertices = reinterpret_cast<float *> (vd.data());
		*vertices++ = 0.0f; // Bottom left
		*vertices++ = 0.0f;
		*vertices++ = 1.0f; // Bottom right
		*vertices++ = 0.0f;
		*vertices++ = 1.0f; // Top right
		*vertices++ = 1.0f;
		*vertices++ = 0.0f; // Top left
		*vertices++ = 1.0f;
		vd.updateBoundingBox();
		vd.markAsChanged();

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		indices[0] = 0;
		indices[1] = 2;
		indices[2] = 1;
		indices[3] = 0;
		indices[4] = 3;
		indices[5] = 2;
		id.updateIndexRange();
		id.markAsChanged();
	}

	Geometry::Matrix4x4 matrix;
	matrix.translate(rect.getX(), rect.getY(), 0.0f);
	matrix.scale(rect.getWidth(), rect.getHeight(), 1.0f);
	rc.pushMatrix();
	rc.multMatrix(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix();
}

void drawRect(RenderingContext & rc, const Geometry::Rect & rect, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawRect(rc, rect);
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

void drawVector(RenderingContext & rc, const Geometry::Vec3f & from, const Geometry::Vec3f & to, const Util::Color4f & color) {
	rc.pushAndSetColorMaterial(color);
	drawVector(rc, from, to);
	rc.popMaterial();
}

void enable2DMode(RenderingContext & rc,const Geometry::Rect_i & screenRect){
	rc.pushProjectionMatrix();
	rc.setProjectionMatrix(Geometry::Matrix4x4f::orthographicProjection(screenRect.getMinX(), screenRect.getMaxX(),screenRect.getMaxY(),screenRect.getMinY(), -1, 1));

	rc.pushMatrix();
	rc.setMatrix(Geometry::Matrix4x4f());
}
void enable2DMode(RenderingContext & rc) {
	enable2DMode(rc, Geometry::Rect_i(0,0, rc.getWindowClientArea().getWidth(), rc.getWindowClientArea().getHeight()));
}

void disable2DMode(RenderingContext & rc) {
	rc.popMatrix();
	rc.popProjectionMatrix();
}

}
