/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "QueryObject.h"
#include "Helper.h"
#include <Util/Macros.h>
#include <algorithm>
#include <deque>

namespace Rendering {

bool QueryObject::isResultAvailable() const {
#if defined(LIB_GL)
	GLint result = 0;
	glGetQueryObjectiv(id, GL_QUERY_RESULT_AVAILABLE, &result);
	return result == GL_TRUE;
#elif defined(LIB_GLESv2)
	return true;
#endif
}

uint32_t QueryObject::getResult() const {
#if defined(LIB_GL)
	GLuint result = 0;
	glGetQueryObjectuiv(id, GL_QUERY_RESULT, &result);
	return result;
#elif defined(LIB_GLESv2)
	return 0;
#endif
}

#if defined(LIB_GL) and defined(LIB_GLEW)
static bool is64BitQueryResultSupported(){
	if(!isExtensionSupported("GL_ARB_timer_query")) {
		WARN("QueryObject::getResult64(): GL_ARB_timer_query is not supported; using QueryObject::getResult() instead.");
		return false;
	}
	return true;
}
#endif

uint64_t QueryObject::getResult64() const {
#if defined(LIB_GLEW) and defined(LIB_GL) and defined(GL_ARB_timer_query)
	static const bool supported = is64BitQueryResultSupported();
	if(supported){
		uint64_t result = 0;
		glGetQueryObjectui64v(id, GL_QUERY_RESULT, &result);
		return result;
	}
#endif
	return getResult();
}

void QueryObject::begin() const {
#if defined(LIB_GL)
	glBeginQuery(static_cast<GLenum>(queryType), id);
#endif
}

void QueryObject::end() const {
#if defined(LIB_GL)
	glEndQuery(static_cast<GLenum>(queryType));
#endif
}

void QueryObject::queryCounter() const {
#if defined(LIB_GL)
	if(queryType == GL_TIMESTAMP)
		glQueryCounter(id, static_cast<GLenum>(queryType));
#endif
}

static std::deque<uint32_t> freeIds;
static const uint32_t batchSize = 500;

//! (static)
uint32_t QueryObject::getFreeId() {
#if defined(LIB_GL)
	if (freeIds.empty()) {
		// Generate a batch of new identifiers.
		GLuint ids[batchSize];
		glGenQueries(batchSize, ids);

		if (ids[0] == 0) {
			WARN("Creation of occlusion query identifiers failed.");
			FAIL();
		}

		// freeIds was empty before.
		freeIds.assign(ids, ids + batchSize);
	}
	uint32_t freeId = freeIds.front();
	freeIds.pop_front();
	return freeId;
#elif defined(LIB_GLESv2)
	return 0;
#endif
}

//! (static)
void QueryObject::freeId(uint32_t id) {
#if defined(LIB_GL)
	if(id!=0){
		freeIds.push_back(id);

		if (freeIds.size() >= batchSize) {
			// Free a batch of identifiers.
			GLuint ids[batchSize];
			std::copy(freeIds.begin(), freeIds.begin() + batchSize, ids);
			freeIds.erase(freeIds.begin(), freeIds.begin() + batchSize);
			glDeleteQueries(batchSize, ids);
		}
	}
#endif
}

}
