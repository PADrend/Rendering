/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STREAMERMVBO_H_
#define RENDERING_STREAMERMVBO_H_

#include "AbstractRenderingStreamer.h"
#include <cstdint>

namespace Rendering {
namespace Serialization {

/**

	MVBO-Format: Tims Reliefboards
	========================

	Die Daten sind wie folgt organisiert:
	1. uint32_t = #Vertexes
	2. uint32_t = #Faces
	3. #Vertexes * sizeof(aVertex)
	4. #Faces * sizeof(aFace)

	aVertex ist ein struct, dass so aussieht:
	struct aVertex{
		Vector3f vertex; //3 float
		Vector4ub color; //4 unsigned char
		Vector4b normal; //4 char
	};

	auch aFace ist ein struct:

	struct aFace {
		uint32_t a;
		uint32_t b;
		uint32_t c;
	}

*/
class StreamerMVBO : public AbstractRenderingStreamer {
	public:
		StreamerMVBO() :
			AbstractRenderingStreamer() {
		}
		virtual ~StreamerMVBO() {
		}

		Util::GenericAttributeList * loadGeneric(std::istream & input) override;
		Mesh * loadMesh(std::istream & input) override;

		static uint8_t queryCapabilities(const std::string & extension);
		static const char * const fileExtension;

	private:
		uint32_t read(std::istream & in)const;
};

}
}

#endif /* RENDERING_STREAMERMVBO_H_ */
