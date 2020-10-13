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

#include "../State/ShaderLayout.h"
#include "../State/BindingState.h"

#include <Util/References.h>
#include <Util/Utils.h>

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class Shader;
using ShaderRef = Util::Reference<Shader>;
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class Texture;
using TextureRef = Util::Reference<Texture>;

//! @addtogroup shader
//! @{

//==================================================================

namespace ShaderUtils {

/**
 * Reflects the shader resources from a compiled shader. 
 * @return List of shader resources.
 */
ShaderResourceList reflect(ShaderStage stage, const std::vector<uint32_t>& code);

//! Creates the default fallback shader.
ShaderRef createDefaultShader(const DeviceRef& device);

//! Creates a simple pass-through shader.
ShaderRef createPassThroughShader(const DeviceRef& device);

//! Create a shader that writes the pixel normal into the color buffer.
ShaderRef createNormalToColorShader(const DeviceRef& device);
[[deprecated]]
ShaderRef createNormalToColorShader();

//! initialized a binding state filled with dummy values for a given shader layout
BindingState initBindingState(const ShaderLayout& layout, const BufferObjectRef& bo, const TextureRef& tex);

}

//! @}
}

#endif /* RENDERING_SHADERUTILS_H_ */
