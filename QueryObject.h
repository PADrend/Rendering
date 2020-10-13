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
#ifndef RENDERING_QUERY_OBJECT_H_
#define RENDERING_QUERY_OBJECT_H_

#include "Core/QueryPool.h"
#include <Util/References.h>

#include <cstdint>

namespace Rendering {
class RenderingContext;

/**
 * Wrapper class for OpenGL queries.
 * @see OcclusionQuery.h
 * @author Benjamin Eikel, Claudius Jähn
 * @date 2013-03-21
 * @ingroup rendering_helper
 */
class QueryObject {
	public:
		//! Standard constructor
		explicit QueryObject(QueryType _queryType);
		
		QueryObject(const QueryObject & other) = delete;
		
		QueryObject(QueryObject && other) : query(other.query) { other.query.id = -1; }

		//! Destructor frees the query identifier.
		~QueryObject();

		QueryObject & operator=(const QueryObject &) = delete;
		QueryObject & operator=(QueryObject && other) = delete;

		/*!	Check if the result of the last query is already available.
		 *	@return @c true if the test is finished, false otherwise.
		 */
		bool isResultAvailable(RenderingContext& rc) const;

		/*!	Return the result of the query.
		 *	@return result value (e.g. sample count)
		 */
		uint32_t getResult(RenderingContext& rc) const;
		
		/*! Returns the result as 64bit value. */
		uint64_t getResult64(RenderingContext& rc) const;

		//!	Start the query. @a end() has to be called after the rendering was done.
		void begin(RenderingContext& rc) const;

		//!	Stop the query.
		void end(RenderingContext& rc) const;
		
		//! Record the time; only used with Timestamp
		void queryCounter(RenderingContext& rc) const;

		bool isValid() const { return query.id>=0 && query.pool; }
		
		//! Returns the query's type.
		QueryType getQueryType() const { return query.type; }
		[[deprecated]]
		uint32_t _getQueryType() const { return static_cast<uint32_t>(query.type); }
	private:
		Query query; //! Internal query object identifier
		Query endQuery; //! For time elapsed queries
};
}

#endif /* RENDERING_QUERY_OBJECT_H_ */
