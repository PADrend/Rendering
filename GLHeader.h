/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef GLHEADER_H_INCLUDED
#define GLHEADER_H_INCLUDED

#if defined(LIB_GL) and defined(LIB_GLEW)
#ifdef _WIN64
// Workaround for mingw32 w64 (without __int64 is not defined in glew.h).
#include <cstdint>
#endif
#include <GL/glew.h>
#else
#error "No OpenGL support available."
#endif

#endif /* GLHEADER_H_INCLUDED */
