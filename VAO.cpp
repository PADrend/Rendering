/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VAO.h"

#include "Mesh/VertexAttribute.h"

#include "GLHeader.h"

namespace Rendering {

void VAO::prepare() {
  if(glHandle == 0)
	 glCreateVertexArrays(1, &glHandle);	
}

void VAO::destroy() {
  if(glHandle > 0)
    glDeleteVertexArrays(1, &glHandle);
  glHandle = 0;
}

void VAO::bind() {
  prepare();
  glBindVertexArray(glHandle);
}

void VAO::unbind() {
  glBindVertexArray(0);
}

void VAO::enableVertexAttrib(uint32_t location, const VertexAttribute& attr, uint32_t binding) {
  prepare();
  if(attr.empty()) {
    glDisableVertexArrayAttrib(glHandle, location);
  } else {
    glEnableVertexArrayAttrib(glHandle, location);
    if(attr.getConvertToFloat()) 
      glVertexArrayAttribFormat(glHandle, location, attr.getNumValues(), attr.getDataType(), attr.getNormalize() ? GL_TRUE : GL_FALSE, attr.getOffset());
    else
      glVertexArrayAttribIFormat(glHandle, location, attr.getNumValues(), attr.getDataType(), attr.getOffset());
    glVertexArrayAttribBinding(glHandle, location, binding);
  }
}

void VAO::disableVertexAttrib(uint32_t location) {
  prepare();
  glDisableVertexArrayAttrib(glHandle, location);
}

void VAO::bindVertexBuffer(uint32_t binding, uint32_t bufferId, uint32_t stride, uint32_t offset, uint32_t divisor) {
  prepare();
  glVertexArrayVertexBuffer(glHandle, binding, bufferId, offset, stride);
  glVertexArrayBindingDivisor(glHandle, binding, divisor);
}

void VAO::bindElementBuffer(uint32_t bufferId) {
  prepare();
  glVertexArrayElementBuffer(glHandle, bufferId);
}


} /* Rendering */
