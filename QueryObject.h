/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_QUERY_OBJECT_H_
#define RENDERING_QUERY_OBJECT_H_

#include <cstdint>

namespace Rendering {

/**
 * Wrapper class for OpenGL queries.
 * @see OcclusionQuery.h
 * @author Benjamin Eikel, Claudius Jähn
 * @date 2013-03-21
 */
class QueryObject {
	public:
		//! Standard constructor
		explicit QueryObject(uint32_t _queryType) : queryType(_queryType),id(getFreeId()) {}
		
		QueryObject(const QueryObject & other) = delete;
		
		QueryObject(QueryObject && other) : queryType(other.queryType),id(other.id){	other.id = 0;	}

		//! Destructor frees the query identifier.
		~QueryObject()	{	freeId(id);	}

		QueryObject & operator=(const QueryObject &) = delete;
		QueryObject & operator=(QueryObject && other) = delete;

		/*!	Check if the result of the last query is already available.
		 *	@return @c true if the test is finished, false otherwise.
		 */
		bool isResultAvailable() const;

		/*!	Return the result of the query.
		 *	@return result value (e.g. sample count	)
		 */
		uint32_t getResult() const;
		
		/*! Returns the result as 64bit value.
		 *	If the used open gl driver does not support 'GL_ARB_timer_query', a warning is shown once 
		 *	and getResult() is returned instead.	*/
		uint64_t getResult64()const;

		//!	Start the query. @a end() has to be called after the rendering was done.
		void begin() const;

		//!	Stop the query.
		void end() const;

		bool isValid()const	{	return id!=0;	}
		
		//! Returns the GL constant of the query's type. \note Don't rely on GL constants from outside of Rendering.
		uint32_t _getQueryType()const	{	return queryType;	}
	private:
		
		uint32_t queryType;	//! OpenGL query object type; e.g. GL_SAMPLES_PASSED
		uint32_t id;	//! OpenGL query object identifier

		/*!	Request the next free query identifier.
		 *	@return Occlusion query identifier
		 */
		static uint32_t getFreeId();

		/*!	Mark the given query identifier as free.
		 * @param id Occlusion query identifier
		 */
		static void freeId(uint32_t id);

};
}

#endif /* RENDERING_QUERY_OBJECT_H_ */
