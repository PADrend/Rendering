/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerOBJ.h"
#include "Serialization.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include "../MeshUtils/MeshUtils.h"
#include <Util/GenericAttribute.h>
#include <Util/StringUtils.h>
#include <list>
#include <map>
#include <set>

using namespace Util;

namespace Rendering {
namespace Serialization {

const char * const StreamerOBJ::fileExtension = "obj";

struct Vertex {

	Vertex() :
		data(), index(0) {
	}

	Vertex(const Vertex & other) :
		data(other.data), index(other.index) {
	}

	Vertex(float pos[3], float nor[3], float tex[2]) :
		data(), index(0) {
		size_t size = 3;
		if (nor != nullptr) {
			size += 3;
		}
		if (tex != nullptr) {
			size += 2;
		}
		data.reserve(size);
		data.push_back(pos[0]);
		data.push_back(pos[1]);
		data.push_back(pos[2]);
		if (nor != nullptr) {
			data.push_back(nor[0]);
			data.push_back(nor[1]);
			data.push_back(nor[2]);
		}
		if (tex != nullptr) {
			// Texture offsets are ignored here.
			data.push_back(tex[0]);
			data.push_back(tex[1]);
		}
	}

	Vertex & operator=(const Vertex & other) {
		data = other.data;
		index = other.index;
		return *this;
	}

	bool operator<(const Vertex & other) const {
		// Do not compare index here.
		return data < other.data;
	}

	std::vector<float> data;
	uint32_t index;
};

static void triangulate(std::list<uint32_t> & l) {
	if (l.size() <= 3) {
		if(l.size() < 3) {
			WARN("cannot triangulate list with < 3 entries");
			l.clear();
		}
		return;
	}
	// Copy list.
	std::list<uint32_t> old(l);
	l.clear();

	uint32_t a, b, c;
	a = old.front();
	old.pop_front();
	b = old.front();
	old.pop_front();
	c = old.front();
	old.pop_front();
	l.push_back(a);
	l.push_back(b);
	l.push_back(c);
	while (!old.empty()) {
		b = c;
		c = old.front();
		old.pop_front();
		l.push_back(a);
		l.push_back(b);
		l.push_back(c);
	}
}


static Mesh * createMesh(VertexDescription * vertexDesc, const std::list<uint32_t> & faces, const std::set<Vertex> & vertexSet) {
	Util::Reference<Mesh> mesh = new Mesh;


	// Create index data.
	std::size_t indexCount = faces.size();

	MeshIndexData & indices = mesh->openIndexData();
	indices.allocate(indexCount);
	uint32_t * indexData = indices.data();


	// Mapping is done because one face list may reference only a small subset of the vertices.
	std::map<uint32_t, uint32_t> indexMap;

	uint32_t newIndex = 0;
	for(const auto & face : faces) {
		auto find = indexMap.lower_bound(face);
		if(find == indexMap.end() || indexMap.key_comp()(face, find->first)) {
			find = indexMap.insert(find, std::make_pair(face, newIndex));
			++newIndex;
		}
		*indexData = find->second;
		++indexData;
	}
	indices.updateIndexRange();

	// Create vertex data.
	MeshVertexData & vertices = mesh->openVertexData();
	vertices.allocate(indexMap.size(), *vertexDesc);

	const uint16_t posOffset = vertexDesc->getAttribute(VertexAttributeIds::POSITION).getOffset();
	const uint16_t texOffset = vertexDesc->getAttribute(VertexAttributeIds::TEXCOORD0).getOffset();
	const uint16_t norOffset = vertexDesc->getAttribute(VertexAttributeIds::NORMAL).getOffset();

	uint8_t * data = vertices.data();
	for(const auto & vertex : vertexSet) {
		auto find = indexMap.find(vertex.index);
		if (find != indexMap.end()) {
			// Copy position.
			auto end = std::next(vertex.data.begin(), 3);
			std::copy(vertex.data.begin(), end, reinterpret_cast<float *> (data + (find->second * vertexDesc->getVertexSize()) + posOffset));

			const bool texAvail = vertex.data.size() == 5 || vertex.data.size() == 8;
			const bool norAvail = vertex.data.size() == 6 || vertex.data.size() == 8;
			if(norAvail) {
				// Copy normal.
				auto begin = end;
				std::advance(end, 3);
				std::copy(begin, end, reinterpret_cast<float *> (data + (find->second * vertexDesc->getVertexSize()) + norOffset));
			}
			if(texAvail) {
				// Copy texture coordinate.
				auto begin = end;
				std::advance(end, 2);
				std::copy(begin, end, reinterpret_cast<float *> (data + (find->second * vertexDesc->getVertexSize()) + texOffset));
			}
		}
	}
	vertices.updateBoundingBox();

	MeshUtils::shrinkMesh(mesh.get());

	if (mesh->getVertexCount() == 0 || mesh->getIndexCount() == 0) {
		mesh = nullptr;
	}

	return mesh.detachAndDecrease();
}

Util::GenericAttributeList * StreamerOBJ::loadGeneric(std::istream & input) {
	auto descriptionList = new Util::GenericAttributeList;

	std::set<Vertex> vertices;


	// Insert initial items, because first index for OBJ is 1.
	std::vector<float> positions(3, 0.0f);
	std::vector<float> normals(3, 0.0f);
	std::vector<float> texcoords(2, 0.0f);

	std::list<uint32_t> faces;

	std::string currentMtl;
	std::list<std::string> mtlFiles;

	VertexDescription * vertexDesc = nullptr;

	char chars[256];

	while (input.getline(chars, 256)) {
		char * cursor = chars;
		StringUtils::stepWhitespaces(&cursor);
		if (*cursor == 'v') {
			++cursor;
			if (*cursor == ' ') {
				++cursor;
				float pos[3];
				pos[0] = strtof(cursor, &cursor);
				pos[1] = strtof(cursor, &cursor);
				pos[2] = strtof(cursor, &cursor);
				positions.insert(positions.end(), pos, pos + 3);
			} else if (*cursor == 't') {
				++cursor;
				float texCoord[2];
				texCoord[0] = strtof(cursor, &cursor);
				texCoord[1] = strtof(cursor, &cursor);
				texcoords.insert(texcoords.end(), texCoord, texCoord + 2);
			} else if (*cursor == 'n') {
				++cursor;
				float normal[3];
				normal[0] = strtof(cursor, &cursor);
				normal[1] = strtof(cursor, &cursor);
				normal[2] = strtof(cursor, &cursor);
				normals.insert(normals.end(), normal, normal + 3);
			}
		} else if (*cursor == 'f') {
			++cursor;
			int32_t v = 0, vt = 0, vn = 0;
			v = strtol(cursor, &cursor, 10);
			std::list<uint32_t> vertexIndexList;
			while (v != 0) {
				if (*cursor == '/') {
					++cursor;
					vt = strtol(cursor, &cursor, 10);
					if (*cursor == '/') {
						++cursor;
						vn = strtol(cursor, &cursor, 10);
					}
				}
				if (!vertexDesc) {
					vertexDesc = new VertexDescription;
					vertexDesc->appendPosition3D();
					if (vt != 0) {
						vertexDesc->appendTexCoord();
					}
					if (vn != 0) {
						vertexDesc->appendNormalFloat();
					}
				}

				float * tex = nullptr;
				float * nor = nullptr;
				if(v < 0)
					v = positions.size()/3 + v;
				if(vt < 0)
					vt = texcoords.size()/2 + vt;
				if(vn < 0)
					vn = normals.size()/3 + vn;
				
				if (vt != 0) {
					tex = &texcoords[2 * vt];
				}
				if (vn != 0) {
					nor = &normals[3 * vn];
				}
				uint32_t index;
				Vertex vertex(&positions[3 * v], nor, tex);
				auto iter = vertices.lower_bound(vertex);
				if (iter != vertices.end() && !(vertices.key_comp()(vertex, *iter))) {
					index = iter->index;
				} else {
					vertex.index = vertices.size();
					vertices.insert(iter, vertex);
					index = vertex.index;
				}

				vertexIndexList.push_back(index);

				v = strtoul(cursor, &cursor, 10);
			}
			triangulate(vertexIndexList);
			while (!vertexIndexList.empty()) {
				// Append one triangle.
				for (uint_fast8_t i = 0; i < 3; ++i) {
					faces.push_back(vertexIndexList.front());
					vertexIndexList.pop_front();
				}
			}
		} else if (StringUtils::beginsWith(cursor, "mtllib")) {
			cursor += 6;
			mtlFiles.push_back(StringUtils::trim(cursor));
		} else if (*cursor == 'g' || *cursor == 's') {
			if (!vertexDesc)
				continue;

			Mesh * mesh = createMesh(vertexDesc, faces, vertices);
			vertexDesc = nullptr;
			faces.clear();
			if(mesh) {
				Util::GenericAttributeMap * d = Serialization::createMeshDescription(mesh);
				d->setString(Serialization::DESCRIPTION_MATERIAL_NAME, currentMtl);
				descriptionList->push_back(d);
			}
		} else if (StringUtils::beginsWith(cursor, "usemtl")) {

			if (vertexDesc) {
				Mesh * mesh = createMesh(vertexDesc, faces, vertices);
				vertexDesc = nullptr;
				faces.clear();
				if(mesh) {
					Util::GenericAttributeMap * d = Serialization::createMeshDescription(mesh);
					d->setString(Serialization::DESCRIPTION_MATERIAL_NAME, currentMtl);
					descriptionList->push_back(d);
				}
			}
			cursor += 6;
			currentMtl = StringUtils::trim(cursor);
		} else if (*cursor != '#' && *cursor != '\0' && *cursor != '\n' && *cursor != '\r' && *cursor != 'o') {
			WARN(std::string("Unknown OBJ keyword \"") + *cursor + "\".");
		}
	}

	Mesh * mesh = createMesh(vertexDesc, faces, vertices);
	faces.clear();
	if(mesh) {
		Util::GenericAttributeMap * d = Serialization::createMeshDescription(mesh);
		d->setString(Serialization::DESCRIPTION_MATERIAL_NAME, currentMtl);
		descriptionList->push_back(d);
	}

	// Traverse list in reverse order to get right order again when using push_front below.
	for(auto it = mtlFiles.rbegin(); it != mtlFiles.rend(); ++it) {
		auto mtlFileDesc = new Util::GenericAttributeMap;
		mtlFileDesc->setString(Serialization::DESCRIPTION_TYPE, Serialization::DESCRIPTION_TYPE_MATERIAL);
		mtlFileDesc->setString(Serialization::DESCRIPTION_FILE, *it);
		// Make sure that the material descriptions are at the front!
		descriptionList->push_front(mtlFileDesc);
	}

	return descriptionList;
}

uint8_t StreamerOBJ::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_GENERIC;
	} else {
		return 0;
	}
}

}
}
