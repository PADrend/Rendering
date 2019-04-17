/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef OCCLUSIONQUERY_H_
#define OCCLUSIONQUERY_H_

#include "QueryObject.h"

namespace Rendering {
class RenderingContext;

/**
 * Wrapper class for OpenGL occlusion queries.
 *
 * @author Benjamin Eikel, Claudius Jähn
 * @date 2009-12-10
 * @ingroup helper
 */
class OcclusionQuery : public QueryObject{
	public:
		/*! Pushes the current gl state, disables lighting, color writes and depth writes.
			\note After calling enableTestMode() you always have to call disableTestMode(context.getRenderingContext()) eventually
					(as internally glPushAttrib() / popAttrib() is used). */
		static void enableTestMode(RenderingContext & renderingContext);

		/*!	Restores the old gl state. */
		static void disableTestMode(RenderingContext & renderingContext);

		OcclusionQuery();
		OcclusionQuery(OcclusionQuery &&) = default;
};
}

#endif /* OCCLUSIONQUERY_H_ */
