/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "UniformRegistry.h"
#include <Util/Macros.h>

namespace Rendering {

//! (static)
UniformRegistry::step_t UniformRegistry::globalUniformUpdateCounter(1); // start with 1 to make sure 0 means 'never' (and not 'initially')

//! (ctor)
UniformRegistry::UniformRegistry() : stepOfLastApply(0),stepOfLastGlobalSync(0){
}

//! (dtor)
UniformRegistry::~UniformRegistry(){
	clear();
}


void UniformRegistry::clear(){
	for(auto & entry : uniforms) {
		delete entry.second;
	}
	uniforms.clear();
	orderedList.clear();
	resetCounters();
}

void UniformRegistry::performGlobalSync(const UniformRegistry & globalUniforms, bool forced){
	if(forced){
		// set all global uniforms
		for(auto it=globalUniforms.orderedList.begin();	it!=globalUniforms.orderedList.end() && (*it)->stepOfLastSet>stepOfLastGlobalSync; ++it )
			setUniform( (*it)->uniform,false,true);
	}else {
		// set all uniforms of the globalUniforms-Set that have been changed since the last call.
		for(auto it=globalUniforms.orderedList.begin();	it!=globalUniforms.orderedList.end() && (*it)->stepOfLastSet>stepOfLastGlobalSync; ++it )
			setUniform( (*it)->uniform,false,false);
	}
	stepOfLastGlobalSync = getNewGlobalStep();
}

void UniformRegistry::setUniform(const Uniform & uniform, bool warnIfUnused, bool forced){
	entry_t * & entry = uniforms[uniform.getNameId()];

	if(entry==nullptr){ // new entry
		entry = new entry_t( uniform,warnIfUnused,getNewGlobalStep() );
		orderedList.push_front(entry);
		entry->positionInUpdateList=orderedList.begin();
	} // if an entry exists and appliance is forced or (uniform is valid and value has changed)
	else if( forced || (entry->valid && !(uniform==entry->uniform)) ){

		//! \note This warning should do no harm - otherwise remove it.
		//if(entry->uniform.getType()!=uniform.getType() ){
		//	WARN("Type of Uniform changed; this may be a problem. "+entry->uniform.toString()+" -> "+uniform.toString());
		//}
		// move entry to the front of the orderedList
		orderedList.erase(entry->positionInUpdateList);
		orderedList.push_front(entry);

		entry->reset(uniform,getNewGlobalStep(),warnIfUnused,orderedList.begin());
	}
	// else: if the value of an uniform has not changed or the uniform could not be set (= invalid), nothing needs to be done.
}

}
