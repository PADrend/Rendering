/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef UNIFORMSET_H
#define UNIFORMSET_H

#include "Uniform.h"
#include <Util/StringIdentifier.h>
#include <cstdint>
#include <list>
#include <unordered_map>

namespace Rendering {
class Shader;


/*! (internal) Collection of Uniforms. Objects of this class are internally used by Shaders to track their Uniforms and
	by the RenderingContext, which has one instance for managing global uniforms.
	@ingroup shader */
class UniformRegistry {
	private:
		struct entry_t;

		typedef uint64_t step_t;
		typedef std::list<entry_t *> orderedEntries_t;
		typedef std::unordered_map<Util::StringIdentifier,entry_t *> uniformRegistry_t;

		static step_t globalUniformUpdateCounter;
		static step_t getNewGlobalStep(){	return ++globalUniformUpdateCounter;	}

		step_t stepOfLastApply; // =0
		step_t stepOfLastGlobalSync; // =0

		uniformRegistry_t uniforms; // collection of all known uniform-entries
		orderedEntries_t orderedList; // ordered list of all entries (when an entry is updated, it is moved to the front)

		struct entry_t{
			Uniform uniform;
			bool valid;
			bool warnIfUnused;
			step_t stepOfLastSet;
			int32_t location;
			int32_t set;

			orderedEntries_t::iterator positionInUpdateList;

			//! (ctor)
			entry_t(Uniform u,bool _warn,step_t step) : uniform(std::move(u)),valid(true),warnIfUnused(_warn),stepOfLastSet(step),location(-1),set(-1) {}
			void reset(const Uniform & u,step_t step,bool warn,const orderedEntries_t::iterator & it) {
				uniform = u;
				valid = true;
				warnIfUnused = warn;
				stepOfLastSet = step;
				positionInUpdateList = it;
			}
		};

		entry_t * getEntry(const Util::StringIdentifier nameId){
			const uniformRegistry_t::const_iterator it(uniforms.find(nameId));
			return it==uniforms.end() ? nullptr : it->second;
		}

		friend class Shader;

	public:

		//! (ctor)
		UniformRegistry();
		//! (dtor)
		~UniformRegistry();

		void clear();

		//! This forces all uniforms to be re-applied. Call this after the Shader has changed somehow.
		void resetCounters()	{	stepOfLastApply=stepOfLastGlobalSync=0;	}

		const Uniform & getUniform(const Util::StringIdentifier nameId)const{
			const uniformRegistry_t::const_iterator it(uniforms.find(nameId));
			return (it == uniforms.end() || !it->second->valid) ? Uniform::nullUniform : // no entry or invalid entry --> return nullUniform
				it->second->uniform;
		}

		//! returns true if a uniform with the given name has already been set, but the appliance failed.
		bool isInvalid(const Util::StringIdentifier nameId)const {
			const uniformRegistry_t::const_iterator it(uniforms.find(nameId));
			return it == uniforms.end() ? false : !it->second->valid;
		}

		//! Transfer all uniforms that have been changed in the globalUniforms since the last stepOfLastGlobalSync
		void performGlobalSync(const UniformRegistry & globalUniforms, bool forced);

		void setUniform(const Uniform & uniform, bool warnIfUnused=false, bool forced=false);
};

}
#endif // UNIFORMSET_H
