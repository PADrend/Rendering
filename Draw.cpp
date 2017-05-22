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
#include "MeshUtils/PlatonicSolids.h"
#include "Shader/Shader.h"
#include "Shader/Uniform.h"
#include "GLHeader.h"
#include "Helper.h"
#include "RenderingContext/RenderingContext.h"
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec2.h>
#include <Geometry/Convert.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <Util/Macros.h>
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

	rc.pushMatrix_cameraToClipping();
	rc.setMatrix_cameraToClipping(projectionMatrix);

	rc.pushMatrix_modelToCamera();
	rc.setMatrix_modelToCamera(modelViewMatrix);

	rc.displayMesh(mesh.get());

	rc.popMatrix_modelToCamera();
	rc.popMatrix_cameraToClipping();

	GET_GL_ERROR();
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
		mesh = MeshUtils::MeshBuilder::createBox(vertexDescription, unitBox);
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
	#ifdef LIB_GL
	rc.pushAndSetShader(nullptr);

//  Too slow:
//	rc.pushMatrix_modelToCamera();
//	rc.resetMatrix();
//	rc.applyChanges();

	glPushMatrix();
	glLoadTransposeMatrixf( rc.getMatrix_worldToCamera().getData());

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
//	rc.popMatrix_modelToCamera();
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
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition2D();
		uint32_t segments = 32;
		mesh = new Mesh(vertexDescription, segments, segments);
		mesh->setDrawMode(Mesh::DRAW_LINE_LOOP);

		MeshVertexData & vd = mesh->openVertexData();
		MeshIndexData & id = mesh->openIndexData();
		float * vertices = reinterpret_cast<float *> (vd.data());
		uint32_t * indices = id.data();
		for(uint32_t s=0; s<segments; ++s) {
			float a = s * Geometry::Convert::degToRad(360.0f) / segments;
			*vertices++ = std::sin(a); 
			*vertices++ = std::cos(a); ;
			indices[s] = s;
		}
		vd.updateBoundingBox();
		vd.markAsChanged();
		id.updateIndexRange();
		id.markAsChanged();
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

void enableInstanceBuffer(RenderingContext & rc, BufferObject & instanceBuffer, int32_t location, uint32_t elements) {	
	if(location < 0)
		return;
		
#if defined(LIB_GL) && defined(GL_VERSION_3_3)
	instanceBuffer.bind(GL_ARRAY_BUFFER);	
	
	if(elements == 16) {
		// Matrix4x4
		for(uint_fast8_t i=0; i<elements/4; ++i) {
			GLuint attribLocation = static_cast<GLuint> (location + i);
			uint8_t * data = 0;
			data += i*4*sizeof(GLfloat);
			glVertexAttribPointer(attribLocation, 4, GL_FLOAT, GL_FALSE, elements*sizeof(GLfloat), data);
			glEnableVertexAttribArray(attribLocation);
			glVertexAttribDivisor(attribLocation, 1);
		}
	} else if(elements <= 4) {
		GLuint attribLocation = static_cast<GLuint> (location);
		glVertexAttribPointer(attribLocation, elements, GL_FLOAT, GL_FALSE, elements*sizeof(GLfloat), nullptr);
		glEnableVertexAttribArray(attribLocation);
		glVertexAttribDivisor(attribLocation, 1);
	} else {
		WARN("Invalid number of elements for instancing (supported are 1,2,3,4 or 16 elements).");
		return;
	}
#endif
}

void disableInstanceBuffer(RenderingContext & rc, BufferObject & instanceBuffer, int32_t location, uint32_t elements) {
	if(location < 0)
		return;
		
#if defined(LIB_GL) && defined(GL_VERSION_3_3)
	if(elements == 16) {
		// Matrix4x4
		for(uint_fast8_t i=0; i<elements/4; ++i) {
			GLuint attribLocation = static_cast<GLuint> (location + i);
			glDisableVertexAttribArray(attribLocation);
			glVertexAttribDivisor(attribLocation, 0);
		}
	} else {
		glVertexAttribDivisor(static_cast<GLuint> (location), 0);
	}
	
	instanceBuffer.unbind(GL_ARRAY_BUFFER);
#endif
}

void drawInstances(RenderingContext & rc, Mesh* m, uint32_t firstElement, uint32_t elementCount, uint32_t instanceCount) {		
		if( m->empty())
			return;		
			
#if defined(LIB_GL) && defined(GL_VERSION_3_3)

		rc.applyChanges();

		MeshVertexData & vd=m->_getVertexData();		
		if(!vd.isUploaded())
			vd.upload();			
			
		vd.bind(rc, vd.isUploaded());			
		
		if(m->isUsingIndexData()) {			
			MeshIndexData & id=m->_getIndexData();								
			if(!id.isUploaded())
				id.upload();	
							
			BufferObject indexBuffer;
			id._swapBufferObject(indexBuffer);
			indexBuffer.bind(GL_ELEMENT_ARRAY_BUFFER);
			
			glDrawElementsInstanced(m->getGLDrawMode(), elementCount > 0 ? std::min(elementCount,id.getIndexCount()) : id.getIndexCount(), 
					GL_UNSIGNED_INT, reinterpret_cast<void*>(sizeof(GLuint)*firstElement), instanceCount);
					
			indexBuffer.unbind(GL_ELEMENT_ARRAY_BUFFER);
			id._swapBufferObject(indexBuffer);
		} else {
			glDrawArraysInstanced(m->getGLDrawMode(), firstElement, elementCount, instanceCount);
		}
		
		vd.unbind(rc, vd.isUploaded());
#else
	WARN("Instancing is not supported.");
#endif
}

}
