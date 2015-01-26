/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef RENDERING_STREAMMESHDATASTRATEGY_H_
#define RENDERING_STREAMMESHDATASTRATEGY_H_

#include "../Mesh/MeshDataStrategy.h"

namespace Rendering {

class StreamMeshDataStrategy : public MeshDataStrategy {
public:
	StreamMeshDataStrategy();
	virtual ~StreamMeshDataStrategy();

	void assureLocalVertexData(Mesh * m) override;
	void assureLocalIndexData(Mesh * m) override;
	void prepare(Mesh * m) override;
	void displayMesh(RenderingContext & context, Mesh * m,uint32_t startIndex,uint32_t indexCount) override;

	void uploadNextVertices(uint32_t addedElementCount) { this->vertexStreamEnd += addedElementCount; }
	uint32_t getStreamStart() const { return vertexStreamStart; }
private:
	uint32_t vertexStreamEnd;
	uint32_t vertexStreamStart;

	void upload(Mesh * m);
};

} /* namespace Rendering */

#endif /* RENDERING_STREAMMESHDATASTRATEGY_H_ */
