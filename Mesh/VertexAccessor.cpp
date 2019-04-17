/*
 	This file is part of the Rendering library.
 	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
 	
 	This library is subject to the terms of the Mozilla Public License, v. 2.0.
 	You should have received a copy of the MPL along with this library; see the
 	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VertexAccessor.h"

#include <Util/Macros.h>

#include <iostream>

namespace Rendering {
  
static const std::string wrongAccessorType("Wrong accessor type for attribute named '");
  
template<typename Accessor>
Accessor* VertexAccessor::getAccessor(Util::StringIdentifier name) const {
  auto& acc = accessors[name];
  if(!acc) {
    if(!vData.getVertexDescription().hasAttribute(name))
      return nullptr;
    acc.reset(Accessor::create(vData, name).detach());
  }
  auto facc = dynamic_cast<Accessor*>(acc.get());
  if(!facc)
    WARN(wrongAccessorType + name.toString() + '\'');
  return facc;
}

const std::vector<float> VertexAccessor::getFloats(uint32_t index, Util::StringIdentifier name) const {
  auto acc = getAccessor<FloatAttributeAccessor>(name);
  return acc ? acc->getValues(index) : std::vector<float>{0.0f, 0.0f, 0.0f, 0.0f};
}

void VertexAccessor::setFloats(uint32_t index, const float* values, uint32_t count, Util::StringIdentifier name) {
  auto acc = getAccessor<FloatAttributeAccessor>(name);
  if(acc)
    acc->setValues(index, values, count);
}

const std::vector<uint32_t> VertexAccessor::getUInts(uint32_t index, Util::StringIdentifier name) const {
  auto acc = getAccessor<UIntAttributeAccessor>(name);
  return acc ? acc->getValues(index) : std::vector<uint32_t>{0,0,0,0};
}

void VertexAccessor::setUInts(uint32_t index, const uint32_t* values, uint32_t count, Util::StringIdentifier name) {
  auto acc = getAccessor<UIntAttributeAccessor>(name);
  if(acc)
    acc->setValues(index, values, count);
}

} /* Rendering */