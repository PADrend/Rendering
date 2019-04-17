/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADERUTILS_H_
#define RENDERING_SHADERUTILS_H_

#include <Util/References.h>

namespace Rendering {
class Shader;
//! @ingroup shader
namespace ShaderUtils {

//! Create a shader that writes the pixel normal into the color buffer.
Util::Reference<Shader> createNormalToColorShader();

//! Create a simple vertex pass through shader
Util::Reference<Shader> createPassThroughShader();

}
}

#endif /* RENDERING_SHADERUTILS_H_ */
