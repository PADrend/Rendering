/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STREAMERMTL_H_
#define RENDERING_STREAMERMTL_H_

#include "AbstractRenderingStreamer.h"

namespace Rendering {
namespace Serialization {

class StreamerMTL : public AbstractRenderingStreamer {
	public:
		StreamerMTL() :
			AbstractRenderingStreamer() {
		}
		virtual ~StreamerMTL() {
		}

		Util::GenericAttributeList * loadGeneric(std::istream & input) override;

		static uint8_t queryCapabilities(const std::string & extension);
		static const char * const fileExtension;
};
}
}

#endif /* RENDERING_STREAMERMTL_H_ */
