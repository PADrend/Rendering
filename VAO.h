/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_VAO_H_
#define RENDERING_VAO_H_

#include <Util/ReferenceCounter.h>
#include <cstdint>

namespace Rendering {
class VertexAttribute;

class VAO : public Util::ReferenceCounter<VAO> {
public:
  ~VAO() { destroy(); }
  
  void prepare();
  void destroy();
  
  void bind();
  void unbind();
  
  void enableVertexAttrib(uint32_t location, const VertexAttribute& attr, uint32_t binding=0);
  void disableVertexAttrib(uint32_t location);
  
  void setVertexAttribValue(uint32_t location, float x, float y, float z, float w);
  
  void bindVertexBuffer(uint32_t binding, uint32_t bufferId, uint32_t stride, uint32_t offset=0, uint32_t divisor=0);
  void bindElementBuffer(uint32_t bufferId);
  
  uint32_t getHandle() const { return glHandle; }
private:
  uint32_t glHandle = 0;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_VAO_H_ */
