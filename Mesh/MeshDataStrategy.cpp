/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshDataStrategy.h"
#include "Mesh.h"
#include "MeshIndexData.h"
#include "MeshVertexData.h"
#include "../RenderingContext.h"
#include "../Core/Common.h"
#include <Util/Macros.h>
#include <iostream>

namespace Rendering {

//! (static)
MeshDataStrategy * MeshDataStrategy::defaultStrategy = nullptr;

//! (static)
MeshDataStrategy * MeshDataStrategy::getDefaultStrategy(){
	if(defaultStrategy == nullptr) {
		defaultStrategy = SimpleMeshDataStrategy::getStaticDrawReleaseLocalStrategy();
	}
	return defaultStrategy;
}

//! (static)
void MeshDataStrategy::setDefaultStrategy(MeshDataStrategy * newDefault){
	defaultStrategy = newDefault;
}

// -------------

//! (static,internal)
void MeshDataStrategy::doDisplayMesh(RenderingContext & context, Mesh * m,uint32_t startIndex,uint32_t indexCount) {
	context.setPrimitiveTopology(m->getTopology());
	if(m->isUsingIndexData()){
		MeshVertexData & vd=m->_getVertexData();
		MeshIndexData & id=m->_getIndexData();

		vd.bind(context);
		id.draw(context, startIndex, indexCount);
	}else{
		MeshVertexData & vd=m->_getVertexData();
		vd.draw(context, startIndex, indexCount);
	}
}

// ------------------------------------------------------------------------------------

//! (static)
SimpleMeshDataStrategy * SimpleMeshDataStrategy::getStaticDrawReleaseLocalStrategy(){
	static SimpleMeshDataStrategy strategy( USE_VBOS );
	return &strategy;
}

//! (static)
SimpleMeshDataStrategy * SimpleMeshDataStrategy::getDebugStrategy(){
	static SimpleMeshDataStrategy strategy( USE_VBOS|DEBUG_OUTPUT );
	return &strategy;
}

//! (static)
SimpleMeshDataStrategy * SimpleMeshDataStrategy::getStaticDrawPreserveLocalStrategy(){
	static SimpleMeshDataStrategy strategy( USE_VBOS|PRESERVE_LOCAL_DATA );
	return &strategy;
}

//! (static)
SimpleMeshDataStrategy * SimpleMeshDataStrategy::getDynamicVertexStrategy(){
	static SimpleMeshDataStrategy strategy( USE_VBOS|PRESERVE_LOCAL_DATA|DYNAMIC_VERTICES );
	return &strategy;
}

//! (static)
SimpleMeshDataStrategy * SimpleMeshDataStrategy::getPureLocalStrategy(){
	static SimpleMeshDataStrategy strategy( 0 );
	return &strategy;
}

// ----

/*! (ctor)	*/
SimpleMeshDataStrategy::SimpleMeshDataStrategy(const uint8_t _flags ) :
		flags(_flags) {
	//ctor
}

/*! (dtor)	*/
SimpleMeshDataStrategy::~SimpleMeshDataStrategy(){
//	std::cout << " ~ds ";
	//dtor
}


//! ---|> MeshDataStrategy
void SimpleMeshDataStrategy::assureLocalVertexData(Mesh * m){
	MeshVertexData & vd=m->_getVertexData();

	if( vd.dataSize()==0 && vd.isUploaded())
		vd.download();
}

//! ---|> MeshDataStrategy
void SimpleMeshDataStrategy::assureLocalIndexData(Mesh * m){
	MeshIndexData & id=m->_getIndexData();

	if( id.dataSize()==0 && id.isUploaded())
		id.download();
}

//! ---|> MeshDataStrategy
void SimpleMeshDataStrategy::prepare(Mesh * m){
	if(!getFlag(USE_VBOS))
		return;

	MeshIndexData & id=m->_getIndexData();
	if( id.empty() && id.isUploaded() ){ // "old" VBO present, although data has been removed
		if(getFlag(DEBUG_OUTPUT))	std::cout << " ~idxBO";
		id.removeGlBuffer();
	} else if( !id.empty() && (id.hasChanged() || !id.isUploaded()) ){ // data has changed or is new
		if(getFlag(DEBUG_OUTPUT))	std::cout << " +idxBO";
		id.upload(MemoryUsage::GpuOnly);
	}
	if(!getFlag(PRESERVE_LOCAL_DATA) && id.isUploaded() && id.hasLocalData()){
		if(getFlag(DEBUG_OUTPUT))	std::cout << " ~idxLD";
		id.releaseLocalData();
	}

	MeshVertexData & vd=m->_getVertexData();
	if( vd.empty() && vd.isUploaded() ){ // "old" VBO present, although data has been removed
		if(getFlag(DEBUG_OUTPUT))	std::cout << " ~vBO";
		vd.removeGlBuffer();
	} else if( !vd.empty() && (vd.hasChanged() || !vd.isUploaded()) ){ // data has changed or is new
		if(getFlag(DEBUG_OUTPUT))	std::cout << " +vBO";
		vd.upload( getFlag(DYNAMIC_VERTICES) ? MemoryUsage::CpuToGpu : MemoryUsage::GpuOnly );
	}
	if(!getFlag(PRESERVE_LOCAL_DATA) && vd.isUploaded() && vd.hasLocalData()){
		if(getFlag(DEBUG_OUTPUT))	std::cout << " ~vLD";
		vd.releaseLocalData();
	}

}

//! ---|> MeshDataStrategy
void SimpleMeshDataStrategy::displayMesh(RenderingContext & context, Mesh * m,uint32_t startIndex,uint32_t indexCount){
	if( !m->empty() )
		MeshDataStrategy::doDisplayMesh(context,m,startIndex,indexCount);
}

}
