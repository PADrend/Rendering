/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "DrawCompound.h"
#include "Mesh/Mesh.h"
#include "MeshUtils/MeshBuilder.h"
#include "MeshUtils/MeshUtils.h"
#include "MeshUtils/PlatonicSolids.h"
#include "RenderingContext/RenderingParameters.h"
#include "RenderingContext/RenderingContext.h"
#include "GLHeader.h"
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <Geometry/Frustum.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Vec3.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <deque>
#include <cstdint>

namespace Rendering {

void drawCamera(RenderingContext & rc, const Util::Color4f & color) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		std::deque<Mesh *> meshes;
		std::deque<Geometry::Matrix4x4f> transformations;

		{
			VertexDescription vertexDescription;
			vertexDescription.appendPosition3D();
			vertexDescription.appendNormalFloat();
			Geometry::Box box(Geometry::Vec3f(0.0f, 0.0f, 0.1f), 0.2f, 0.5f, 0.8f);
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			transformations.push_back(Geometry::Matrix4x4());
		}
		{
			// Lens
			meshes.push_back(MeshUtils::MeshBuilder::createConicalFrustum(0.1f, 0.25f, 0.2f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(0.0f, 0.0f, -0.3f);
			mat.rotate_deg(90.0f, 0.0f, 1.0f, 0.0f);
			transformations.push_back(mat);
		}
		{
			// Lens cap
			meshes.push_back(MeshUtils::MeshBuilder::createDiscSector(0.25f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(0.0f, 0.0f, -0.5f);
			mat.rotate_deg(-90.0f, 0.0f, 1.0f, 0.0f);
			transformations.push_back(mat);
		}

		{
			// First film reel
			meshes.push_back(MeshUtils::MeshBuilder::createConicalFrustum(0.2f, 0.2f, 0.1f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(-0.05f, 0.45f, -0.1f);
			transformations.push_back(mat);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createDiscSector(0.2f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(-0.05f, 0.45f, -0.1f);
			transformations.push_back(mat);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createDiscSector(0.2f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(0.05f, 0.45f, -0.1f);
			mat.rotate_deg(180.0f, 0.0f, 1.0f, 0.0f);
			transformations.push_back(mat);
		}

		{
			// Second film reel
			meshes.push_back(MeshUtils::MeshBuilder::createConicalFrustum(0.2f, 0.2f, 0.1f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(-0.05f, 0.45f, 0.3f);
			transformations.push_back(mat);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createDiscSector(0.2f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(-0.05f, 0.45f, 0.3f);
			transformations.push_back(mat);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createDiscSector(0.2f, 16));
			Geometry::Matrix4x4f mat;
			mat.translate(0.05f, 0.45f, 0.3f);
			mat.rotate_deg(180.0f, 0.0f, 1.0f, 0.0f);
			transformations.push_back(mat);
		}

		mesh = MeshUtils::combineMeshes(meshes, transformations);
	}

	rc.pushAndSetLighting(LightingParameters(false));
	rc.pushAndSetColorMaterial(Util::Color4f(color));
	rc.displayMesh(mesh.get());
	rc.popMaterial();
	rc.popLighting();
}

void drawCoordSys(RenderingContext & rc, float scale) {
	static Util::Reference<Mesh> arrow;
	static Util::Reference<Mesh> sphere;
	static Util::Reference<Mesh> charX;
	static Util::Reference<Mesh> charY;
	static Util::Reference<Mesh> charZ;
	const float radius = 0.025f;
	if (arrow.isNull()) {
		std::deque<Mesh *> meshes;
		std::deque<Geometry::Matrix4x4> transformations;

		Geometry::Matrix4x4 transform;

		meshes.push_back(MeshUtils::MeshBuilder::createConicalFrustum(radius, radius, 0.7f, 16));
		transformations.push_back(transform);

		meshes.push_back(MeshUtils::MeshBuilder::createConicalFrustum(radius, 2.0f * radius, 0.01f, 16));
		transform.translate(0.7f, 0.0f, 0.0f);
		transformations.push_back(transform);

		meshes.push_back(MeshUtils::MeshBuilder::createCone(2.0f * radius, 0.29f, 16));
		transform.translate(0.01f, 0.0f, 0.0f);
		transformations.push_back(transform);

		arrow = MeshUtils::combineMeshes(meshes, transformations);
		MeshUtils::optimizeIndices(arrow.get());

		while (!meshes.empty()) {
			delete meshes.back();
			meshes.pop_back();
		}
	}
	if (sphere.isNull()) {
		Util::Reference<Mesh> icosahedron = MeshUtils::PlatonicSolids::createIcosahedron();
		sphere = MeshUtils::PlatonicSolids::createEdgeSubdivisionSphere(icosahedron.get(), 2);
		Geometry::Matrix4x4 transform;
		transform.scale(1.1f * radius);
		MeshUtils::transform(sphere.get()->openVertexData(), transform);
	}
	if(charX.isNull()) {
		std::deque<Mesh *> meshes;
		std::deque<Geometry::Matrix4x4> transformations;

		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();

		const Geometry::Box box(Geometry::Vec3f(0.0f, 0.0f, 0.0f), 0.02f, 0.2f, 0.05f);
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(1.2f, 0.0f, 0.0f);
			transform.rotate_deg(30.0f, 0.0f, 0.0f, -1.0f);
			transformations.push_back(transform);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(1.2f, 0.0f, 0.0f);
			transform.rotate_deg(-30.0f, 0.0f, 0.0f, -1.0f);
			transformations.push_back(transform);
		}
		charX = MeshUtils::combineMeshes(meshes, transformations);
		MeshUtils::optimizeIndices(charX.get());

		while(!meshes.empty()) {
			delete meshes.back();
			meshes.pop_back();
		}
	}
	if(charY.isNull()) {
		std::deque<Mesh *> meshes;
		std::deque<Geometry::Matrix4x4> transformations;

		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();

		const Geometry::Box box(Geometry::Vec3f(0.0f, 0.0f, 0.0f), 0.02f, 0.1f, 0.05f);
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(0.025f, 0.045f, 0.0f);
			transform.rotate_deg(30.0f, 0.0f, 0.0f, -1.0f);
			transformations.push_back(transform);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(-0.025f, 0.045f, 0.0f);
			transform.rotate_deg(-30.0f, 0.0f, 0.0f, -1.0f);
			transformations.push_back(transform);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(0.0f, -0.045f, 0.0f);
			transformations.push_back(transform);
		}
		charY = MeshUtils::combineMeshes(meshes, transformations);
		Geometry::Matrix4x4 transform;
		transform.translate(1.2f, 0.0f, 0.0f);
		transform.rotate_deg(90.0f, 0.0f, 0.0f, -1.0f);
		MeshUtils::transform(charY->openVertexData(), transform);
		MeshUtils::optimizeIndices(charY.get());

		while(!meshes.empty()) {
			delete meshes.back();
			meshes.pop_back();
		}
	}
	if(charZ.isNull()) {
		std::deque<Mesh *> meshes;
		std::deque<Geometry::Matrix4x4> transformations;

		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();

		const Geometry::Box box(Geometry::Vec3f(0.0f, 0.0f, 0.0f), 0.02f, 0.1f, 0.05f);
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(1.2f, 0.075f, 0.0f);
			transform.rotate_deg(90.0f, 0.0f, 0.0f, -1.0f);
			transformations.push_back(transform);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(1.2f, 0.0f, 0.0f);
			transform.rotate_deg(-30.0f, 0.0f, 0.0f, -1.0f);
			transform.scale(1.0f, 1.6f, 1.0f);
			transformations.push_back(transform);
		}
		{
			meshes.push_back(MeshUtils::MeshBuilder::createBox(vertexDescription, box));
			Geometry::Matrix4x4 transform;
			transform.translate(1.2f, -0.075f, 0.0f);
			transform.rotate_deg(-90.0f, 0.0f, 0.0f, -1.0f);
			transformations.push_back(transform);
		}
		charZ = MeshUtils::combineMeshes(meshes, transformations);
		MeshUtils::optimizeIndices(charZ.get());

		while(!meshes.empty()) {
			delete meshes.back();
			meshes.pop_back();
		}
	}
	// Origin
	rc.pushAndSetColorMaterial(Util::ColorLibrary::WHITE);
	rc.displayMesh(sphere.get());
	rc.popMaterial();

	// X axis
	Geometry::Matrix4x4 transform;
	transform.scale(scale, 1.0f, 1.0f);
	rc.pushMatrix();
	rc.multMatrix(transform);
	rc.pushAndSetColorMaterial(Util::ColorLibrary::RED);
	rc.displayMesh(arrow.get());
	rc.displayMesh(charX.get());
	rc.popMaterial();
	rc.popMatrix();
	// Y axis
	transform.setIdentity();
	transform.scale(1.0f, scale, 1.0f);
	transform.rotate_deg(90.0f, 0.0f, 0.0f, 1.0f);
	rc.pushMatrix();
	rc.multMatrix(transform);
	rc.pushAndSetColorMaterial(Util::ColorLibrary::GREEN);
	rc.displayMesh(arrow.get());
	rc.displayMesh(charY.get());
	rc.popMaterial();
	rc.popMatrix();
	// Z axis
	transform.setIdentity();
	transform.scale(1.0f, 1.0f, scale);
	transform.rotate_deg(90.0f, 0.0f, -1.0f, 0.0f);
	rc.pushMatrix();
	rc.multMatrix(transform);
	rc.pushAndSetColorMaterial(Util::ColorLibrary::BLUE);
	rc.displayMesh(arrow.get());
	rc.displayMesh(charZ.get());
	rc.popMaterial();
	rc.popMatrix();
}

void drawFrustum(RenderingContext & rc, const Geometry::Frustum & frustum, const Util::Color4f & color, float lineWidth) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		mesh = new Mesh(vertexDescription, 8, 16);
		mesh->setDrawMode(Mesh::DRAW_LINE_STRIP);

		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
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
	}

	MeshVertexData & vd = mesh->openVertexData();
	float * vertices = reinterpret_cast<float *>(vd.data());
	for (uint_fast8_t c = 0; c < 8; ++c) {
		const Geometry::Vec3 & corner = frustum[static_cast<Geometry::corner_t> (c)];
		*vertices++ = corner.getX();
		*vertices++ = corner.getY();
		*vertices++ = corner.getZ();
	}
	vd.updateBoundingBox();
	vd.markAsChanged();

	rc.pushAndSetLine(lineWidth);
	rc.pushAndSetLighting(LightingParameters(false));
	rc.pushAndSetColorMaterial(color);
	rc.displayMesh(mesh.get());
	rc.popMaterial();
	rc.popLighting();
	rc.popLine();
}

void drawGrid(RenderingContext & rc, float scale) {
	static Util::Reference<Mesh> mesh;
	if (mesh.isNull()) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		mesh = new Mesh(vertexDescription, 4 * 101, 4 * 101);
		mesh->setDrawMode(Mesh::DRAW_LINES);
		
		MeshVertexData & vd = mesh->openVertexData();
		float * vertices = reinterpret_cast<float *> (vd.data());
		MeshIndexData & id = mesh->openIndexData();
		uint32_t * indices = id.data();
		uint32_t nextIndex = 0;
		const float step = 1.0f / 100.0f;
		for (uint_fast8_t line = 0; line < 101; ++line) {
			const float pos = -0.5f + static_cast<float> (line) * step;

			*vertices++ = -0.5f;
			*vertices++ = 0.0f;
			*vertices++ = pos;

			*vertices++ = 0.5f;
			*vertices++ = 0.0f;
			*vertices++ = pos;

			*indices++ = nextIndex++;
			*indices++ = nextIndex++;

			*vertices++ = pos;
			*vertices++ = 0.0f;
			*vertices++ = -0.5f;

			*vertices++ = pos;
			*vertices++ = 0.0f;
			*vertices++ = 0.5f;

			*indices++ = nextIndex++;
			*indices++ = nextIndex++;

		}
		vd.updateBoundingBox();
		vd.markAsChanged();
		id.updateIndexRange();
		id.markAsChanged();
	}

	Geometry::Matrix4x4 matrix;
	matrix.scale(scale);
	rc.pushMatrix();
	rc.multMatrix(matrix);
	rc.displayMesh(mesh.get());
	rc.popMatrix();
}

}
