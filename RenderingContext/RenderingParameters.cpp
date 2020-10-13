/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "RenderingParameters.h"
#include "../Texture/Texture.h"
#include "../State/PipelineState.h"
#include <Util/StringIdentifier.h>
#include <Util/Macros.h>
#include <stdexcept>
#include <string>

namespace Rendering {

ComparisonFunc Comparison::functionToComparisonFunc(Comparison::function_t function) {
	switch(function) {
		case Comparison::NEVER: return ComparisonFunc::Never;
		case Comparison::LESS: return ComparisonFunc::Less;
		case Comparison::EQUAL: return ComparisonFunc::Equal;
		case Comparison::LEQUAL: return ComparisonFunc::LessOrEqual;
		case Comparison::GREATER: return ComparisonFunc::Greater;
		case Comparison::NOTEQUAL: return ComparisonFunc::NotEqual;
		case Comparison::GEQUAL: return ComparisonFunc::GreaterOrEqual;
		case Comparison::ALWAYS: return ComparisonFunc::Always;
		default:
			break;
	}
	throw std::invalid_argument("Invalid Comparison::function_t enumerator");
}

Comparison::function_t Comparison::comparisonFuncToFunction(ComparisonFunc function) {
	switch(function) {
		case ComparisonFunc::Never: return Comparison::NEVER;
		case ComparisonFunc::Less: return Comparison::LESS;
		case ComparisonFunc::Equal: return Comparison::EQUAL;
		case ComparisonFunc::LessOrEqual: return Comparison::LEQUAL;
		case ComparisonFunc::Greater: return Comparison::GREATER;
		case ComparisonFunc::NotEqual: return Comparison::NOTEQUAL;
		case ComparisonFunc::GreaterOrEqual: return Comparison::GEQUAL;
		case ComparisonFunc::Always: return Comparison::ALWAYS;
		default:
			break;
	}
	return Comparison::ALWAYS;
}


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

static BlendingParameters::function_t toFunction(BlendFactor f) {
	switch(f) {
		case BlendFactor::Zero: return BlendingParameters::ZERO;
		case BlendFactor::One: return BlendingParameters::ONE;
		case BlendFactor::SrcColor: return BlendingParameters::SRC_COLOR;
		case BlendFactor::OneMinusSrcColor: return BlendingParameters::ONE_MINUS_SRC_COLOR;
		case BlendFactor::DstColor: return BlendingParameters::DST_COLOR;
		case BlendFactor::OneMinusDstColor: return BlendingParameters::ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlpha: return BlendingParameters::SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha: return BlendingParameters::ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha: return BlendingParameters::DST_ALPHA;
		case BlendFactor::OneMinusDstAlpha: return BlendingParameters::ONE_MINUS_DST_ALPHA;
		case BlendFactor::ConstantColor: return BlendingParameters::CONSTANT_COLOR;
		case BlendFactor::OneMinusConstantColor: return BlendingParameters::ONE_MINUS_CONSTANT_COLOR;
		case BlendFactor::ConstantAlpha: return BlendingParameters::CONSTANT_ALPHA;
		case BlendFactor::OneMinusConstantAlpha: return BlendingParameters::ONE_MINUS_CONSTANT_ALPHA;
		case BlendFactor::SrcAlphaSaturate: return BlendingParameters::SRC_ALPHA_SATURATE;
		default:
			WARN("Unsupported blend function");
			return BlendingParameters::ZERO;
	}
}


static BlendFactor toBlendFactor(BlendingParameters::function_t f) {
	switch(f) {
		case BlendingParameters::ZERO: return BlendFactor::Zero;
		case BlendingParameters::ONE: return BlendFactor::One;
		case BlendingParameters::SRC_COLOR: return BlendFactor::SrcColor;
		case BlendingParameters::ONE_MINUS_SRC_COLOR: return BlendFactor::OneMinusSrcColor;
		case BlendingParameters::DST_COLOR: return BlendFactor::DstColor;
		case BlendingParameters::ONE_MINUS_DST_COLOR: return BlendFactor::OneMinusDstColor;
		case BlendingParameters::SRC_ALPHA: return BlendFactor::SrcAlpha;
		case BlendingParameters::ONE_MINUS_SRC_ALPHA: return BlendFactor::OneMinusSrcAlpha;
		case BlendingParameters::DST_ALPHA: return BlendFactor::DstAlpha;
		case BlendingParameters::ONE_MINUS_DST_ALPHA: return BlendFactor::OneMinusDstAlpha;
		case BlendingParameters::CONSTANT_COLOR: return BlendFactor::ConstantColor;
		case BlendingParameters::ONE_MINUS_CONSTANT_COLOR: return BlendFactor::OneMinusConstantColor;
		case BlendingParameters::CONSTANT_ALPHA: return BlendFactor::ConstantAlpha;
		case BlendingParameters::ONE_MINUS_CONSTANT_ALPHA: return BlendFactor::OneMinusConstantAlpha;
		case BlendingParameters::SRC_ALPHA_SATURATE: return BlendFactor::SrcAlphaSaturate;
	}
}

static BlendingParameters::equation_t toEquation(BlendOp op) {
	switch(op) {
		case BlendOp::Add: return BlendingParameters::FUNC_ADD;
		case BlendOp::Subtract: return BlendingParameters::FUNC_SUBTRACT;
		case BlendOp::ReverseSubtract: return BlendingParameters::FUNC_REVERSE_SUBTRACT;
		default:
			WARN("Unsupported blend equation");
			return BlendingParameters::FUNC_ADD;
	}
}

static BlendOp toBlendOp(BlendingParameters::equation_t op) {
	switch(op) {
		case BlendingParameters::FUNC_ADD: return BlendOp::Add;
		case BlendingParameters::FUNC_SUBTRACT: return BlendOp::Subtract;
		case BlendingParameters::FUNC_REVERSE_SUBTRACT: return BlendOp::ReverseSubtract;
	}
}

BlendingParameters::BlendingParameters(const ColorBlendState& state) :
	enabled(state.getAttachment().blendEnable),
	blendFuncSrcRGB(toFunction(state.getAttachment().srcColorBlendFactor)),
	blendFuncDstRGB(toFunction(state.getAttachment().dstColorBlendFactor)),
	blendFuncSrcAlpha(toFunction(state.getAttachment().srcAlphaBlendFactor)),
	blendFuncDstAlpha(toFunction(state.getAttachment().dstAlphaBlendFactor)),
	blendEquationRGB(toEquation(state.getAttachment().colorBlendOp)),
	blendEquationAlpha(toEquation(state.getAttachment().alphaBlendOp)),
	blendColor(state.getConstantColor()) {
}

ColorBlendState BlendingParameters::toBlendState() const {
	ColorBlendState s;
	s.setAttachment({
		enabled,
		toBlendFactor(blendFuncSrcRGB), toBlendFactor(blendFuncDstRGB), toBlendOp(blendEquationRGB),
		toBlendFactor(blendFuncSrcAlpha), toBlendFactor(blendFuncDstAlpha), toBlendOp(blendEquationAlpha)
	});
	s.setConstantColor(blendColor);
}


CullFaceParameters::CullFaceParameters(CullMode m) : enabled(true) {
	switch(m) {
		case CullMode::None: enabled = false; break;
		case CullMode::Front: mode = CULL_FRONT; break;
		case CullMode::Back: mode = CULL_BACK; break;
		case CullMode::FrontAndBack: mode = CULL_FRONT_AND_BACK; break;
	}
}

CullMode CullFaceParameters::getCullMode() const {
	if(!enabled) return CullMode::None;	
	switch(mode) {
		case CULL_FRONT: return CullMode::Front;
		case CULL_BACK: return CullMode::Back;
		case CULL_FRONT_AND_BACK: return CullMode::FrontAndBack;
		default: return CullMode::None;
	}
}


//==============================================================================================================================


// declare here to allow forward declaration of Texture
ImageBindParameters::ImageBindParameters(Texture*t) : texture(t),layer(0),level(0),multiLayer(false),readOperations(true),writeOperations(true) {}
ImageBindParameters::ImageBindParameters() : layer(0),level(0),multiLayer(false),readOperations(true),writeOperations(true) {}
ImageBindParameters::~ImageBindParameters(){} 
void ImageBindParameters::setTexture(Texture* t)			{	texture = t;	}


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
