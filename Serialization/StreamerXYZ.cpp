/*
	This file is part of the Rendering library.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerXYZ.h"
#include "Serialization.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexDescription.h"
#include "../MeshUtils/MeshBuilder.h"
#include <Geometry/Vec3.h>
#include <Geometry/PointOctree.h>
#include <Util/GenericAttribute.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>
#include <istream>
#include <random>
#include <limits>

namespace Rendering {
namespace Serialization {

const char * const StreamerXYZ::fileExtension = "xyz";

Mesh * StreamerXYZ::loadMesh(std::istream & input, std::size_t numPoints) {
	VertexDescription vertexDesc;
	vertexDesc.appendPosition3D();
	vertexDesc.appendColorRGBAByte();

	struct Point {
		float x, y, z;
		uint8_t r, g, b, a;
		Point(float _x, float _y, float _z,
			  uint8_t _r, uint8_t _g, uint8_t _b) :
			x(_x), y(_y), z(_z),
			r(_r), g(_g), b(_b), a(255) {
		}
	};
	if(sizeof(Point) != vertexDesc.getVertexSize()) {
		WARN("Different vertex sizes.");
		FAIL();
	}
	std::vector<Point> points;

	float x, y, z;
	uint16_t r, g, b;
	while(input.good()) {
		input >> x >> y >> z >> r >> g >> b;
		points.emplace_back(x, y, z, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
		
		if(numPoints != 0 && points.size() >= numPoints) {
			break;
		}
	}

	auto mesh = new Mesh(vertexDesc, static_cast<uint32_t>(points.size()), 0);

	MeshVertexData & vd = mesh->openVertexData();
	std::copy(points.data(), points.data() + points.size(), reinterpret_cast<Point *>(vd.data()));
	vd.markAsChanged();
	vd.updateBoundingBox();

	mesh->setDrawMode(Mesh::DRAW_POINTS);
	mesh->setUseIndexData(false);
	return mesh;
}

Util::GenericAttributeList * StreamerXYZ::loadGeneric(std::istream & input) {
	auto list = new Util::GenericAttributeList;
	while(input.good()) {
		Mesh * mesh = loadMesh(input, 1000000);
		Util::GenericAttributeMap * desc = Serialization::createMeshDescription(mesh);
		list->push_back(desc);
	}
	return list;
}

uint8_t StreamerXYZ::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_MESH | CAP_LOAD_GENERIC;
	} else {
		return 0;
	}
}

//! (static)
void StreamerXYZ::clusterPoints( const Util::FileName & inputFile, size_t numberOfClusters ){
	auto input = Util::FileUtils::openForReading(inputFile);
	FAIL_IF(!input->good());

	std::vector<std::unique_ptr<std::ostream> > outputHolder;
	std::vector<std::ostream* > outputs;
	
	std::string baseFileName;
	{
		Util::FileName f(inputFile);
		f.setEnding("");
		baseFileName = f.toString();
	}
	
	for(size_t i=0;i<numberOfClusters;++i){
		std::ostringstream outFileName;
		outFileName<<baseFileName<<"_"<<i<<".xyz";
		outputHolder.emplace_back(Util::FileUtils::openForWriting(Util::FileName(outFileName.str())));
		outputs.push_back(outputHolder.back().get());
	}
	clusterPoints(*input,outputs);
}

//! (static)
void StreamerXYZ::clusterPoints( std::istream & input, std::vector<std::ostream*> & outputs ){
	std::default_random_engine engine;
	const size_t numClusters = outputs.size();
	const size_t numSamples = numClusters * 100;
	
//	input.seekg( 0, std::ios::end);
//	const uint64_t fileSize = static_cast<uint64_t>(input.tellg());

	input.seekg( 0, std::ios::beg);
	uint64_t fileSize = 0;
	while(true){
		input.seekg( (1024*1024), std::ios::cur);
		if(!input.good())
			break;
		fileSize+=1024*1024;
		std::cout << ".";
	}
	input.clear(std::ios::eofbit);
	input.seekg( 0, std::ios::beg);
	FAIL_IF(!input.good());
		
	std::cout << "FileSize: "<<fileSize <<"\n";
	std::cout << "FileSize: "<<fileSize/(1024*1024) <<"MB\n";
	
	std::vector<Geometry::Vec3> allSamples;
	
	{ // collect bunch of samples from file
		std::cout << "Taking "<<numSamples<<" samples...\n";
		std::vector<uint64_t> sampleLocations;
		auto distribution = std::uniform_int_distribution<uint64_t>(0,fileSize > 200 ? fileSize-200 : fileSize); // do not jump to the end (heuristic!)
		for(size_t i=0;i<numSamples;++i){
			sampleLocations.push_back(distribution(engine));
		}
		uint64_t cursor = 0;
		std::sort(sampleLocations.begin(),sampleLocations.end());
		for(const auto & location : sampleLocations){
			input.seekg( location-cursor, std::ios::cur);
			if(!input.good())
				break;
			std::string garbage;
			std::getline(input,garbage,'\n');

			if(!input.good())
				break;
			cursor = location + garbage.length(); // + the current line; we just ignore this...

			float x, y, z;
			input >> x >> y >> z;
			allSamples.emplace_back(x,y,z);
		}
		input.clear(std::ios::eofbit);
		input.seekg( 0, std::ios::beg);
	}
	FAIL_IF(!input.good());

	std::vector<Geometry::Vec3> clusterCenters;

	{	// select "best" samples as cluster centers
		std::cout << "Selecting cluster centers..\n";
		class Point{
				Geometry::Vec3 pos;
			public:
				Point(Geometry::Vec3 & _pos) : pos(_pos){}
				const Geometry::Vec3 & getPosition()const	{	return pos;	}
		};
		Geometry::Box bb;
		bb.invalidate();
		for(const auto & p:allSamples)
			bb.include(p);
		bb.resizeRel(1.01f);
		
		Geometry::PointOctree<Point> octree(bb,bb.getExtentMax()*0.1f,5);
		
		for(size_t i=0;i<numClusters && !allSamples.empty();++i ){
			if(clusterCenters.empty()){
				clusterCenters.emplace_back(allSamples.back());
				octree.insert(allSamples.back());
				allSamples.pop_back();
				continue;
			}else{
				std::vector<Geometry::Vec3>::iterator bestIt;
				float bestDistance=-1;

				for(auto it = allSamples.begin();it!=allSamples.end();++it){
					std::deque<Point> nearest;
					octree.getClosestPoints(*it, 1, nearest);
					FAIL_IF(nearest.empty());
					const float dist = nearest.front().getPosition().distanceSquared(*it);
					if(dist>bestDistance){
						bestDistance = dist;
						bestIt = it;
					}
				}
				clusterCenters.emplace_back(*bestIt);
				octree.insert(*bestIt);
				allSamples.erase(bestIt);
			}
		}
		for(const auto & p : clusterCenters ){
			std::cout << p<<" ";
		}
		std::cout << "\n";
	}
	
	
	{ // distribute points
		std::cout << "Distribute points..\n";
		uint64_t pointCounter = 0;
		uint64_t dataCounter = 0;
		float x, y, z;
		while(input.good()) {
			std::string line;
			std::getline(input,line);
			std::istringstream lineInput(line);
			
			dataCounter+=line.length()+1;
			lineInput >> x >> y >> z;

			float closestDist = std::numeric_limits<float>::max();
			auto outStreamIt = outputs.begin();
			auto selectedStreamIt = outStreamIt;
			for(const auto & center : clusterCenters){
				const float dist = center.distanceSquared( Geometry::Vec3(x,y,z));
				if(dist<closestDist){
					closestDist = dist;
					selectedStreamIt = outStreamIt;
				}
				++outStreamIt;
			}
			**selectedStreamIt << line;
			
			++pointCounter;
			if( (pointCounter%1000000) == 0){
				std::cout << "Point #"<<pointCounter<<"\t"<< static_cast<double>(dataCounter) / fileSize<<"% \n";
			}
		}
		
	}
	std::cout << "Done.\n";
	
}
}
}
