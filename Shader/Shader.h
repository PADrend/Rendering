/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADER_H
#define RENDERING_SHADER_H

#include "../Core/Common.h"

#include "ShaderObjectInfo.h"
#include "../RenderingContext.h"

#include <Util/ReferenceCounter.h>
#include <Util/StringIdentifier.h>

#include <cstdint>
#include <memory>
#include <string>
#include <map>
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
class Device;
using DeviceRef = Util::Reference<Device>;
class DescriptorPool;
using DescriptorPoolRef = Util::Reference<DescriptorPool>;
class UniformBuffer;
using UniformBufferRef = Util::Reference<UniformBuffer>;

//! @defgroup shader Shader

/** Shader
 * @ingroup shader
 */
class Shader : public Util::ReferenceCounter<Shader> {
	/*! @name Main */
	// @{
	public:
		using Ref = Util::Reference<Shader>;
		typedef uint32_t flag_t;
		static const flag_t USE_GL = 1 << 0;
		static const flag_t USE_UNIFORMS = 1 << 1;

		~Shader();

		//! (internal) called by RenderingContext
		[[deprecated]]
		bool _enable();

		/*! Sets the active shader at the renderingContext. If the shader
			has not been linked, it is linked.
			\return returns true if the status of the shader is LINKED */
		[[deprecated]]
		bool enable(RenderingContext & rc);
		[[deprecated]]
		bool isActive(RenderingContext & rc); // (???) !=enabled

		[[deprecated]]
		bool usesClassicOpenGL() const { return false; }
		[[deprecated]]
		bool usesSGUniforms() const { return true; }
		[[deprecated]]
		void setUsage(flag_t newUsage) { }

		[[deprecated]]
		RenderingStatus * getRenderingStatus() { return renderingData.get(); }

		const DeviceRef& getDevice() const { return device; }
	private:
		std::unique_ptr<RenderingStatus> renderingData; // created when the shader is successfully initialized
		DeviceRef device;

		Shader(const DeviceRef& device);
	// @}

	// ------------------------

	/*! @name (static) Factories */
	// @{
	public:
		static Ref loadShader(const DeviceRef& device, const Util::FileName & vsFile, const Util::FileName & fsFile);
		static Ref loadShader(const DeviceRef& device, const Util::FileName & vsFile, const Util::FileName & gsFile, const Util::FileName & fsFile);
		static Ref loadComputeShader(const DeviceRef& device, const Util::FileName & csFile);
		static Ref createShader(const DeviceRef& device);
		static Ref createShader(const DeviceRef& device, const std::string & vsa, const std::string & fsa);
		static Ref createShader(const DeviceRef& device, const std::string & vsa, const std::string & gsa, const std::string & fsa);
		static Ref createComputeShader(const DeviceRef& device, const std::string & csa);

		[[deprecated]]
		static Shader * loadShader(const Util::FileName & vsFile, const Util::FileName & fsFile, flag_t usage = USE_GL|USE_UNIFORMS);
		[[deprecated]]
		static Shader * loadShader(const Util::FileName & vsFile, const Util::FileName & gsFile, const Util::FileName & fsFile, flag_t usage = USE_GL|USE_UNIFORMS);
		[[deprecated]]
		static Shader * createShader(flag_t usage = USE_GL|USE_UNIFORMS);
		[[deprecated]]
		static Shader * createShader(const std::string & vsa, const std::string & fsa, flag_t usage = USE_GL|USE_UNIFORMS);
		[[deprecated]]
		static Shader * createShader(const std::string & vsa, const std::string & gsa, const std::string & fsa, flag_t usage = USE_GL|USE_UNIFORMS);
	// @}

	// ------------------------

	/*! @name Program and status */
	// @{
	public:
		enum status_t {
			UNKNOWN = 0,
			COMPILED = 1,
			LINKED = 2,
			INVALID = 3
		};
		status_t getStatus() const { return status; }

		[[deprecated]]
		uint32_t getShaderProg() const { return 0; }

		//! Try to transfer the shader into LINKED-state. Returns true on success.
		bool init();

	private:
		status_t status = UNKNOWN;

		/*! (internal) Compile all objects and create the shader program.
			If everything works fine, status is set to COMPILED and true is returned.
			Otherwise, status is set to INVALID and false is returned. */
		bool compileProgram();

		/*! (internal) Link the program (which must already exist).
			If everything works fine, status is set to LINKED and true is returned.
			Otherwise, status is set to INVALID and false is returned. */
		bool linkProgram();
	// @}

	// ------------------------

	/*! @name Shader Resources */
	// @{
	public:
		const ShaderLayout& getLayout() const { return layout; }
		const PipelineLayoutHandle& getLayoutHandle() const { return layoutHandle; }
		
		const std::map<uint32_t, DescriptorPoolRef>& getDescriptorPools() const { return descriptorPools; }
		const DescriptorPoolRef& getDescriptorPool(uint32_t set) const { return descriptorPools.at(set); }
		const ShaderResource& getResource(const Util::StringIdentifier& nameId) const;
		const std::unordered_map<Util::StringIdentifier, ShaderResource>& getResources() const { return resources; }
	private:
		ShaderLayout layout;
		PipelineLayoutHandle layoutHandle;
		std::unordered_map<Util::StringIdentifier, ShaderResource> resources;		
		std::map<uint32_t, DescriptorPoolRef> descriptorPools;
	// @}

	// ------------------------

	// ------------------------

	/*! @name Shader Objects*/
	// @{
	private:
		std::vector<ShaderObjectInfo> shaderObjects;
		std::map<ShaderStage, ShaderModuleHandle> shaderModules;
	public:
		void attachShaderObject(ShaderObjectInfo && obj);
		const std::map<ShaderStage, ShaderModuleHandle>& getShaderModules() const { return shaderModules; }
	// @}

	// ------------------------

	/*! @name Uniforms */
	// @{
	private:
		const std::unique_ptr<UniformRegistry> uniforms;
		std::map<std::pair<uint32_t,uint32_t>, UniformBufferRef> uniformBuffers;

		/*! (internal) Make sure that all uniforms declared in the shader are registered to the registry
			with their current value. Called when a shader is linked successfully */
		void initUniformRegistry();

	public:
		//! (internal) should only be used by renderingContext
		UniformRegistry * _getUniformRegistry() const { return uniforms.get(); }

		/*! Apply those uniforms stored in the internal uniformRegistry to the shader, that have been changed since
			the last call to this function (or all, if forced is true).
			\note The Shader needs not to be active. */
		void applyUniforms(bool forced=false);

		bool isUniform(const Util::StringIdentifier name);

		/*! Get the values of all uniforms defined in the shader's program.
			\note The Shader needs not to be active. */
		void getActiveUniforms(std::vector<Uniform> & uniforms);

		/*! Get the value of the uniform with the given name.
			If the name is not defined in the shader's program, the resulting Uniform is null (uniform.isNull()==true).
			\note The Shader needs not to be active. */
		const Uniform & getUniform(const Util::StringIdentifier name);

		/*! Set an Uniform. The uniform is applied when the shader is active and the renderingContext's changes are applied.
			\note The uniform is stored at the Shader's internal uniformRegistry.
			\note The Shader needs not to be active.*/
		void setUniform(RenderingContext & rc,const Uniform & uniform, bool warnIfUnused=true, bool forced=false);
	// @}

	// ------------------------

	/*! @name Vertex attributes */
	// @{
	private:
		std::unordered_map<Util::StringIdentifier, int32_t> vertexAttributeLocations;
	public:
		[[deprecated]]
		void defineVertexAttribute(const std::string & attrName, uint32_t index) {}
		int32_t getVertexAttributeLocation(const Util::StringIdentifier& attrName);
	// @}


	// ------------------------

	/*! @name Transform Feedback */
	// @{
	public:
		[[deprecated]]
		void setInterleavedFeedbackVaryings(const std::vector<std::string>& names) {}
		[[deprecated]]
		void setSeparateFeedbackVaryings(const std::vector<std::string>& names) {}
	// @}
	

	/*! @name Shader Subroutines */
	// @{
	public:
		[[deprecated]]
		int32_t getSubroutineIndex(uint32_t stage, const std::string & name) { return -1; }
	// @}
};
}

#endif // RENDERING_SHADER_H
