/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SHADER_H
#define SHADER_H

#include "ShaderObjectInfo.h"
#include "../RenderingContext/RenderingContext.h"
#include <Util/ReferenceCounter.h>
#include <Util/StringIdentifier.h>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
namespace Geometry {
template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4f;
}
namespace Util {
class FileName;
}

namespace Rendering {
class Uniform;
class UniformRegistry;
class RenderingStatus;

//! @defgroup shader Shader

/** Shader
 * @ingroup shader
 */
class Shader : public Util::ReferenceCounter<Shader> {
	/*! @name Main */
	// @{
	public:
		typedef uint32_t flag_t;
		static const flag_t USE_GL = 1 << 0;
		static const flag_t USE_UNIFORMS = 1 << 1;

		RENDERINGAPI ~Shader();

		//! (internal) called by RenderingContext
		RENDERINGAPI bool _enable();

		/*! Sets the active shader at the renderingContext. If the shader
			has not been linked, it is linked.
			\return returns true if the status of the shader is LINKED	*/
		RENDERINGAPI bool enable(RenderingContext & rc);
		RENDERINGAPI bool isActive(RenderingContext & rc); // (???) !=enabled

		bool usesClassicOpenGL()const		{	return RenderingContext::getCompabilityMode() && (usageFlags & USE_GL);	}
		bool usesSGUniforms()const			{	return !RenderingContext::getCompabilityMode() || (usageFlags & USE_UNIFORMS);	}
		void setUsage(flag_t newUsage)		{	usageFlags=newUsage;	}

		RenderingStatus * getRenderingStatus()	{	return renderingData.get();	}

	private:
		flag_t usageFlags;
		std::unique_ptr<RenderingStatus> renderingData; // created when the shader is successfully initialized

		RENDERINGAPI Shader(flag_t usage = USE_GL|USE_UNIFORMS);
		RENDERINGAPI static void printProgramInfoLog(uint32_t obj);
	// @}

	// ------------------------

	/*! @name (static) Factories */
	// @{
	public:
		RENDERINGAPI static Shader * loadShader(const Util::FileName & vsFile, const Util::FileName & fsFile, flag_t usage = USE_GL|USE_UNIFORMS);
		RENDERINGAPI static Shader * loadShader(const Util::FileName & vsFile, const Util::FileName & gsFile, const Util::FileName & fsFile, flag_t usage = USE_GL|USE_UNIFORMS);
		RENDERINGAPI static Shader * createShader(flag_t usage = USE_GL|USE_UNIFORMS);
		RENDERINGAPI static Shader * createShader(const std::string & vsa, const std::string & fsa, flag_t usage = USE_GL|USE_UNIFORMS);
		RENDERINGAPI static Shader * createShader(const std::string & vsa, const std::string & gsa, const std::string & fsa, flag_t usage = USE_GL|USE_UNIFORMS);
	// @}

	// ------------------------

	/*! @name Program and status*/
	// @{
	public:
		enum status_t{
			UNKNOWN = 0,
			COMPILED = 1,
			LINKED = 2,
			INVALID = 3
		};
		status_t getStatus()const				{	return status;	}
		uint32_t getShaderProg()const			{	return prog;	}

		//! Try to transfer the shader into LINKED-state. Returns true on success.
		RENDERINGAPI bool init();

	private:
		uint32_t prog;
		status_t status;

		/*! (internal) Compile all objects and create the shader program.
			If everything works fine, status is set to COMPILED and true is returned.
			Otherwise, status is set to INVALID and false is returned.	*/
		RENDERINGAPI bool compileProgram();

		/*! (internal) Link the program (which must already exist).
			If everything works fine, status is set to LINKED and true is returned.
			Otherwise, status is set to INVALID and false is returned.	*/
		RENDERINGAPI bool linkProgram();
	// @}

	// ------------------------

	/*! @name Shader Objects*/
	// @{
	private:
		std::vector<ShaderObjectInfo> shaderObjects;
	public:
		RENDERINGAPI void attachShaderObject(ShaderObjectInfo && obj);
	// @}

	// ------------------------

	/*! @name Uniforms */
	// @{
	private:
		const std::unique_ptr<UniformRegistry> uniforms;

		/*! (internal) Make sure that all uniforms declared in the shader are registered to the registry
			with their current value. Called when a shader is linked successfully */
		RENDERINGAPI void initUniformRegistry();

		/*! (internal)	Really set a value to a uniform variable of the shader.
			It is linked and that the uniform variable is used within the shader.
			@note *** matrices are committed in transposed order ***
			@param uniform Uniform variable.
			@param uniformLocation Valid uniform location for the current shader.
			@return @c true if successful, @c false if the uniform is invalid.	 */
		RENDERINGAPI static bool applyUniform(const Uniform & uniform, int32_t uniformLocation);

	public:
		//! (internal) should only be used by renderingContext
		UniformRegistry * _getUniformRegistry()const			{	return uniforms.get();	}

		/*! Apply those uniforms stored in the internal uniformRegistry to the shader, that have been changed since
			the last call to this function (or all, if forced is true).
			\note The Shader needs not to be active. */
		RENDERINGAPI void applyUniforms(bool forced=false);

		RENDERINGAPI bool isUniform(const Util::StringIdentifier name);

		/*! Get the values of all uniforms defined in the shader's program.
			\note The Shader needs not to be active.	*/
		RENDERINGAPI void getActiveUniforms(std::vector<Uniform> & uniforms);

		/*! Get the value of the uniform with the given name.
			If the name is not defined in the shader's program, the resulting Uniform is null (uniform.isNull()==true).
			\note The Shader needs not to be active.	*/
		RENDERINGAPI const Uniform & getUniform(const Util::StringIdentifier name);

		/*!	Set an Uniform. The uniform is applied when the shader is active and the renderingContext's changes are applied.
			\note The uniform is stored at the Shader's internal uniformRegistry.
			\note The Shader needs not to be active.*/
		RENDERINGAPI void setUniform(RenderingContext & rc,const Uniform & uniform, bool warnIfUnused=true, bool forced=false);
	// @}

	// ------------------------

	/*! @name Vertex attributes */
	// @{
	private:
		std::unordered_map<Util::StringIdentifier, int32_t> vertexAttributeLocations;
	public:
		RENDERINGAPI void defineVertexAttribute(const std::string & attrName, uint32_t index);
		RENDERINGAPI int32_t getVertexAttributeLocation(Util::StringIdentifier attrName);
	// @}


	// ------------------------

	/*! @name Transform Feedback */
	// @{
	public:
		RENDERINGAPI void setInterleavedFeedbackVaryings(const std::vector<std::string>& names);
		RENDERINGAPI void setSeparateFeedbackVaryings(const std::vector<std::string>& names);
	private:
		std::vector<std::string> feedbackVaryings;
		uint32_t glFeedbackVaryingType; //!< 0...disabled, interleaved, or separate
	// @}
	

	/*! @name Shader Subroutines */
	// @{
	public:
		RENDERINGAPI int32_t getSubroutineIndex(uint32_t stage, const std::string & name);
	// @}
};
}

#endif // SHADER_H
