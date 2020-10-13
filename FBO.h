/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_FBO_H
#define RENDERING_FBO_H

#include "Core/Common.h"

#include <Geometry/Rect.h>
#include <Util/ReferenceCounter.h>
#include <cstdint>
#include <vector>

namespace Rendering {
class Texture;
using TextureRef = Util::Reference<Texture>;
//class TextureView;
//using TextureViewRef = Util::Reference<TextureView>;
class Device;
using DeviceRef = Util::Reference<Device>;
class RenderingContext;

/*! Representation of a frame buffer object (FBO).
	\see A good introduction http://www.songho.ca/opengl/gl_fbo.html
	\code
		// create a FBO
		Util::Reference<FBO> fbo = new FBO;

		// create a color and depthTexures
		Util::Reference<Texture> depthTexture = TextureUtils::createDepthTexture(width, height);
		Util::Reference<Texture> colorTexture = TextureUtils::createStdTexture(width, height, true);

		myRenderingContext.pushFBO(fbo.get());	// enable FBO

		// attach textures
		fbo->attachColorTexture(colorTexture.get());
		fbo->attachDepthTexture(depthTexture.get());

		// check framebuffer
		if(!fbo->isComplete()){
			WARN( fbo->getStatusMessage() );
			myRenderingContext.popFBO();
			return;
		}

		myRenderingContext.popFBO();	// disable FBO

		//...

		myRenderingContext.pushFBO(fbo.get());
		// ... do some rendering
		myRenderingContext.popFBO();

		// download a texture to access the pixel data
		colorTexture->downloadGLTexture();
	\endcode
	@ingroup rendering_resources
*/
class FBO : public Util::ReferenceCounter<FBO> {
public:
	struct Attachment {
		ImageViewHandle view;
		TextureRef texture;
		uint32_t mipLevel;
		uint32_t baseLayer;
		uint32_t layerCount;
	};
	using Ref = Util::Reference<FBO>;
			
	static Ref create(const DeviceRef& device);
	~FBO();

	//void attachColorTexture(const TextureViewRef& textureView, uint32_t index = 0);
	void attachColorTexture(const TextureRef& texture, uint32_t index = 0, uint32_t mipLevel=0, uint32_t baseLayer=0, uint32_t layerCount=0);
	void detachColorTexture(uint32_t index = 0);

	//void attachDepthStencilTexture(const TextureViewRef& textureView);
	void attachDepthStencilTexture(const TextureRef& texture, uint32_t mipLevel=0, uint32_t baseLayer=0, uint32_t layerCount=0);
	void detachDepthStencilTexture();
	
	const Attachment& getColorTexture(uint32_t index = 0) const;
	const Attachment& getDepthStencilTexture() const;
	
	uint32_t getWidth() const { return width; }
	uint32_t getHeight() const { return height; }
	
	bool isComplete();
	const std::string getStatusMessage();
	
	const FramebufferHandle& getApiHandle() const { return handle; }
	const RenderPassHandle& getRenderPass() const { return renderPass; }
private:
	FBO(const DeviceRef& device);
	bool validate();
	void init();
	
	FramebufferHandle handle;
	RenderPassHandle renderPass;
	Util::WeakPointer<Device> device;
	std::vector<Attachment> colorAttachments;
	Attachment depthStencilAttachment;
	uint32_t width = 0;
	uint32_t height = 0;
	bool isValid = false;

public:
	//! @name Deprecated
	//! @{
	
	[[deprecated]]
	FBO();

	[[deprecated]]
	static void _disable() { }

	[[deprecated]]
	void _enable() { isComplete(); }

	[[deprecated]]
	bool isComplete(RenderingContext & context) { return isComplete(); }

	[[deprecated]]
	const char * getStatusMessage(RenderingContext & context) { return getStatusMessage().c_str(); }

	[[deprecated]]
	void attachTexture(RenderingContext & context, uint32_t attachmentPoint, Texture * t, uint32_t level, int32_t layer=-1);

	[[deprecated]]
	void detachTexture(RenderingContext & context, uint32_t attachmentPoint) { attachTexture(context,attachmentPoint,nullptr,0,-1); }
	
	[[deprecated]]
	void attachColorTexture(RenderingContext & context, Texture * t, uint32_t colorBufferId = 0, uint32_t level=0, int32_t layer=-1);
	
	[[deprecated]]
	void detachColorTexture(RenderingContext & context, uint32_t colorBufferId = 0);
	
	[[deprecated]]
	void attachDepthStencilTexture(RenderingContext & context, Texture * t, uint32_t level=0, int32_t layer=-1);
	
	[[deprecated]]
	void detachDepthStencilTexture(RenderingContext & context);
	
	[[deprecated]]
	void attachDepthTexture(RenderingContext & context, Texture * t, uint32_t level=0, int32_t layer=-1);
	
	[[deprecated]]
	void detachDepthTexture(RenderingContext & context);

	[[deprecated]]
	void setDrawBuffers(RenderingContext & context, uint32_t number);

	[[deprecated]]
	void blitToScreen(RenderingContext & context, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect);

	//! @}		
};

}

#endif // RENDERING_FBO_H
