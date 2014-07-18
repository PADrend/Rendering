/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Shader.h"
#include "Uniform.h"
#include "UniformRegistry.h"
#include "../RenderingContext/internal/RenderingStatus.h"
#include "../RenderingContext/RenderingContext.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include <Util/Macros.h>
#include <Util/StringIdentifier.h>
#include <Util/StringUtils.h>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace std;
namespace Rendering {

// -----------------------------------------
// static helper

//! (static)
void Shader::printProgramInfoLog(uint32_t obj) {
	int infoLogLength = 0;
	GET_GL_ERROR();
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infoLogLength);
	GET_GL_ERROR();
	if (infoLogLength > 1) {
		int charsWritten = 0;
		auto infoLog = new char[infoLogLength];
		glGetProgramInfoLog(obj, infoLogLength, &charsWritten, infoLog);
		string s(infoLog, charsWritten);
		// Skip "Everything ok" messages from AMD-drivers.
		if(s.find("successfully")==string::npos && s.find("shader(s) linked.")==string::npos && s.find("No errors.")==string::npos) {
			WARN(std::string("Program error:\n") + s);
		}
		delete [] infoLog;
	}
	GET_GL_ERROR();
}

// ---------------------------------------------------------------------------------------------
// Shader

//! (static)
Shader * Shader::loadShader(const Util::FileName & vsFile, const Util::FileName & fsFile, flag_t usage) {
	Shader * s = createShader(usage);
	s->attachShaderObject(ShaderObjectInfo::loadVertex(vsFile));
	s->attachShaderObject(ShaderObjectInfo::loadFragment(fsFile));
	return s;
}
#ifdef LIB_GL
//! (static)
Shader * Shader::loadShader(const Util::FileName & vsFile, const Util::FileName & gsFile, const Util::FileName & fsFile, flag_t usage) {
	Shader * s = createShader(usage);
	s->attachShaderObject(ShaderObjectInfo::loadVertex(vsFile));
	s->attachShaderObject(ShaderObjectInfo::loadGeometry(gsFile));
	s->attachShaderObject(ShaderObjectInfo::loadFragment(fsFile));
	return s;
}
#endif /* LIB_GL */
//! (static)
Shader * Shader::createShader(flag_t usage) {
	return new Shader(usage);
}

//! (static)
Shader * Shader::createShader(const std::string & vsa, const std::string & fsa, flag_t usage) {
	Shader * s = createShader(usage);
	s->attachShaderObject(ShaderObjectInfo::createVertex(vsa));
	s->attachShaderObject(ShaderObjectInfo::createFragment(fsa));
	return s;
}
#ifdef LIB_GL
//! (static)
Shader * Shader::createShader(const std::string & vsa, const std::string & gsa, const std::string & fsa, flag_t usage) {
	Shader * s = createShader(usage);
	s->attachShaderObject(ShaderObjectInfo::createVertex(vsa));
	s->attachShaderObject(ShaderObjectInfo::createGeometry(gsa));
	s->attachShaderObject(ShaderObjectInfo::createFragment(fsa));
	return s;
}
#endif /* LIB_GL */

// ------------------------------------------------------------------

/*!	[ctor]	*/
Shader::Shader(flag_t _usage) :
		usageFlags(_usage), renderingData(), prog(0), status(UNKNOWN), uniforms(new UniformRegistry),glFeedbackVaryingType(0){
}

/*!	[dtor]	*/
Shader::~Shader() {
	glDeleteProgram(prog);
}

bool Shader::init() {
	while(status!=LINKED){
		if(status == UNKNOWN){
			status = compileProgram() ? COMPILED : INVALID;
		}else if(status == COMPILED){
			if( linkProgram() ){
				status = LINKED;

				// recreate renderingData
				renderingData.reset(new RenderingStatus(this));

				// make sure all set uniforms are re-applied.
				uniforms->resetCounters();

				// initialize uniforms with default
				initUniformRegistry();
			}else{
				status = INVALID;
			}
		}else{ // if(status == INVALID)
			DEBUG("shader is invalid");
			return false;
		}
	}
	return true;
}

/*!	(internal) */
bool Shader::compileProgram() {
	prog = glCreateProgram();

	for(const auto & shaderObject : shaderObjects) {
		GLuint handle = shaderObject.compile();
		if(handle == 0) {
			GET_GL_ERROR();
			return false;
		}
		glAttachShader(prog, handle);
		glDeleteShader(handle);
	}
	return true;
}


//! (internal)
bool Shader::linkProgram() {
	
	// apply feedback varyings
	#if defined(GL_EXT_transform_feedback)
	if(!feedbackVaryings.empty() && RenderingContext::requestTransformFeedbackSupport()){
		const auto namesBuffer = new const char *[feedbackVaryings.size()];
		size_t i = 0;
		for(const auto& nameStr : feedbackVaryings)
			namesBuffer[i++] = nameStr.c_str();
		
		glTransformFeedbackVaryingsEXT(prog,feedbackVaryings.size(),namesBuffer,static_cast<GLenum>(glFeedbackVaryingType));
		delete [] namesBuffer;
	}
	#endif // GL_EXT_transform_feedback

	
	glLinkProgram(prog);
	GET_GL_ERROR();

	GLint linkStatus;
	glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		printProgramInfoLog(prog);
		GET_GL_ERROR();
		glDeleteProgram(prog);
		prog = 0;
		return false;
	}

	GET_GL_ERROR();
	return true;
}

void Shader::attachShaderObject(ShaderObjectInfo && obj) {
	shaderObjects.emplace_back(obj);
	status = UNKNOWN;
}

bool Shader::_enable() {
	if( status==LINKED || init() ){
		glUseProgram(prog);
		return true;
	}
	else
		return false;
}

bool Shader::enable(RenderingContext & rc){
	rc.setShader(this);
	return getStatus() == LINKED || init();
}

bool Shader::isActive(RenderingContext & rc){
	return rc.getActiveShader() == this;
}


// ----------------------------------------------------------
// Uniforms
void Shader::applyUniforms(bool forced){
	if( getStatus()!=LINKED && !init())
		return;

	// apply the uniforms that have been changed since the last call (or all, if forced)
	for(auto it=uniforms->orderedList.begin();
			it!=uniforms->orderedList.end() && ( (*it)->stepOfLastSet > uniforms->stepOfLastApply || forced ); ++it ){
		UniformRegistry::entry_t * entry(*it);

		// new uniform? --> query and store the location
		if( entry->location==-1 ){
			entry->location = glGetUniformLocation( getShaderProg(), entry->uniform.getName().c_str());
			if(entry->location==-1){
				entry->valid = false;
				if(entry->warnIfUnused)
					WARN(std::string("No uniform named: ") + entry->uniform.getName());
				continue;
			}
		}
		// set the data
		applyUniform(entry->uniform,entry->location);
	}
	uniforms->stepOfLastApply = UniformRegistry::getNewGlobalStep();
}

//! (internal)
bool Shader::applyUniform(const Uniform & uniform, int32_t uniformLocation) {
	switch (uniform.getType()) {
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
	}
	return true;
}

const Uniform & Shader::getUniform(const Util::StringIdentifier name) {
	// apply all pending changes, as this may lead to an invalidation of a newly set uniform which
	// would not be detected otherwise.
	applyUniforms();
	return uniforms->getUniform(name);
}

//! (internal)
void Shader::initUniformRegistry(){
	std::vector<Uniform> activeUniforms;

	getActiveUniforms(activeUniforms);
	for(const auto & activeUniform : activeUniforms) {
		uniforms->setUniform(activeUniform,true,false); // warn if the uniform is unused; though this should never happen
	}

	// as the uniforms are initialized with their original values, we don't need to re-apply them
	uniforms->stepOfLastApply = UniformRegistry::getNewGlobalStep();
}

bool Shader::isUniform(const Util::StringIdentifier name) {
	applyUniforms();
	return !uniforms->getUniform(name).isNull();
}


void Shader::getActiveUniforms(std::vector<Uniform> & activeUniforms){
	// make sure shader is ready and that all pending changes are applied.
	applyUniforms();
	if(getStatus()!=LINKED)
		return;

	GLint uniformCount = 0;
	glGetProgramiv(prog,GL_ACTIVE_UNIFORMS,&uniformCount);
	GLint bufSize=0;
	glGetProgramiv(prog,GL_ACTIVE_UNIFORM_MAX_LENGTH,&bufSize);
	auto nameBuffer=new char[bufSize];

	activeUniforms.reserve(uniformCount);

	for(GLint i=0;i<uniformCount;++i){
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
		switch(glType){
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

#ifdef LIB_GL
			case GL_SAMPLER_1D:
			case GL_SAMPLER_3D:
			case GL_SAMPLER_1D_SHADOW:
			case GL_SAMPLER_2D_SHADOW:
			case GL_IMAGE_1D:
			case GL_IMAGE_2D:
			case GL_IMAGE_3D:
			case GL_INT_IMAGE_1D:
			case GL_INT_IMAGE_2D:
			case GL_INT_IMAGE_3D:
			case GL_UNSIGNED_INT_IMAGE_1D:
			case GL_UNSIGNED_INT_IMAGE_2D:
			case GL_UNSIGNED_INT_IMAGE_3D:
#endif /* LIB_GL */
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
				WARN(std::string("getActiveUniforms: Unimplmented uniform type.")+name);
				continue;
			}

		}
		const size_t valueSize( Uniform::getValueSize(dataType) );

		// reserve memory for all values in the array
		std::vector<uint8_t> data(valueSize * arraySize);
		bool valid=true;
		// fetch the values
		for(int index=0;index<arraySize;++index){
			// add '[index]' for index>0
			const std::string name2(index==0? name : name+'['+Util::StringUtils::toString(index)+']');
			// query location
			const GLint location = glGetUniformLocation( getShaderProg(), name2.c_str() );
			if(location==-1){
//				WARN(std::string("Uniform not found (should not be possible):")+name2);
				valid=false;
				break;
			}

			if(readFloats)
				glGetUniformfv(prog,location,reinterpret_cast<GLfloat*>(data.data()+index*valueSize));
			else
				glGetUniformiv(prog,location,reinterpret_cast<GLint*>(data.data()+index*valueSize));
		}
		if(valid){
//				std::cout << name<<"\n";
			activeUniforms.emplace_back(name, dataType, arraySize, data);
		}
	}

	delete [] nameBuffer;
}

void Shader::setUniform(RenderingContext & rc,const Uniform & uniform, bool warnIfUnused, bool forced){
	if(!init()){
		WARN("setUniform: Shader not ready.");
		return;
	}
	rc._setUniformOnShader(this,uniform,warnIfUnused,forced);
}

// --------------------------------
// vertexAttributes

void Shader::defineVertexAttribute(const std::string & attrName, uint32_t index){
	if(!init()){
		WARN("defineVertexAttribute: Shader not ready.");
		return;
	}

	glBindAttribLocation(prog,index,attrName.c_str());
}

int32_t Shader::getVertexAttributeLocation(Util::StringIdentifier attrName){
	if(getStatus()!=LINKED && !init())
		return -1;

	auto it = vertexAttributeLocations.find(attrName);
	if( it == vertexAttributeLocations.end() ){
		GLint location = glGetAttribLocation(getShaderProg(), attrName.toString().c_str());
		vertexAttributeLocations[attrName] = location;
		return location;
	}
	return it->second;
}
// ---------------------------------
// feedback handler


void Shader::setInterleavedFeedbackVaryings(const std::vector<std::string>& names){
	if(RenderingContext::requestTransformFeedbackSupport()){
		feedbackVaryings = names;
		#if defined(GL_INTERLEAVED_ATTRIBS_EXT)
		glFeedbackVaryingType = static_cast<uint32_t>(GL_INTERLEAVED_ATTRIBS_EXT);
		#endif
		status = UNKNOWN;
	}
}
void Shader::setSeparateFeedbackVaryings(const std::vector<std::string>& names){
	if(RenderingContext::requestTransformFeedbackSupport()){
		feedbackVaryings = names;
		#if defined(GL_SEPARATE_ATTRIBS_EXT)
		glFeedbackVaryingType = static_cast<uint32_t>(GL_SEPARATE_ATTRIBS_EXT);
		#endif
		status = UNKNOWN;
	}
}
}
