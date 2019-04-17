/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_FBO_H
#define RENDERING_FBO_H

#include <Geometry/Rect.h>
#include <Util/ReferenceCounter.h>
#include <cstdint>

namespace Rendering {

class RenderingContext;
class Texture;

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
	@ingroup texture
*/
class FBO : public Util::ReferenceCounter<FBO> {
	public:
		FBO();
		~FBO();

		void prepare();
		void bind();
		void unbind();

		bool isComplete();
		bool isComplete(RenderingContext & context) __attribute((deprecated)) { return isComplete(); }
		
		const char * getStatusMessage();
		const char * getStatusMessage(RenderingContext & context) __attribute((deprecated)) { return getStatusMessage(); }

		void attachTexture(RenderingContext & context, uint32_t attachmentPoint, Texture * t, uint32_t level, int32_t layer=-1);
		void detachTexture(RenderingContext & context, uint32_t attachmentPoint) { attachTexture(context,attachmentPoint,nullptr,0,-1); }

		void attachColorTexture(RenderingContext & context, Texture * t, uint32_t colorBufferId = 0, uint32_t level=0, int32_t layer=-1);
		void detachColorTexture(RenderingContext & context, uint32_t colorBufferId = 0);
		void attachDepthStencilTexture(RenderingContext & context, Texture * t, uint32_t level=0, int32_t layer=-1);
		void detachDepthStencilTexture(RenderingContext & context);
		void attachDepthTexture(RenderingContext & context, Texture * t, uint32_t level=0, int32_t layer=-1);
		void detachDepthTexture(RenderingContext & context);

		/**
		 * Activate the given number of draw buffers.
		 *
		 * @param number Number of draw buffers to activate. Must be from [0, 8].
		 * @throw std::invalid_argument if @a number is greater than eight.
		 * @throw std::logic_error if the GL implementation does not support this functionality.
		 * @see function @c glDrawBuffers
		 */
	 	void setDrawBuffers(uint32_t number);
		
		//! copy a block of pixels from this framebuffer to another framebuffer
		void blit(RenderingContext & context, FBO* other, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect, bool includeDepth=false);
				
		//! copy a block of pixels from this framebuffer to the screen
		void blitToScreen(RenderingContext & context, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect, bool includeDepth=false) {
			blit(context, nullptr, srcRect, tgtRect, includeDepth);
		}
		
		uint32_t getHandle() const { return glId; }
	private:
		uint32_t glId;
};

}

#endif // RENDERING_FBO_H
