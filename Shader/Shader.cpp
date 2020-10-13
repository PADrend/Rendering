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
#include "Shader.h"
#include "Uniform.h"
#include "UniformBuffer.h"
#include "UniformRegistry.h"
#include "../Core/Device.h"
#include "../Core/DescriptorPool.h"
#include "../Core/ResourceCache.h"
#include "../Helper.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
//#include <spirv-tools/linker.hpp>

#include <Util/Macros.h>
#include <Util/StringIdentifier.h>
#include <Util/StringUtils.h>
#include <Util/Utils.h>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace std;
namespace Rendering {

vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);

// ---------------------------------------------------------------------------------------------
// Shader

Shader::Ref Shader::loadShader(const DeviceRef& device, const Util::FileName & vsFile, const Util::FileName & fsFile) {
	Ref s = createShader(device);
	s->attachShaderObject(ShaderObjectInfo::loadVertex(vsFile));
	s->attachShaderObject(ShaderObjectInfo::loadFragment(fsFile));
	return s;
}

//-----------------

Shader::Ref Shader::loadShader(const DeviceRef& device, const Util::FileName & vsFile, const Util::FileName & gsFile, const Util::FileName & fsFile) {
	Ref s = createShader(device);
	s->attachShaderObject(ShaderObjectInfo::loadVertex(vsFile));
	s->attachShaderObject(ShaderObjectInfo::loadGeometry(gsFile));
	s->attachShaderObject(ShaderObjectInfo::loadFragment(fsFile));
	return s;
}

//-----------------

Shader::Ref Shader::loadComputeShader(const DeviceRef& device, const Util::FileName & csFile) {
	Ref s = createShader(device);
	s->attachShaderObject(ShaderObjectInfo::loadCompute(csFile));
	return s;
}

//-----------------

Shader::Ref Shader::createShader(const DeviceRef& device) {
	return new Shader(device);
}

//-----------------

Shader::Ref Shader::createShader(const DeviceRef& device, const std::string & vsa, const std::string & fsa) {
	Ref s = createShader(device);
	s->attachShaderObject(ShaderObjectInfo::createVertex(vsa));
	s->attachShaderObject(ShaderObjectInfo::createFragment(fsa));
	return s;
}

//-----------------

Shader::Ref Shader::createShader(const DeviceRef& device, const std::string & vsa, const std::string & gsa, const std::string & fsa) {
	Ref s = createShader(device);
	s->attachShaderObject(ShaderObjectInfo::createVertex(vsa));
	s->attachShaderObject(ShaderObjectInfo::createGeometry(gsa));
	s->attachShaderObject(ShaderObjectInfo::createFragment(fsa));
	return s;
}

//-----------------

Shader::Ref Shader::createComputeShader(const DeviceRef& device, const std::string & csa) {
	Ref s = createShader(device);
	s->attachShaderObject(ShaderObjectInfo::createCompute(csa));
	return s;
}

//-----------------


//! (static)
Shader * Shader::loadShader(const Util::FileName & vsFile, const Util::FileName & fsFile, flag_t usage) {
	return loadShader(Device::getDefault(), vsFile, fsFile).detachAndDecrease();
}

//-----------------

//! (static)
Shader * Shader::loadShader(const Util::FileName & vsFile, const Util::FileName & gsFile, const Util::FileName & fsFile, flag_t usage) {
	return loadShader(Device::getDefault(), vsFile, gsFile, fsFile).detachAndDecrease();
}

//-----------------

//! (static)
Shader * Shader::createShader(flag_t usage) {
	return createShader(Device::getDefault()).detachAndDecrease();
}

//-----------------

//! (static)
Shader * Shader::createShader(const std::string & vsa, const std::string & fsa, flag_t usage) {
	return createShader(Device::getDefault(), vsa, fsa).detachAndDecrease();
}

//-----------------

//! (static)
Shader * Shader::createShader(const std::string & vsa, const std::string & gsa, const std::string & fsa, flag_t usage) {
	return createShader(Device::getDefault(), vsa, gsa, fsa).detachAndDecrease();
}

//-----------------


// ------------------------------------------------------------------

/*!	[ctor]	*/
Shader::Shader(const DeviceRef& device) : device(device), status(UNKNOWN), uniforms(new UniformRegistry) { }

//-----------------

/*!	[dtor]	*/
Shader::~Shader() = default;

//-----------------

bool Shader::init() {
	while(status!=LINKED) {
		if(status == UNKNOWN) {
			status = compileProgram() ? COMPILED : INVALID;
		} else if(status == COMPILED) {
			if( linkProgram() ) {
				status = LINKED;

				// make sure all set uniforms are re-applied.
				uniforms->resetCounters();

				// initialize uniforms with default
				initUniformRegistry();
			} else {
				status = INVALID;
			}
		} else { // if(status == INVALID)
			DEBUG("shader is invalid");
			return false;
		}
	}
	return true;
}

//-----------------

/*!	(internal) */
bool Shader::compileProgram() {
	shaderModules.clear();
	vk::Device vkDevice(device->getApiHandle());
	for(auto& shaderObject : shaderObjects) {
		if(!shaderObject.compile(device))
			return false;
		auto& code = shaderObject.getCode();
		// TODO: shader modules are only needed during pipeline creation and can then deleted. Maybe only don't store them here?
		shaderModules.emplace(shaderObject.getType(), ShaderModuleHandle::create(vkDevice.createShaderModule({{}, static_cast<uint32_t>(code.size()) * sizeof(uint32_t), code.data()}), vkDevice));
	}
	return true;
}

//-----------------

//! (internal)
bool Shader::linkProgram() {
	using namespace ShaderUtils;
	vk::Device vkDevice(device->getApiHandle());

	resources.clear();
	vertexAttributeLocations.clear();
	
	// Merge resources from shader objects
	for(auto& obj : shaderObjects) {
		auto objResources = reflect(obj.getType(), obj.getCode());
		
		for(auto& resource : objResources) {
			std::string key = resource.name;

			// Update name as input and output resources can have the same name
			if(resource.layout.type == ShaderResourceType::Output || resource.layout.type == ShaderResourceType::Input) {
				key = toString(resource.layout.stages) + "_" + key;
			}

			// store vertex input attribute locations
			if(resource.layout.type == ShaderResourceType::Input && resource.layout.stages == ShaderStage::Vertex) {
				vertexAttributeLocations.emplace(Util::StringIdentifier(resource.name), static_cast<int32_t>(resource.location));
			}

			auto it = resources.find(key);
			if(it == resources.end()) {
				resources.emplace(key, resource);
			} else if(it->second == resource) {
				it->second.layout.stages = it->second.layout.stages | resource.layout.stages;
			} else {
				WARN("Shader: Cannot link shader. Resource missmatch: " + toString(it->second) + " != " + toString(resource));
				return false;
			}
		}
	}
	

	// Separate resources by set index
	std::vector<PushConstantRange> pushConstantRanges;
	std::map<uint32_t, ShaderResourceLayoutSet> layoutSets;
	for(const auto& it : resources) {
		const auto& res = it.second;
		if(hasBindingPoint(res.layout.type)) {
			layoutSets[res.set].setLayout(res.binding, res.layout);
		} else if(res.layout.type == ShaderResourceType::PushConstant) {
			pushConstantRanges.emplace_back(PushConstantRange{res.offset, res.size, res.layout.stages});
		}
	}
	layout.setLayoutSets(layoutSets);
	layout.setPushConstantRanges(pushConstantRanges);
	layoutHandle = device->getResourceCache()->createPipelineLayout(layout);

	WARN_IF(!layoutHandle, "Shader: Cannot link shader. Failed to create pipeline layout.");
	return layoutHandle.isNotNull();
}

//-----------------

void Shader::attachShaderObject(ShaderObjectInfo && obj) {
	shaderObjects.emplace_back(obj);
	status = UNKNOWN;
}

//-----------------

bool Shader::_enable() {
	if( status==LINKED || init() ) {
		return true;
	}
	else
		return false;
}

//-----------------

bool Shader::enable(RenderingContext & rc) {
	if(!init())
		return false;
	rc.setShader(this);
	return isActive(rc);
}

//-----------------

bool Shader::isActive(RenderingContext & rc) {
	return rc.getActiveShader().get() == this;
}

//-----------------

const ShaderResource& Shader::getResource(const Util::StringIdentifier& nameId) const {
	static const ShaderResource nullResource{};
	const auto& it = resources.find(nameId);
	if(it == resources.end())
		return nullResource;
	return it->second;
}

//-----------------
// ----------------------------------------------------------
// Uniforms
void Shader::applyUniforms(bool forced) {
	if( getStatus()!=LINKED && !init())
		return;

	// apply the uniforms that have been changed since the last call (or all, if forced)
	for(auto it=uniforms->orderedList.begin();
			it!=uniforms->orderedList.end() && ( (*it)->stepOfLastSet > uniforms->stepOfLastApply || forced ); ++it ) {
		UniformRegistry::entry_t * entry(*it);

		// new uniform? --> query and store the location
		if( entry->location==-1 ) {
			// find uniform
			entry->valid = false;
			for(const auto& rIt : uniformBuffers) {
				if(rIt.second->getFormat().hasAttribute(entry->uniform.getNameId())) {
					entry->set = static_cast<int32_t>(rIt.first.first); 
					entry->location = static_cast<int32_t>(rIt.first.second);
				}
			}
		}

		// set the data
		const auto& bIt = uniformBuffers.find({entry->set, entry->location});
		if(bIt != uniformBuffers.end()) {
			bIt->second->applyUniform(entry->uniform);
		}
	}
	uniforms->stepOfLastApply = UniformRegistry::getNewGlobalStep();
}

//-----------------

const Uniform & Shader::getUniform(const Util::StringIdentifier name) {
	// apply all pending changes, as this may lead to an invalidation of a newly set uniform which
	// would not be detected otherwise.
	applyUniforms();
	return uniforms->getUniform(name);
}

//-----------------

//! (internal)
void Shader::initUniformRegistry() {
	std::vector<Uniform> activeUniforms;
	uniformBuffers.clear();

	// allocate uniform buffers
	for(const auto& it : resources) {
		const auto& res = it.second;
		auto key = std::make_pair(res.set, res.binding);
		
		if(res.layout.type == ShaderResourceType::PushConstant) {
			key = std::make_pair<uint32_t,uint32_t>(std::numeric_limits<int32_t>::max(),std::numeric_limits<int32_t>::max());
		} else if(res.layout.type != ShaderResourceType::BufferUniform) {
			continue;
		}

		uniformBuffers[key] = UniformBuffer::createFromShaderResource(device, res);

		for(const auto& attr : res.format.getAttributes()) {
			Uniform::dataType_t type;
			switch (attr.getDataType()) {
				case Util::TypeConstant::INT32: type = Uniform::UNIFORM_INT; break;
				case Util::TypeConstant::FLOAT: type = Uniform::UNIFORM_FLOAT; break;
				case Util::TypeConstant::BOOL: type = Uniform::UNIFORM_BOOL; break;
				default:
					type = Uniform::UNIFORM_INT;
			}
			// TODO: There is currently no way to differentiate vector & matrix types. Maybe add to AttributeFormat?
			std::vector<uint8_t> data(attr.getDataSize(), 0);
			uniforms->setUniform({{attr.getNameId()}, type, attr.getComponentCount(), data},true,false);
		}
	}

	// as the uniforms are initialized with their original values, we don't need to re-apply them
	uniforms->stepOfLastApply = UniformRegistry::getNewGlobalStep();
}

//-----------------

bool Shader::isUniform(const Util::StringIdentifier name) {
	applyUniforms();
	return !uniforms->getUniform(name).isNull();
}

//-----------------


void Shader::getActiveUniforms(std::vector<Uniform> & activeUniforms) {
	// make sure shader is ready and that all pending changes are applied.
	applyUniforms();
	if(getStatus()!=LINKED)
		return;
	
	for(const auto& it : uniforms->orderedList) {
		if(it->valid) {
			activeUniforms.emplace_back(it->uniform);
		}
	}
}

//-----------------

void Shader::setUniform(RenderingContext & rc,const Uniform & uniform, bool warnIfUnused, bool forced) {
	WARN_AND_RETURN_IF(!init(), "setUniform: Shader not ready.",);
	uniforms->setUniform(uniform, warnIfUnused, forced);
}

// --------------------------------
// vertexAttributes

int32_t Shader::getVertexAttributeLocation(const Util::StringIdentifier& attrName) {
	if(getStatus()!=LINKED && !init())
		return -1;
	auto it = vertexAttributeLocations.find(attrName);
	return it != vertexAttributeLocations.end() ? it->second : -1;
}

//-----------------


}

//-----------------
