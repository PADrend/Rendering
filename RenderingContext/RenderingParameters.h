/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef PARAMETERSTRUCTS_H
#define PARAMETERSTRUCTS_H

#include <Geometry/Convert.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec4.h>
#include <Geometry/Plane.h>
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <string>

namespace Rendering {
class Texture;


/** @addtogroup context
 * @{
 * @defgroup rendering_parameter Rendering Parameters
 * @}
 */

/** @addtogroup rendering_parameter
 * @{
 */

namespace Comparison {
/**
 * @brief Type of comparison function
 * @see functions @c glAlphaFunc, @c glDepthFunc, @c glStencilFunc
 * @author Benjamin Eikel
 * @date 2012-02-04
 */
enum function_t {
	NEVER,
	LESS,
	EQUAL,
	LEQUAL,
	GREATER,
	NOTEQUAL,
	GEQUAL,
	ALWAYS
};
std::string functionToString(function_t function);
function_t stringToFunction(const std::string & str);

uint32_t functionToGL(function_t function);
function_t glToFunction(uint32_t value);
}

/**
 * @brief Parameters of alpha test
 *
 * Abstraction layer class for the alpha test configuration.
 * The OpenGL functions working with the alpha test settings are encapsulated inside.
 * @author Ralf Petring, Claudius Jähn, Benjamin Eikel
 * @date 2012-02-23
 * @see function @c glAlphaFunc, and constant @c GL_ALPHA_TEST of function @c glEnable
 */
class AlphaTestParameters {
	private:
		bool enabled;
		Comparison::function_t mode;
		float refValue;
	public:
		//! Create AlphaTestParameters representing the default OpenGL state.
		AlphaTestParameters() : enabled(false), mode(Comparison::ALWAYS), refValue(0.0f) {
		}
		//! Create AlphaTestParameters with the given values.
		AlphaTestParameters(Comparison::function_t _mode, float _refValue) : enabled(true), mode(_mode), refValue(_refValue) {
		}
		bool operator!=(const AlphaTestParameters & other) const {
			return enabled != other.enabled || mode != other.mode || refValue != other.refValue;
		}
		bool operator==(const AlphaTestParameters & other) const	{
			return enabled == other.enabled && mode == other.mode && refValue == other.refValue;
		}

		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}
		float getReferenceValue() const {
			return refValue;
		}
		Comparison::function_t getMode() const {
			return mode;
		}
		void setReferenceValue(float _refValue) {
			refValue = _refValue;
		}
		void setMode(Comparison::function_t _mode) {
			mode = _mode;
		}
};

// -------------------------------------------

class BlendingParameters {
	public:
		/**
		 * @brief Type of blending function
		 * @see function @c glBlendFuncSeparate
		 * @author Benjamin Eikel
		 * @date 2012-02-17
		 */
		enum function_t {
			ZERO,
			ONE,
			SRC_COLOR,
			ONE_MINUS_SRC_COLOR,
			SRC_ALPHA,
			ONE_MINUS_SRC_ALPHA,
			DST_ALPHA,
			ONE_MINUS_DST_ALPHA,
			DST_COLOR,
			ONE_MINUS_DST_COLOR,
			SRC_ALPHA_SATURATE,
			CONSTANT_COLOR,
			ONE_MINUS_CONSTANT_COLOR,
			CONSTANT_ALPHA,
			ONE_MINUS_CONSTANT_ALPHA
		};

		static std::string functionToString(function_t function);
		static function_t stringToFunction(const std::string & str);

		static uint32_t functionToGL(function_t function);
		static function_t glToFunction(uint32_t value);

		/**
		 * @brief Type of blending equation
		 * @see function @c glBlendEquationSeparate
		 * @author Benjamin Eikel
		 * @date 2012-02-17
		 */
		enum equation_t {
			FUNC_ADD,
			FUNC_SUBTRACT,
			FUNC_REVERSE_SUBTRACT
		};

		static std::string equationToString(equation_t equation);
		static equation_t stringToEquation(const std::string & str);

		static uint32_t equationToGL(equation_t equation);
		static equation_t glToEquation(uint32_t value);
	private:
		bool enabled;
		function_t blendFuncSrcRGB;
		function_t blendFuncDstRGB;
		function_t blendFuncSrcAlpha;
		function_t blendFuncDstAlpha;
		equation_t blendEquationRGB;
		equation_t blendEquationAlpha;
		Util::Color4f blendColor;
	public:
		//! Create BlendingParameters representing the default OpenGL state.
		explicit BlendingParameters() :
			enabled(false),
			blendFuncSrcRGB(ONE),
			blendFuncDstRGB(ZERO),
			blendFuncSrcAlpha(ONE),
			blendFuncDstAlpha(ZERO),
			blendEquationRGB(FUNC_ADD),
			blendEquationAlpha(FUNC_ADD),
			blendColor(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f)) {
		}

		//! Create BlendingParameters with the given values.
		explicit BlendingParameters(function_t srcFunc, function_t dstFunc) :
			enabled(true),
			blendFuncSrcRGB(srcFunc),
			blendFuncDstRGB(dstFunc),
			blendFuncSrcAlpha(srcFunc),
			blendFuncDstAlpha(dstFunc),
			blendEquationRGB(FUNC_ADD),
			blendEquationAlpha(FUNC_ADD),
			blendColor(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f)) {
		}

		bool operator!=(const BlendingParameters & other) const {
			return enabled != other.enabled ||
				   blendFuncSrcRGB != other.blendFuncSrcRGB ||
				   blendFuncDstRGB != other.blendFuncDstRGB ||
				   blendFuncSrcAlpha != other.blendFuncSrcAlpha ||
				   blendFuncDstAlpha != other.blendFuncDstAlpha ||
				   blendEquationRGB != other.blendEquationRGB ||
				   blendEquationAlpha != other.blendEquationAlpha ||
				   blendColor != other.blendColor;
		}
		bool operator==(const BlendingParameters & other) const {
			return enabled == other.enabled &&
				   blendFuncSrcRGB == other.blendFuncSrcRGB &&
				   blendFuncDstRGB == other.blendFuncDstRGB &&
				   blendFuncSrcAlpha == other.blendFuncSrcAlpha &&
				   blendFuncDstAlpha == other.blendFuncDstAlpha &&
				   blendEquationRGB == other.blendEquationRGB &&
				   blendEquationAlpha == other.blendEquationAlpha &&
				   blendColor == other.blendColor;
		}

		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}

		function_t getBlendFuncSrcRGB() const {
			return blendFuncSrcRGB;
		}
		function_t getBlendFuncDstRGB() const {
			return blendFuncDstRGB;
		}
		function_t getBlendFuncSrcAlpha() const {
			return blendFuncSrcAlpha;
		}
		function_t getBlendFuncDstAlpha() const {
			return blendFuncDstAlpha;
		}
		void setBlendFuncSrcRGB(function_t func) {
			blendFuncSrcRGB = func;
		}
		void setBlendFuncDstRGB(function_t func) {
			blendFuncDstRGB = func;
		}
		void setBlendFuncSrcAlpha(function_t func) {
			blendFuncSrcAlpha = func;
		}
		void setBlendFuncDstAlpha(function_t func) {
			blendFuncDstAlpha = func;
		}
		void setBlendFunc(function_t srcFunc, function_t dstFunc) {
			blendFuncSrcRGB = srcFunc;
			blendFuncDstRGB = dstFunc;
			blendFuncSrcAlpha = srcFunc;
			blendFuncDstAlpha = dstFunc;
		}

		equation_t getBlendEquationRGB() const {
			return blendEquationRGB;
		}
		equation_t getBlendEquationAlpha() const {
			return blendEquationAlpha;
		}
		void setBlendEquationRGB(equation_t equationRGB) {
			blendEquationRGB = equationRGB;
		}
		void setBlendEquationAlpha(equation_t equationAlpha) {
			blendEquationAlpha = equationAlpha;
		}
		void setBlendEquation(equation_t equation) {
			blendEquationRGB = equation;
			blendEquationAlpha = equation;
		}

		void setBlendColor(const Util::Color4f & c) {
			blendColor = c;
		}
		const Util::Color4f & getBlendColor() const {
			return blendColor;
		}
};

// -------------------------------------------

class ClipPlaneParameters {
	private:
		Geometry::Plane plane;
		bool enabled;

	public:
		//! Disable the clip plane.
		ClipPlaneParameters() : plane(), enabled(false) {}
		//! Enable the clip plane with the given plane.
		explicit ClipPlaneParameters(Geometry::Plane plane) : plane(std::move(plane)), enabled(true) {}
		bool operator!=(const ClipPlaneParameters & other) const { return enabled != other.enabled || !(plane == other.plane); }
		bool operator==(const ClipPlaneParameters & other) const { return enabled == other.enabled && plane == other.plane; }

		const Geometry::Plane & getPlane() const { return plane; }
		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}
};
static const uint8_t MAX_CLIP_PLANES = 6;

// -------------------------------------------

/**
 * @brief Parameters of color buffer
 *
 * Abstraction layer class for the color buffer configuration.
 * The OpenGL functions working with the color buffer settings are encapsulated inside.
 * @author Benjamin Eikel
 * @date 2012-02-16
 * @see @c glColorMask
 */
class ColorBufferParameters {
	private:
		//! @see parameter @c red of function @c glColorMask
		bool enableRedWriting;

		//! @see parameter @c green of function @c glColorMask
		bool enableGreenWriting;

		//! @see parameter @c blue of function @c glColorMask
		bool enableBlueWriting;

		//! @see parameter @c alpha of function @c glColorMask
		bool enableAlphaWriting;

	public:
		//! Create ColorBufferParameters representing the default OpenGL state.
		explicit ColorBufferParameters() :
			enableRedWriting(true),
			enableGreenWriting(true),
			enableBlueWriting(true),
			enableAlphaWriting(true) {
		}
		//! Create ColorBufferParameters with the given values.
		explicit ColorBufferParameters(bool redWritingEnabled,
									   bool greenWritingEnabled,
									   bool blueWritingEnabled,
									   bool alphaWritingEnabled) :
			enableRedWriting(redWritingEnabled),
			enableGreenWriting(greenWritingEnabled),
			enableBlueWriting(blueWritingEnabled),
			enableAlphaWriting(alphaWritingEnabled) {
		}

		//! Return @c true if the whole set of parameters is @b equal to the @a other set.
		bool operator==(const ColorBufferParameters & other) const {
			return enableRedWriting == other.enableRedWriting &&
				   enableGreenWriting == other.enableGreenWriting &&
				   enableBlueWriting == other.enableBlueWriting &&
				   enableAlphaWriting == other.enableAlphaWriting;
		}
		//! Return @c true if the whole set of parameters is @b unequal to the @a other set.
		bool operator!=(const ColorBufferParameters & other) const {
			return enableRedWriting != other.enableRedWriting ||
				   enableGreenWriting != other.enableGreenWriting ||
				   enableBlueWriting != other.enableBlueWriting ||
				   enableAlphaWriting != other.enableAlphaWriting;
		}

		bool isRedWritingEnabled() const {
			return enableRedWriting;
		}
		bool isGreenWritingEnabled() const {
			return enableGreenWriting;
		}
		bool isBlueWritingEnabled() const {
			return enableBlueWriting;
		}
		bool isAlphaWritingEnabled() const {
			return enableAlphaWriting;
		}
		bool isAnyWritingEnabled() const {
			return isRedWritingEnabled()||isGreenWritingEnabled()||isBlueWritingEnabled()||isAlphaWritingEnabled();
		}
};

// -------------------------------------------

/**
 * @brief Parameters of front- or back-face culling
 *
 * Abstraction layer class for the face culling configuration.
 * The OpenGL functions working with the face culling settings are encapsulated inside.
 * @author Ralf Petring, Benjamin Eikel
 * @date 2012-02-21
 * @see @c glCullFace, and constant @c GL_CULL_FACE of function @c glEnable
 */
class CullFaceParameters {
	public:
		enum cullFaceMode_t {
			CULL_BACK,
			CULL_FRONT,
			CULL_FRONT_AND_BACK
		};
	private:
		bool enabled;
		cullFaceMode_t mode;
	public:
		//! Create CullFaceParameters representing the default OpenGL state.
		CullFaceParameters() : enabled(false), mode(CULL_BACK) {}
		//! Create CullFaceParameters with the given values.
		CullFaceParameters(cullFaceMode_t m) : enabled(true), mode(m) {}
		bool operator!=(const CullFaceParameters & other) const {
			return enabled != other.enabled || mode != other.mode;
		}
		bool operator==(const CullFaceParameters & other) const {
			return enabled == other.enabled && mode == other.mode;
		}
		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}
		cullFaceMode_t getMode() const {
			return mode;
		}
		void setMode(cullFaceMode_t _mode) {
			mode = _mode;
		}
};

// -------------------------------------------

/**
 * @brief Parameters of depth buffer
 *
 * Abstraction layer class for the depth buffer configuration.
 * The OpenGL functions working with the depth buffer settings are encapsulated inside.
 * @author Benjamin Eikel
 * @date 2012-02-14
 * @see @c glDepthFunc, @c glDepthMask, and constant @c GL_DEPTH_BUFFER_BIT of function @c glPushAttrib
 */
class DepthBufferParameters {
	private:
		//! @see constant @c GL_DEPTH_TEST of function @c glEnable
		bool enableTest;

		//! @see function @c glDepthMask
		bool enableWriting;

		//! @see function @c glDepthFunc
		Comparison::function_t function;

	public:
		//! Create DepthBufferParameters representing the default OpenGL state.
		explicit DepthBufferParameters() : enableTest(false), enableWriting(true), function(Comparison::LESS) {
		}

		//! Create DepthBufferParameters with the given values.
		explicit DepthBufferParameters(bool testEnabled, bool writingEnabled, Comparison::function_t comparison) :
			enableTest(testEnabled), enableWriting(writingEnabled), function(comparison) {
		}

		//! Return @c true if the whole set of parameters is @b equal to the @a other set.
		bool operator==(const DepthBufferParameters & other) const {
			return enableTest == other.enableTest && enableWriting == other.enableWriting && function == other.function;
		}
		//! Return @c true if the whole set of parameters is @b unequal to the @a other set.
		bool operator!=(const DepthBufferParameters & other) const {
			return enableTest != other.enableTest || enableWriting != other.enableWriting || function != other.function;
		}

		bool isTestEnabled() const {
			return enableTest;
		}
		bool isWritingEnabled() const {
			return enableWriting;
		}
		Comparison::function_t getFunction() const {
			return function;
		}
};

// -------------------------------------------
//! Controls the binding of an image (part of a texture) for load and store operations in the shader.
class ImageBindParameters {
	private:
		Util::Reference<Texture> texture;
		uint32_t layer,level;
		bool  multiLayer,readOperations,writeOperations;
	
	public:
		ImageBindParameters();
		ImageBindParameters(Texture*t);
		~ImageBindParameters();
		
		uint32_t getLayer()const			{	return layer;	}
		void setLayer(uint32_t i)			{	layer = i;	}
		
		uint32_t getLevel()const			{	return level;	}
		void setLevel(uint32_t i)			{	level = i;	}
		
		bool getMultiLayer()const			{	return multiLayer;	}
		void setMultiLayer(bool b)			{	multiLayer = b;	}
		
		bool getReadOperations()const		{	return readOperations;	}
		void setReadOperations(bool b)		{	readOperations = b;	}
		
		bool getWriteOperations()const		{	return writeOperations;	}
		void setWriteOperations(bool b)		{	writeOperations = b;	}
		
		Texture* getTexture()const			{	return texture.get();	}
		void setTexture(Texture* t);
		
		bool operator==(const ImageBindParameters & other) const {
			return texture == other.texture&&layer==other.layer&&level==other.level&&multiLayer==other.multiLayer&&
					readOperations==other.readOperations&&writeOperations==other.writeOperations;
		}
};
static const uint8_t MAX_BOUND_IMAGES = 8;

// -------------------------------------------

/**
 * @brief Parameters of lighting
 *
 * Abstraction layer class for the lighting configuration.
 * The OpenGL functions working with the lighting settings are encapsulated inside.
 * @author Benjamin Eikel
 * @date 2012-02-16
 */
class LightingParameters {
	private:
		//! @see constant @c GL_LIGHTING of function @c glEnable
		bool enabled;

	public:
		//! Create LightingParameters representing the default OpenGL state.
		explicit LightingParameters() : enabled(false) {
		}
		//! Create LightingParameters with the given values.
		explicit LightingParameters(bool enableLighting) : enabled(enableLighting) {
		}

		//! Return @c true if the whole set of parameters is @b equal to the @a other set.
		bool operator==(const LightingParameters & other) const {
			return enabled == other.enabled;
		}
		//! Return @c true if the whole set of parameters is @b unequal to the @a other set.
		bool operator!=(const LightingParameters & other) const {
			return enabled != other.enabled;
		}

		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}
};

// -------------------------------------------

class LightParameters {
	public:
		enum lightType_t {
			DIRECTIONAL = 1, POINT = 2, SPOT = 3
		}type;

		Geometry::Vec3 direction,position;
		Util::Color4f ambient, diffuse, specular;
		float constant, linear, quadratic;
		float cutoff, cosCutoff, exponent;

		LightParameters() :	type(LightParameters::POINT), direction(), position(), ambient(0.2f, 0.2f, 0.2f, 1.0f), diffuse(0.8f, 0.8f, 0.8f, 1.0f), specular(1.0f, 1.0f, 1.0f,
					1.0f), constant(1.0f), linear(0.0f), quadratic(0.0f), cutoff(20.0f), cosCutoff(std::cos(Geometry::Convert::degToRad(cutoff))), exponent(2.0f) {
		}

		bool operator!=(const LightParameters & other) const {
			// cosCutoff depends on cutoff and therefore does not have to be checked
			return	type != other.type || direction != other.direction || position != other.position || ambient != other.ambient || diffuse != other.diffuse
					|| specular != other.specular || constant != other.constant || linear != other.linear || quadratic != other.quadratic || cutoff
					!= other.cutoff || exponent != other.exponent;
		}

		bool operator==(const LightParameters & other) const {
			return !(*this != other);
		}
};

// -------------------------------------------

/**
 * @brief Parameters of rasterized lines
 *
 * Abstraction layer class for the line rasterization configuration.
 * The OpenGL functions working with the line rasterization settings are encapsulated inside.
 * @author Benjamin Eikel
 * @date 2012-05-07
 * @see @c glLineWidth
 */
class LineParameters {
	private:
		//! Line width in pixels
		float width;
	public:
		//! Create LineParameters representing the default OpenGL state.
		LineParameters() : width(1.0f) {}
		//! Create LineParameters with the given values.
		LineParameters(float lineWidth) : width(lineWidth){}
		bool operator!=(const LineParameters & other) const {
			return width != other.width;
		}
		bool operator==(const LineParameters & other) const {
			return width == other.width;
		}
		float getWidth() const {
			return width;
		}
		void setWidth(float lineWidth) {
			width = lineWidth;
		}
};

// -------------------------------------------

class MaterialParameters {
	private:
		bool colorMaterial;

		Util::Color4f ambient;
		Util::Color4f diffuse;
		Util::Color4f specular;
		Util::Color4f emission;
		float shininess;
	public:
		MaterialParameters() :
			colorMaterial(false),
			ambient(Util::Color4f(0.2f, 0.2f, 0.2f, 1.0f)),
			diffuse(Util::Color4f(0.8f, 0.8f, 0.8f, 1.0f)),
			specular(Util::Color4f(0.0f, 0.0f, 0.0f, 1.0f)),
			emission(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f)),
			shininess(0.0f) {
		}

		bool operator==(const MaterialParameters & other) const {
			return colorMaterial == other.colorMaterial &&
				   ambient == other.ambient &&
				   diffuse == other.diffuse &&
				   specular == other.specular &&
				   emission == other.emission &&
				   shininess == other.shininess;
		}
		bool operator!=(const MaterialParameters & other) const {
			return colorMaterial != other.colorMaterial ||
				   ambient != other.ambient ||
				   diffuse != other.diffuse ||
				   specular != other.specular ||
				   emission != other.emission ||
				   shininess != other.shininess;
		}

		bool getColorMaterial() const {
			return colorMaterial;
		}
		void enableColorMaterial() {
			colorMaterial = true;
		}
		void disableColorMaterial() {
			colorMaterial = false;
		}
		const Util::Color4f & getAmbient() const {
			return ambient;
		}
		void setAmbient(const Util::Color4f & newAmbient) {
			ambient = newAmbient;
		}
		const Util::Color4f & getDiffuse() const {
			return diffuse;
		}
		void setDiffuse(const Util::Color4f & newDiffuse) {
			diffuse = newDiffuse;
		}
		const Util::Color4f & getSpecular() const {
			return specular;
		}
		void setSpecular(const Util::Color4f & newSpecular) {
			specular = newSpecular;
		}
		const Util::Color4f & getEmission() const {
			return emission;
		}
		void setEmission(const Util::Color4f & newEmission) {
			emission = newEmission;
		}
		float getShininess() const {
			return shininess;
		}
		void setShininess(float newShininess) {
			//if(newShininess < 0.0f || newShininess > 128.0f) {
			//	WARN("shininess out of range [0, 128].");
			//}
			shininess = std::min(std::max(newShininess, 0.0f), 128.0f);
		}
};

// -------------------------------------------

/**
 * @brief Parameters of rastered points
 *
 * @see @c glPointSize
 */
class PointParameters {
	private:
		//! Point width in pixels
		float size;
		bool smooth;
	public:
		PointParameters() : size(1.0f), smooth(false) {}
		PointParameters(float _size,bool _smooth=false) : size(_size),smooth(_smooth){}
		bool operator!=(const PointParameters & other) const {
			return size != other.size || smooth!=other.smooth;
		}
		bool operator==(const PointParameters & other) const {
			return size == other.size && smooth == other.smooth;
		}
		void enablePointSmoothing(){
			smooth = true;
		}
		void disablePointSmoothing(){
			smooth = false;
		}
		bool isPointSmoothingEnabled() const {
			return smooth;
		}
		float getSize() const {
			return size;
		}
		void setSize(float f) {
			size = f;
		}
};
// -------------------------------------------

class PolygonModeParameters {
	public:
		enum polygonModeMode_t { POINT = 1, LINE = 2, FILL = 3 };

		static std::string modeToString(polygonModeMode_t mode);
		static polygonModeMode_t stringToMode(const std::string & str);

		static uint32_t modeToGL(polygonModeMode_t mode);
		static polygonModeMode_t glToMode(uint32_t value);
	private:
		polygonModeMode_t mode;
	public:
		//! Create PolygonModeParameters representing the default OpenGL state.
		explicit PolygonModeParameters() : mode(FILL) {
		}
		//! Create PolygonModeParameters with the given values.
		explicit PolygonModeParameters(const polygonModeMode_t _mode) : mode(_mode) {
		}
		bool operator!=(const PolygonModeParameters & other) const {
			return mode != other.mode;
		}
		bool operator==(const PolygonModeParameters & other) const {
			return mode == other.mode;
		}

		polygonModeMode_t getMode() const {
			return mode;
		}
		void setMode(polygonModeMode_t _mode) {
			mode = _mode;
		}
};

// -------------------------------------------

/**
 * @brief Parameters of the polygon offset settings
 *
 * Abstraction layer class for polygon offsets.
 * The OpenGL functions working with the polygon offset are encapsulated inside.
 * @author Benjamin Eikel
 * @date 2012-02-14
 */
class PolygonOffsetParameters {
	private:
		bool enabled;

		//! Scale factor for variable depth offset
		float factor;
		//! Constant depth offset
		float units;

	public:
		/**
		 * Create PolygonOffsetParameters representing an disabled state.
		 *
		 * @see Parameter GL_POLYGON_OFFSET_FILL of glDisable
		 */
		explicit PolygonOffsetParameters() : enabled(false), factor(0.0f), units(0.0f) {
		}

		/**
		 * Create PolygonOffsetParameters with given values representing an enabled state.
		 *
		 * @see Parameter GL_POLYGON_OFFSET_FILL of glEnable
		 */
		explicit PolygonOffsetParameters(float newFactor, float newUnits) : enabled(true), factor(newFactor), units(newUnits) {
		}

		//! Return @c true if the whole set of parameters is @b equal to the @a other set.
		bool operator==(const PolygonOffsetParameters & other) const {
			return enabled == other.enabled && factor == other.factor && units == other.units;
		}
		//! Return @c true if the whole set of parameters is @b unequal to the @a other set.
		bool operator!=(const PolygonOffsetParameters & other) const {
			return enabled != other.enabled || factor != other.factor || units != other.units;
		}

		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}

		float getFactor() const {
			return factor;
		}
		//! @see Parameter factor of glPolygonOffset
		void setFactor(float newFactor) {
			factor = newFactor;
		}

		float getUnits() const {
			return units;
		}
		//! @see Parameter units of glPolygonOffset
		void setUnits(float newUnits) {
			units = newUnits;
		}
};


// -------------------------------------------

class PrimitiveRestartParameters {
	private:
		uint32_t index;
		bool enabled;

	public:
		//! Disable primitive restart.
		PrimitiveRestartParameters() : index(), enabled(false) {}
		//! Enable primitive restart with the given index.
		explicit PrimitiveRestartParameters(uint32_t index) : index(index), enabled(true) {}
		bool operator!=(const PrimitiveRestartParameters & other) const { return enabled != other.enabled || index != other.index; }
		bool operator==(const PrimitiveRestartParameters & other) const { return enabled == other.enabled && index == other.index; }

		uint32_t getIndex() const { return index; }
		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}
};

// -------------------------------------------

class ScissorParameters {
	private:
		Geometry::Rect_i rect;
		bool enabled;

	public:
		//! Disable the scissor test.
		ScissorParameters() : rect(), enabled(false) {}
		//! Enable the scissor test with the given rect.
		explicit ScissorParameters(Geometry::Rect_i scissorRect) : rect(std::move(scissorRect)), enabled(true) {}
		bool operator!=(const ScissorParameters & other) const { return enabled != other.enabled || rect != other.rect; }
		bool operator==(const ScissorParameters & other) const { return enabled == other.enabled && rect == other.rect; }

		const Geometry::Rect_i & getRect() const { return rect; }
		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}
};

// -------------------------------------------

/**
 * @brief Access to and modifcation of the stencil buffer
 *
 * Abstraction layer class for the stencil buffer.
 * The OpenGL functions working with the stencil buffer are encapsulated inside.
 * @author Benjamin Eikel
 * @date 2012-01-11
 */
class StencilParameters {
	public:
		enum action_t {
			KEEP,
			ZERO,
			REPLACE,
			INCR,
			INCR_WRAP,
			DECR,
			DECR_WRAP,
			INVERT
		};

	private:
		bool enabled;

		//! Stencil test function
		Comparison::function_t function;
		//! Reference value for the stencil test
		int32_t referenceValue;
		//! Bit mask for the stencil test;
		std::bitset<32> bitMask;

		//! Stencil action when the stencil test fails.
		action_t failAction;
		//! Stencil action when the stencil test passes, but the depth test fails.
		action_t depthTestFailAction;
		//! Stencil action when both the stencil test and the depth test pass.
		action_t depthTestPassAction;

	public:
		//! Create StencilParameters representing the default OpenGL state.
		explicit StencilParameters():
			enabled(false),
			function(Comparison::ALWAYS),
			referenceValue(0),
			bitMask(),
			failAction(KEEP),
			depthTestFailAction(KEEP),
			depthTestPassAction(KEEP) {
			// Set all bits to one.
			bitMask.set();
		}

		//! Return @c true if the function subset of parameters is @b equal to the @a other set.
		bool equalFunctionParameters(const StencilParameters & other) const {
			return function == other.function && referenceValue == other.referenceValue && bitMask == other.bitMask;
		}
		//! Return @c true if the function subset of parameters is @b equal to the @a other set.
		bool differentFunctionParameters(const StencilParameters & other) const {
			return function != other.function || referenceValue != other.referenceValue || bitMask != other.bitMask;
		}

		//! Return @c true if the action subset of parameters is @b equal to the @a other set.
		bool equalActionParameters(const StencilParameters & other) const {
			return failAction == other.failAction && depthTestFailAction == other.depthTestFailAction && depthTestPassAction == other.depthTestPassAction;
		}
		//! Return @c true if the action subset of parameters is @b equal to the @a other set.
		bool differentActionParameters(const StencilParameters & other) const {
			return failAction != other.failAction || depthTestFailAction != other.depthTestFailAction || depthTestPassAction != other.depthTestPassAction;
		}

		//! Return @c true if the whole set of parameters is @b equal to the @a other set.
		bool operator==(const StencilParameters & other) const {
			return enabled == other.enabled && equalFunctionParameters(other) && equalActionParameters(other);
		}
		//! Return @c true if the whole set of parameters is @b unequal to the @a other set.
		bool operator!=(const StencilParameters & other) const {
			return enabled != other.enabled || differentFunctionParameters(other) || differentActionParameters(other);
		}

		bool isEnabled() const {
			return enabled;
		}
		void enable() {
			enabled = true;
		}
		void disable() {
			enabled = false;
		}

		Comparison::function_t getFunction() const {
			return function;
		}
		//! @see Parameter func of glStencilFunc
		void setFunction(Comparison::function_t newFunction) {
			function = newFunction;
		}

		int32_t getReferenceValue() const {
			return referenceValue;
		}
		//! @see Parameter ref of glStencilFunc
		void setReferenceValue(int32_t newValue) {
			referenceValue = newValue;
		}

		const std::bitset<32> & getBitMask() const {
			return bitMask;
		}
		//! @see Parameter mask of glStencilFunc
		void setBitMask(const std::bitset<32> & newMask) {
			bitMask = newMask;
		}

		action_t getFailAction() const {
			return failAction;
		}
		//! @see Parameter sfail of glStencilOp
		void setFailAction(action_t newAction) {
			failAction = newAction;
		}

		action_t getDepthTestFailAction() const {
			return depthTestFailAction;
		}
		//! @see Parameter dpfail of glStencilOp
		void setDepthTestFailAction(action_t newAction) {
			depthTestFailAction = newAction;
		}

		action_t getDepthTestPassAction() const {
			return depthTestPassAction;
		}
		//! @see Parameter dppass of glStencilOp
		void setDepthTestPassAction(action_t newAction) {
			depthTestPassAction = newAction;
		}
};

//! Determines the intended usage of a texture bound to a texture unit.
enum class TexUnitUsageParameter : uint8_t {
	/**
	 * The texture is not used for texturing. When using legacy OpenGL, the
	 * corresponding state is disabled (see, e.g., the parameter
	 * @c GL_TEXTURE_2D of @c glDisable) for the corresponding unit (see
	 * @c glActiveTexture). If a shader is used, the corresponding uniform
	 * @c sg_textureEnabled[unit] is set to @c false.
	 */
	GENERAL_PURPOSE,
	/**
	 * The texture is used for texturing mapping using per vertex
	 * texture coordinates. When using legacy OpenGL, the corresponding state
	 * is enabled (see the parameter @c GL_TEXTURE_1D/2D/3D of @c glEnable) for the
	 * corresponding unit (see @c glActiveTexture). If a shader is used, the
	 * corresponding uniform @c sg_textureEnabled[unit] is set to @c true.
	 */
	TEXTURE_MAPPING,
	
	//! No Texture is bound to the texture unit.
	DISABLED
};

static const uint8_t MAX_TEXTURES = 8;

//! @}
}

#endif // PARAMETERSTRUCTS_H
