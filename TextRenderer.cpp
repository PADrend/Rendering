/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TextRenderer.h"
#include "Mesh/Mesh.h"
#include "Mesh/VertexDescription.h"
#include "Mesh/VertexAttributeAccessors.h"
#include "Mesh/MeshDataStrategy.h"
#include "RenderingContext/RenderingParameters.h"
#include "RenderingContext/RenderingContext.h"
#include "Shader/Shader.h"
#include "Shader/Uniform.h"
#include "Texture/Texture.h"
#include "Texture/TextureUtils.h"
#include "Draw.h"
#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/FontRenderer.h>
#include <Util/References.h>
#include <cstdint>
#include <string>

namespace Rendering {

static const std::string vertexProgram(R"***(#version 420
#extension GL_ARB_shader_draw_parameters : require
layout(location = 0) in vec2 sg_Position;
layout(location = 1) in vec2 sg_TexCoord0;
layout(location = 2) in vec2 glyphDim;

out Glyph {
	vec2 pos;
	vec2 tex;
	vec2 dim;
	uint drawId;
} vOut;

void main(void) {
	vOut.pos = sg_Position;
	vOut.tex = sg_TexCoord0;
	vOut.dim = glyphDim;
	vOut.drawId = gl_BaseInstanceARB;
}
)***");

static const std::string geometryProgram(R"***(#version 420
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
	
layout(std140, binding=0, row_major) uniform FrameData {
  mat4 sg_matrix_worldToCamera;
  mat4 sg_matrix_cameraToWorld;
  mat4 sg_matrix_cameraToClipping;
  mat4 sg_matrix_clippingToCamera;
  vec4 sg_viewport;
};
struct Object {
  mat4 sg_matrix_modelToCamera;
  float sg_pointSize;
  uint materialId;
  uint lightSetId;
  uint drawId;
};
layout(std140, binding=4, row_major) uniform ObjectData {
  Object objects[512];
};

in Glyph {
	vec2 pos;
	vec2 tex;
	vec2 dim;
	uint drawId;
} vIn[];

out vec2 glyphPos;

void main(void) {
	mat4 sg_matrix_modelToClipping = sg_matrix_cameraToClipping * objects[vIn[0].drawId].sg_matrix_modelToCamera;
	
	vec2 pos = vIn[0].pos;
	vec2 tex = vIn[0].tex;
	vec2 dim = vIn[0].dim;
	
	// Top left
	glyphPos = tex;
	gl_Position = (sg_matrix_modelToClipping * vec4(pos, 0.0, 1.0));
  EmitVertex();	
	
	// Bottom left
	glyphPos = tex + vec2(0, -dim.y);
	gl_Position = (sg_matrix_modelToClipping * vec4(pos + vec2(0, dim.y), 0.0, 1.0));
  EmitVertex();
	
	// Top right
	glyphPos = tex + vec2(dim.x, 0);
	gl_Position = (sg_matrix_modelToClipping * vec4(pos + vec2(dim.x, 0), 0.0, 1.0));
  EmitVertex();
	
	// Bottom right
	glyphPos = tex + vec2(dim.x, -dim.y);
	gl_Position = (sg_matrix_modelToClipping * vec4(pos + dim, 0.0, 1.0));
  EmitVertex();
	
  EndPrimitive();
}
)***");

static const std::string fragmentProgram(R"***(#version 420

layout(binding=0) uniform sampler2D glyphTexture;
uniform vec4 textColor;

in vec2 glyphPos;
out vec4 fragColor;

void main(void) {
	fragColor = vec4(1.0, 1.0, 1.0, texelFetch(glyphTexture, ivec2(glyphPos), 0).r) * textColor;
}
)***");

struct TextRenderer::Implementation {
	Util::Reference<Shader> shader;
	Util::Reference<Texture> texture;
	Util::FontInfo fontInfo;
	uint32_t characterBufferSize = 128;
	Util::Reference<Mesh> mesh;
};
static Util::StringIdentifier GLYPH_DIM_ATTR("glyphDim");

static Mesh* initializeMesh(uint32_t maxCharacters) {
	VertexDescription vertexDescription;
	vertexDescription.appendPosition2D();
	vertexDescription.appendTexCoord();
	vertexDescription.appendFloatAttribute(GLYPH_DIM_ATTR, 2);
	auto mesh = new Mesh(vertexDescription, maxCharacters, 0);
	mesh->setUseIndexData(false);
	mesh->setDrawMode(Mesh::DRAW_POINTS);
	mesh->setDataStrategy(SimpleMeshDataStrategy::getDynamicVertexStrategy());
	return mesh;
}

TextRenderer::TextRenderer(const Util::Bitmap & glyphBitmap, const Util::FontInfo & fontInfo) : impl(new Implementation) {
	impl->texture = TextureUtils::createTextureFromBitmap(glyphBitmap);
	impl->fontInfo = fontInfo;
}

TextRenderer::~TextRenderer() = default;

TextRenderer::TextRenderer(const TextRenderer & other) :
		impl(new Implementation(*other.impl.get())) {
}

TextRenderer::TextRenderer(TextRenderer &&) = default;

void TextRenderer::draw(RenderingContext & context,
						const std::u32string & text,
						const Geometry::Vec2i & textPosition,
						const Util::Color4f & textColor) const {
	if(impl->shader.isNull()) {
		impl->shader = Shader::createShader(vertexProgram, geometryProgram, fragmentProgram, Shader::USE_UNIFORMS);
	}
	context.pushAndSetBlending(BlendingParameters(BlendingParameters::SRC_ALPHA, BlendingParameters::ONE_MINUS_SRC_ALPHA));
	context.pushAndSetDepthBuffer(DepthBufferParameters(false, false, Comparison::LESS));
	context.pushAndSetShader(impl->shader.get());
	context.pushAndSetTexture(0, impl->texture.get());

	impl->shader->setUniform(context, Uniform("textColor", textColor));

	if(impl->mesh.isNull()) {
		impl->mesh = initializeMesh(impl->characterBufferSize);
	}
	
	auto& data = impl->mesh->openVertexData();
	auto posAcc = TexCoordAttributeAccessor::create(data, VertexAttributeIds::POSITION);
	auto texAcc = TexCoordAttributeAccessor::create(data, VertexAttributeIds::TEXCOORD0);
	auto glyphAcc = TexCoordAttributeAccessor::create(data, GLYPH_DIM_ATTR);
	
	const auto textureHeight = static_cast<int32_t>(impl->texture->getHeight());

	int cursorX = textPosition.getX();
	uint32_t start = 0;
	uint32_t cursor = 0;
	for(const auto & character : text) {
		const auto it = impl->fontInfo.glyphMap.find(character);
		if(it == impl->fontInfo.glyphMap.cend()) {
			// Skip missing characters
			continue;
		}
		const auto & glyphInfo = it->second;

		const Geometry::Vec2i topLeftPos(cursorX + glyphInfo.offset.first,
										 textPosition.getY() + impl->fontInfo.ascender - glyphInfo.offset.second);
		const Geometry::Vec2i topLeftTexCoord(glyphInfo.position.first,
											  textureHeight - glyphInfo.position.second);
		const Geometry::Vec2i glyphDim(glyphInfo.size.first, glyphInfo.size.second);

		posAcc->setCoordinate(cursor, topLeftPos);
		texAcc->setCoordinate(cursor, topLeftTexCoord);
		glyphAcc->setCoordinate(cursor, glyphDim);
		
		cursorX += glyphInfo.xAdvance;
		++cursor;
		
		if(cursor >= impl->characterBufferSize) {
			data.markAsChanged();
			context.displayMesh(impl->mesh.get(), start, cursor);
			start = 0;
			cursor = 0;
		}
	}
	
	if(cursor > start) {
		data.markAsChanged();
		context.displayMesh(impl->mesh.get(), start, cursor);
	}

	context.popTexture(0);
	context.popShader();
	context.popDepthBuffer();
	context.popBlending();
}

Geometry::Rect_i TextRenderer::getTextSize(const std::u32string & text) const {
	Geometry::Rect_i textRect;
	textRect.invalidate();

	int cursorX = 0;
	for(const auto & character : text) {
		const auto it = impl->fontInfo.glyphMap.find(character);
		if(it == impl->fontInfo.glyphMap.cend()) {
			// Skip missing characters
			continue;
		}
		const auto & glyphInfo = it->second;

		const auto glyphWidth = glyphInfo.size.first;
		const auto glyphHeight = glyphInfo.size.second;
		
		const Geometry::Vec2i topLeftPos(cursorX + glyphInfo.offset.first,
										 impl->fontInfo.ascender - glyphInfo.offset.second);

		// Top left
		textRect.include(topLeftPos);
		// Bottom right
		textRect.include(topLeftPos + Geometry::Vec2i(glyphWidth, glyphHeight));

		cursorX += glyphInfo.xAdvance;
	}

	return textRect;
}

int TextRenderer::getHeightOfX() const {
	const auto it = impl->fontInfo.glyphMap.find('x');
	if(it == impl->fontInfo.glyphMap.cend()) {
		return 0;
	}
	const auto & glyphInfo = it->second;
	return glyphInfo.size.second;
}

int TextRenderer::getWidthOfM() const {
	const auto it = impl->fontInfo.glyphMap.find('M');
	if(it == impl->fontInfo.glyphMap.cend()) {
		return 0;
	}
	const auto & glyphInfo = it->second;
	return glyphInfo.size.first;
}

}
