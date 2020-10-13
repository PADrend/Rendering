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
#include "UniformRegistry.h"
#include "../Core/Device.h"
#include "../Core/DescriptorPool.h"
#include "../Core/DescriptorSet.h"
#include "../RenderingContext/internal/RenderingStatus.h"
#include "../Helper.h"

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

				// recreate renderingData
				renderingData.reset(new RenderingStatus(this));

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
	//hash = 0;
	vk::Device vkDevice(device->getApiHandle());
	for(auto& shaderObject : shaderObjects) {
		if(!shaderObject.compile(device))
			return false;
		auto& code = shaderObject.getCode();
		//Util::hash_combine(hash, Util::calcHash(reinterpret_cast<const uint8_t*>(code.data()), code.size() * sizeof(uint32_t)));
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
	descriptorPools.clear();
	
	// Merge resources from shader objects
	for(auto& obj : shaderObjects) {
		auto objResources = reflect(obj.getType(), obj.getCode());
		if(objResources.empty())
			return false;
		
		for(auto& resource : objResources) {
			std::string key = resource.name;

			// Update name as input and output resources can have the same name
			if(resource.layout.type == ShaderResourceType::Output || resource.layout.type == ShaderResourceType::Input) {
				key = toString(resource.layout.stages) + "_" + key;
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
	for(auto& res : resources) {
		layoutSets[res.second.set].setLayout(res.second.binding, res.second.layout);
		if(res.second.layout.type == ShaderResourceType::PushConstant) {
			pushConstantRanges.emplace_back(PushConstantRange{res.second.offset, res.second.size, res.second.layout.stages});
		}
	}
	layout.setLayoutSets(layoutSets);
	layout.setPushConstantRanges(pushConstantRanges);

	// Create descriptor set pools
	for(auto& res : layoutSets) {
		DescriptorPool::Ref pool = new DescriptorPool(device, res.second);
		if(pool->init())
			descriptorPools.emplace(res.first, pool);
	}

	return true;
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
	return getStatus() == LINKED || init();
}

//-----------------

bool Shader::isActive(RenderingContext & rc) {
	return false;
}

//-----------------


// ----------------------------------------------------------
// Uniforms
void Shader::applyUniforms(bool forced) {
	if( getStatus()!=LINKED && !init())
		return;

	// apply the uniforms that have been changed since the last call (or all, if forced)
	/*for(auto it=uniforms->orderedList.begin();
			it!=uniforms->orderedList.end() && ( (*it)->stepOfLastSet > uniforms->stepOfLastApply || forced ); ++it ) {
		UniformRegistry::entry_t * entry(*it);

		// new uniform? --> query and store the location
		if( entry->location==-1 ) {
			entry->location = glGetUniformLocation( getShaderProg(), entry->uniform.getName().c_str());
			if(entry->location==-1) {
				entry->valid = false;
				if(entry->warnIfUnused)
					WARN(std::string("No uniform named: ") + entry->uniform.getName());
				continue;
			}
		}
		// set the data
		applyUniform(entry->uniform,entry->location);
	}
	uniforms->stepOfLastApply = UniformRegistry::getNewGlobalStep();*/
}

//-----------------

//! (internal)
bool Shader::applyUniform(const Uniform & uniform, int32_t uniformLocation) {
	/*switch (uniform.getType()) {
		case Uniform::UNIFORM_FLOAT: {
			glUniform1fv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLfloat *>(uniform.getData()));
			break;
		}
		case Uniform::UNIFORM_VEC2F:{
			glUniform2fv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLfloat *>(uniform.getData()));
			break;
		}
		case Uniform::UNIFORM_VEC3F:{
			glUniform3fv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLfloat *>(uniform.getData()));
			break;
		}
		case Uniform::UNIFORM_VEC4F:{
			glUniform4fv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLfloat *>(uniform.getData()));
			break;
		}

		case Uniform::UNIFORM_INT:
		case Uniform::UNIFORM_BOOL: {
			glUniform1iv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLint *>(uniform.getData()));
			break;
		}
		case Uniform::UNIFORM_VEC2B:
		case Uniform::UNIFORM_VEC2I:{
			glUniform2iv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLint *>(uniform.getData()));
			break;
		}
		case Uniform::UNIFORM_VEC3B:
		case Uniform::UNIFORM_VEC3I:{
			glUniform3iv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLint *>(uniform.getData()));
			break;
		}
		case Uniform::UNIFORM_VEC4B:
		case Uniform::UNIFORM_VEC4I:{
			glUniform4iv(uniformLocation, uniform.getNumValues(), reinterpret_cast<const GLint *>(uniform.getData()));
			break;
		}

		case Uniform::UNIFORM_MATRIX_2X2F: {
			glUniformMatrix2fv(uniformLocation, uniform.getNumValues(), GL_FALSE, reinterpret_cast<const GLfloat *> (uniform.getData()));
			break;
		}

		case Uniform::UNIFORM_MATRIX_3X3F: {
			glUniformMatrix3fv(uniformLocation, uniform.getNumValues(), GL_FALSE, reinterpret_cast<const GLfloat *> (uniform.getData()));
			break;
		}

		case Uniform::UNIFORM_MATRIX_4X4F: {
			glUniformMatrix4fv(uniformLocation, uniform.getNumValues(), GL_FALSE, reinterpret_cast<const GLfloat *> (uniform.getData()));
			break;
		}
		default:
			WARN("Unsupported data type of Uniform.");
			return false;
	}*/
	return true;
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

	getActiveUniforms(activeUniforms);
	for(const auto & activeUniform : activeUniforms) {
		uniforms->setUniform(activeUniform,true,false); // warn if the uniform is unused; though this should never happen
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

	/*GLint uniformCount = 0;
	glGetProgramiv(prog,GL_ACTIVE_UNIFORMS,&uniformCount);
	GLint bufSize=0;
	glGetProgramiv(prog,GL_ACTIVE_UNIFORM_MAX_LENGTH,&bufSize);
	auto nameBuffer=new char[bufSize];

	activeUniforms.reserve(uniformCount);

	for(GLint i=0;i<uniformCount;++i) {
		GLsizei nameLength=0;
		GLint arraySize=0;
		GLenum glType=0;
		glGetActiveUniform(prog,i,bufSize,&nameLength,&arraySize,&glType,nameBuffer);

		std::string name(nameBuffer,nameLength);

		// name is the name of an array (name[index]) -> strip the index
		if(name.length()>0&&name.at(name.length()-1) == ']')
			name=name.substr(0,name.rfind('['));

		// determine data type
		Uniform::dataType_t dataType;
		bool readFloats=false; // false :== read bool or int
		switch(glType) {
			// bool
			case GL_BOOL:{
				dataType = Uniform::UNIFORM_BOOL;
				break;
			}
			case GL_BOOL_VEC2:{
				dataType = Uniform::UNIFORM_VEC2B;
				break;
			}
			case GL_BOOL_VEC3:{
				dataType = Uniform::UNIFORM_VEC3B;
				break;
			}
			case GL_BOOL_VEC4:{
				dataType = Uniform::UNIFORM_VEC4B;
				break;
			}
			// float
			case GL_FLOAT:{
				dataType = Uniform::UNIFORM_FLOAT;
				readFloats = true;
				break;
			}
			case GL_FLOAT_VEC2:{
				dataType = Uniform::UNIFORM_VEC2F;
				readFloats = true;
				break;
			}
			case GL_FLOAT_VEC3:{
				dataType = Uniform::UNIFORM_VEC3F;
				readFloats = true;
				break;
			}
			case GL_FLOAT_VEC4:{
				dataType = Uniform::UNIFORM_VEC4F;
				readFloats = true;
				break;
			}
			// int
			case GL_INT:
			case GL_SAMPLER_2D:
			case GL_SAMPLER_CUBE:


			case GL_SAMPLER_1D:
			case GL_SAMPLER_1D_ARRAY:
			case GL_SAMPLER_2D_ARRAY:
			case GL_SAMPLER_3D:
			case GL_SAMPLER_1D_SHADOW:
			case GL_SAMPLER_2D_SHADOW:
			case GL_UNSIGNED_INT_SAMPLER_2D:
			case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			case GL_INT_SAMPLER_2D:
			case GL_IMAGE_1D:
			case GL_IMAGE_2D:
			case GL_IMAGE_3D:
			case GL_INT_IMAGE_1D:
			case GL_INT_IMAGE_2D:
			case GL_INT_IMAGE_3D:
			case GL_UNSIGNED_INT_IMAGE_1D:
			case GL_UNSIGNED_INT_IMAGE_2D:
			case GL_UNSIGNED_INT_IMAGE_3D:
			case GL_UNSIGNED_INT_ATOMIC_COUNTER:
			case GL_UNSIGNED_INT_IMAGE_BUFFER:
			case GL_INT_IMAGE_BUFFER:
			case GL_IMAGE_BUFFER:			
			case GL_UNSIGNED_INT:
			case GL_IMAGE_1D_ARRAY:
			case GL_IMAGE_2D_ARRAY:
			case GL_INT_IMAGE_1D_ARRAY:
			case GL_INT_IMAGE_2D_ARRAY:
			case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
			case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:

			{
				dataType = Uniform::UNIFORM_INT;
				break;
			}
			case GL_INT_VEC2:{
				dataType = Uniform::UNIFORM_VEC2I;
				break;
			}
			case GL_INT_VEC3:{
				dataType = Uniform::UNIFORM_VEC3I;
				break;
			}
			case GL_INT_VEC4:{
				dataType = Uniform::UNIFORM_VEC4I;
				break;
			}
			// matrix
			case GL_FLOAT_MAT2:{
				dataType = Uniform::UNIFORM_MATRIX_2X2F;
				readFloats = true;
				break;
			}
			case GL_FLOAT_MAT3:{
				dataType = Uniform::UNIFORM_MATRIX_3X3F;
				readFloats = true;
				break;
			}
			case GL_FLOAT_MAT4:{
				dataType = Uniform::UNIFORM_MATRIX_4X4F;
				readFloats = true;
				break;
			}
			default:{
				std::cout << "Uniform type: 0x" << std::hex << glType <<  std::dec<<"\n";
				WARN("getActiveUniforms: Unimplemented uniform type '"+name+"'");
				continue;
			}

		}
		const size_t valueSize( Uniform::getValueSize(dataType) );

		// reserve memory for all values in the array
		std::vector<uint8_t> data(valueSize * arraySize);
		bool valid=true;
		// fetch the values
		for(int index=0;index<arraySize;++index) {
			// add '[index]' for index>0
			const std::string name2(index==0? name : name+'['+Util::StringUtils::toString(index)+']');
			// query location
			const GLint location = glGetUniformLocation( getShaderProg(), name2.c_str() );
			if(location==-1) {
//				WARN(std::string("Uniform not found (should not be possible):")+name2);
				valid=false;
				break;
			}

			if(readFloats)
				glGetUniformfv(prog,location,reinterpret_cast<GLfloat*>(data.data()+index*valueSize));
			else
				glGetUniformiv(prog,location,reinterpret_cast<GLint*>(data.data()+index*valueSize));
		}
		if(valid) {
//				std::cout << name<<"\n";
			activeUniforms.emplace_back(name, dataType, arraySize, data);
		}
	}

	delete [] nameBuffer;*/
}

//-----------------

void Shader::setUniform(RenderingContext & rc,const Uniform & uniform, bool warnIfUnused, bool forced) {
	if(!init()) {
		WARN("setUniform: Shader not ready.");
		return;
	}
	//rc._setUniformOnShader(this,uniform,warnIfUnused,forced);
}

// --------------------------------
// vertexAttributes

int32_t Shader::getVertexAttributeLocation(Util::StringIdentifier attrName) {
	if(getStatus()!=LINKED && !init())
		return -1;

	auto it = resources.find(attrName.toString());
	if( it != resources.end() && it->second.layout.type == ShaderResourceType::Input ) {
		return static_cast<int32_t>(it->second.location);
	}
	return -1;
}

//-----------------


}

//-----------------
