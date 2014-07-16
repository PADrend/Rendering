/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef TEXTURE_PIXEL_FORMAT_GL_H
#define TEXTURE_PIXEL_FORMAT_GL_H

#include <cstdint>

namespace Rendering {

struct PixelFormatGL{
	uint32_t glLocalDataFormat, glLocalDataType, glInternalFormat;
	bool compressed;
	
	PixelFormatGL() : glLocalDataFormat(0),glLocalDataType(0),glInternalFormat(0),compressed(false){}
	
	bool isValid()const{	return glInternalFormat!=0;	}
};

}


#endif // TEXTURE_PIXEL_FORMAT_GL_H
