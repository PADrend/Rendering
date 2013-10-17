/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ParameterStructs.h"
#include "../GLHeader.h"
#include <Util/StringIdentifier.h>
#include <stdexcept>
#include <string>

namespace Rendering {

std::string Comparison::functionToString(Comparison::function_t function) {
	switch(function) {
		case NEVER:
			return "NEVER";
		case LESS:
			return "LESS";
		case EQUAL:
			return "EQUAL";
		case LEQUAL:
			return "LEQUAL";
		case GREATER:
			return "GREATER";
		case NOTEQUAL:
			return "NOTEQUAL";
		case GEQUAL:
			return "GEQUAL";
		case ALWAYS:
			return "ALWAYS";
		default:
			break;
	}
	throw std::invalid_argument("Invalid Comparison::function_t enumerator");
}

Comparison::function_t Comparison::stringToFunction(const std::string & str) {
	static const Util::StringIdentifier idNEVER("NEVER");
	static const Util::StringIdentifier idLESS("LESS");
	static const Util::StringIdentifier idEQUAL("EQUAL");
	static const Util::StringIdentifier idLEQUAL("LEQUAL");
	static const Util::StringIdentifier idGREATER("GREATER");
	static const Util::StringIdentifier idNOTEQUAL("NOTEQUAL");
	static const Util::StringIdentifier idGEQUAL("GEQUAL");
	static const Util::StringIdentifier idALWAYS("ALWAYS");

	const Util::StringIdentifier idStr(str);
	if(idStr == idNEVER) {
		return NEVER;
	} else if(idStr == idLESS) {
		return LESS;
	} else if(idStr == idEQUAL) {
		return EQUAL;
	} else if(idStr == idLEQUAL) {
		return LEQUAL;
	} else if(idStr == idGREATER) {
		return GREATER;
	} else if(idStr == idNOTEQUAL) {
		return NOTEQUAL;
	} else if(idStr == idGEQUAL) {
		return GEQUAL;
	} else if(idStr == idALWAYS) {
		return ALWAYS;
	}
	throw std::invalid_argument("Invalid string representation of Comparison::function_t enumerator");
}

uint32_t Comparison::functionToGL(Comparison::function_t function) {
	switch(function) {
		case NEVER:
			return GL_NEVER;
		case LESS:
			return GL_LESS;
		case EQUAL:
			return GL_EQUAL;
		case LEQUAL:
			return GL_LEQUAL;
		case GREATER:
			return GL_GREATER;
		case NOTEQUAL:
			return GL_NOTEQUAL;
		case GEQUAL:
			return GL_GEQUAL;
		case ALWAYS:
			return GL_ALWAYS;
		default:
			break;
	}
	throw std::invalid_argument("Invalid Comparison::function_t enumerator");
}

Comparison::function_t Comparison::glToFunction(uint32_t value) {
	switch(value) {
		case GL_NEVER:
			return NEVER;
		case GL_LESS:
			return LESS;
		case GL_EQUAL:
			return EQUAL;
		case GL_LEQUAL:
			return LEQUAL;
		case GL_GREATER:
			return GREATER;
		case GL_NOTEQUAL:
			return NOTEQUAL;
		case GL_GEQUAL:
			return GEQUAL;
		case GL_ALWAYS:
			return ALWAYS;
		default:
			break;
	}
	throw std::invalid_argument("Invalid GLenum value for Comparison::function_t enumerator");
}

std::string BlendingParameters::functionToString(BlendingParameters::function_t function) {
	switch(function) {
		case ZERO:
			return "ZERO";
		case ONE:
			return "ONE";
		case SRC_COLOR:
			return "SRC_COLOR";
		case ONE_MINUS_SRC_COLOR:
			return "ONE_MINUS_SRC_COLOR";
		case SRC_ALPHA:
			return "SRC_ALPHA";
		case ONE_MINUS_SRC_ALPHA:
			return "ONE_MINUS_SRC_ALPHA";
		case DST_ALPHA:
			return "DST_ALPHA";
		case ONE_MINUS_DST_ALPHA:
			return "ONE_MINUS_DST_ALPHA";
		case DST_COLOR:
			return "DST_COLOR";
		case ONE_MINUS_DST_COLOR:
			return "ONE_MINUS_DST_COLOR";
		case SRC_ALPHA_SATURATE:
			return "SRC_ALPHA_SATURATE";
		case CONSTANT_COLOR:
			return "CONSTANT_COLOR";
		case ONE_MINUS_CONSTANT_COLOR:
			return "ONE_MINUS_CONSTANT_COLOR";
		case CONSTANT_ALPHA:
			return "CONSTANT_ALPHA";
		case ONE_MINUS_CONSTANT_ALPHA:
			return "ONE_MINUS_CONSTANT_ALPHA";
		default:
			break;
	}
	throw std::invalid_argument("Invalid BlendingParameters::function_t enumerator");
}

BlendingParameters::function_t BlendingParameters::stringToFunction(const std::string & str) {
	static const Util::StringIdentifier idZERO("ZERO");
	static const Util::StringIdentifier idONE("ONE");
	static const Util::StringIdentifier idSRC_COLOR("SRC_COLOR");
	static const Util::StringIdentifier idONE_MINUS_SRC_COLOR("ONE_MINUS_SRC_COLOR");
	static const Util::StringIdentifier idSRC_ALPHA("SRC_ALPHA");
	static const Util::StringIdentifier idONE_MINUS_SRC_ALPHA("ONE_MINUS_SRC_ALPHA");
	static const Util::StringIdentifier idDST_ALPHA("DST_ALPHA");
	static const Util::StringIdentifier idONE_MINUS_DST_ALPHA("ONE_MINUS_DST_ALPHA");
	static const Util::StringIdentifier idDST_COLOR("DST_COLOR");
	static const Util::StringIdentifier idONE_MINUS_DST_COLOR("ONE_MINUS_DST_COLOR");
	static const Util::StringIdentifier idSRC_ALPHA_SATURATE("SRC_ALPHA_SATURATE");
	static const Util::StringIdentifier idCONSTANT_COLOR("CONSTANT_COLOR");
	static const Util::StringIdentifier idONE_MINUS_CONSTANT_COLOR("ONE_MINUS_CONSTANT_COLOR");
	static const Util::StringIdentifier idCONSTANT_ALPHA("CONSTANT_ALPHA");
	static const Util::StringIdentifier idONE_MINUS_CONSTANT_ALPHA("ONE_MINUS_CONSTANT_ALPHA");

	const Util::StringIdentifier idStr(str);
	if(idStr == idZERO) {
		return ZERO;
	} else if(idStr == idONE) {
		return ONE;
	} else if(idStr == idSRC_COLOR) {
		return SRC_COLOR;
	} else if(idStr == idONE_MINUS_SRC_COLOR) {
		return ONE_MINUS_SRC_COLOR;
	} else if(idStr == idSRC_ALPHA) {
		return SRC_ALPHA;
	} else if(idStr == idONE_MINUS_SRC_ALPHA) {
		return ONE_MINUS_SRC_ALPHA;
	} else if(idStr == idDST_ALPHA) {
		return DST_ALPHA;
	} else if(idStr == idONE_MINUS_DST_ALPHA) {
		return ONE_MINUS_DST_ALPHA;
	} else if(idStr == idDST_COLOR) {
		return DST_COLOR;
	} else if(idStr == idONE_MINUS_DST_COLOR) {
		return ONE_MINUS_DST_COLOR;
	} else if(idStr == idSRC_ALPHA_SATURATE) {
		return SRC_ALPHA_SATURATE;
	} else if(idStr == idCONSTANT_COLOR) {
		return CONSTANT_COLOR;
	} else if(idStr == idONE_MINUS_CONSTANT_COLOR) {
		return ONE_MINUS_CONSTANT_COLOR;
	} else if(idStr == idCONSTANT_ALPHA) {
		return CONSTANT_ALPHA;
	} else if(idStr == idONE_MINUS_CONSTANT_ALPHA) {
		return ONE_MINUS_CONSTANT_ALPHA;
	}
	throw std::invalid_argument("Invalid string representation of BlendingParameters::function_t enumerator");
}

uint32_t BlendingParameters::functionToGL(BlendingParameters::function_t function) {
	switch(function) {
		case ZERO:
			return GL_ZERO;
		case ONE:
			return GL_ONE;
		case SRC_COLOR:
			return GL_SRC_COLOR;
		case ONE_MINUS_SRC_COLOR:
			return GL_ONE_MINUS_SRC_COLOR;
		case SRC_ALPHA:
			return GL_SRC_ALPHA;
		case ONE_MINUS_SRC_ALPHA:
			return GL_ONE_MINUS_SRC_ALPHA;
		case DST_ALPHA:
			return GL_DST_ALPHA;
		case ONE_MINUS_DST_ALPHA:
			return GL_ONE_MINUS_DST_ALPHA;
		case DST_COLOR:
			return GL_DST_COLOR;
		case ONE_MINUS_DST_COLOR:
			return GL_ONE_MINUS_DST_COLOR;
		case SRC_ALPHA_SATURATE:
			return GL_SRC_ALPHA_SATURATE;
		case CONSTANT_COLOR:
			return GL_CONSTANT_COLOR;
		case ONE_MINUS_CONSTANT_COLOR:
			return GL_ONE_MINUS_CONSTANT_COLOR;
		case CONSTANT_ALPHA:
			return GL_CONSTANT_ALPHA;
		case ONE_MINUS_CONSTANT_ALPHA:
			return GL_ONE_MINUS_CONSTANT_ALPHA;
		default:
			break;
	}
	throw std::invalid_argument("Invalid BlendingParameters::function_t enumerator");
}

BlendingParameters::function_t BlendingParameters::glToFunction(uint32_t value) {
	switch(value) {
		case GL_ZERO:
			return ZERO;
		case GL_ONE:
			return ONE;
		case GL_SRC_COLOR:
			return SRC_COLOR;
		case GL_ONE_MINUS_SRC_COLOR:
			return ONE_MINUS_SRC_COLOR;
		case GL_SRC_ALPHA:
			return SRC_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:
			return ONE_MINUS_SRC_ALPHA;
		case GL_DST_ALPHA:
			return DST_ALPHA;
		case GL_ONE_MINUS_DST_ALPHA:
			return ONE_MINUS_DST_ALPHA;
		case GL_DST_COLOR:
			return DST_COLOR;
		case GL_ONE_MINUS_DST_COLOR:
			return ONE_MINUS_DST_COLOR;
		case GL_SRC_ALPHA_SATURATE:
			return SRC_ALPHA_SATURATE;
		case GL_CONSTANT_COLOR:
			return CONSTANT_COLOR;
		case GL_ONE_MINUS_CONSTANT_COLOR:
			return ONE_MINUS_CONSTANT_COLOR;
		case GL_CONSTANT_ALPHA:
			return CONSTANT_ALPHA;
		case GL_ONE_MINUS_CONSTANT_ALPHA:
			return ONE_MINUS_CONSTANT_ALPHA;
		default:
			break;
	}
	throw std::invalid_argument("Invalid GLenum value for BlendingParameters::function_t enumerator");
}

std::string BlendingParameters::equationToString(BlendingParameters::equation_t equation) {
	switch(equation) {
		case FUNC_ADD:
			return "FUNC_ADD";
		case FUNC_SUBTRACT:
			return "FUNC_SUBTRACT";
		case FUNC_REVERSE_SUBTRACT:
			return "FUNC_REVERSE_SUBTRACT";
		default:
			break;
	}
	throw std::invalid_argument("Invalid BlendingParameters::equation_t enumerator");
}

BlendingParameters::equation_t BlendingParameters::stringToEquation(const std::string & str) {
	static const Util::StringIdentifier idFUNC_ADD("FUNC_ADD");
	static const Util::StringIdentifier idFUNC_SUBTRACT("FUNC_SUBTRACT");
	static const Util::StringIdentifier idFUNC_REVERSE_SUBTRACT("FUNC_REVERSE_SUBTRACT");

	const Util::StringIdentifier idStr(str);
	if(idStr == idFUNC_ADD) {
		return FUNC_ADD;
	} else if(idStr == idFUNC_SUBTRACT) {
		return FUNC_SUBTRACT;
	} else if(idStr == idFUNC_REVERSE_SUBTRACT) {
		return FUNC_REVERSE_SUBTRACT;
	}
	throw std::invalid_argument("Invalid string representation of BlendingParameters::equation_t enumerator");
}

uint32_t BlendingParameters::equationToGL(BlendingParameters::equation_t equation) {
	switch(equation) {
		case FUNC_ADD:
			return GL_FUNC_ADD;
		case FUNC_SUBTRACT:
			return GL_FUNC_SUBTRACT;
		case FUNC_REVERSE_SUBTRACT:
			return GL_FUNC_REVERSE_SUBTRACT;
		default:
			break;
	}
	throw std::invalid_argument("Invalid BlendingParameters::equation_t enumerator");
}

BlendingParameters::equation_t BlendingParameters::glToEquation(uint32_t value) {
	switch(value) {
		case GL_FUNC_ADD:
			return FUNC_ADD;
		case GL_FUNC_SUBTRACT:
			return FUNC_SUBTRACT;
		case GL_FUNC_REVERSE_SUBTRACT:
			return FUNC_REVERSE_SUBTRACT;
		default:
			break;
	}
	throw std::invalid_argument("Invalid GLenum value for BlendingParameters::equation_t enumerator");
}

std::string PolygonModeParameters::modeToString(PolygonModeParameters::polygonModeMode_t mode) {
	switch(mode) {
		case POINT:
			return "POINT";
		case LINE:
			return "LINE";
		case FILL:
			return "FILL";
		default:
			break;
	}
	throw std::invalid_argument("Invalid PolygonModeParameters::polygonModeMode_t enumerator");
}

PolygonModeParameters::polygonModeMode_t PolygonModeParameters::stringToMode(const std::string & str) {
	static const Util::StringIdentifier idPOINT("POINT");
	static const Util::StringIdentifier idLINE("LINE");
	static const Util::StringIdentifier idFILL("FILL");

	const Util::StringIdentifier idStr(str);
	if(idStr == idPOINT) {
		return POINT;
	} else if(idStr == idLINE) {
		return LINE;
	} else if(idStr == idFILL) {
		return FILL;
	}
	throw std::invalid_argument("Invalid string representation of PolygonModeParameters::polygonModeMode_t enumerator");
}

uint32_t PolygonModeParameters::modeToGL(PolygonModeParameters::polygonModeMode_t mode) {
#ifdef LIB_GL
	switch(mode) {
		case POINT:
			return GL_POINT;
		case LINE:
			return GL_LINE;
		case FILL:
			return GL_FILL;
		default:
			break;
	}
#endif /* LIB_GL */
	throw std::invalid_argument("Invalid PolygonModeParameters::polygonModeMode_t enumerator");
}

PolygonModeParameters::polygonModeMode_t PolygonModeParameters::glToMode(uint32_t value) {
#ifdef LIB_GL
	switch(value) {
		case GL_POINT:
			return POINT;
		case GL_LINE:
			return LINE;
		case GL_FILL:
			return FILL;
		default:
			break;
	}
#endif /* LIB_GL */
	throw std::invalid_argument("Invalid GLenum value for PolygonModeParameters::polygonModeMode_t enumerator");
}

}
