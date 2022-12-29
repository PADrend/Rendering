/*
	This file is part of the Platform for Algorithm Development and Rendering (PADrend).
	Web page: http://www.padrend.de/
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2022 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_FRAMECONTEXT_H_
#define RENDERING_FRAMECONTEXT_H_

#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>

#include <nvrhi/nvrhi.h>
namespace Util {
namespace UI {
class Window;
} /* UI */
} /* Util */

namespace Rendering {
class RenderFrameContext;
using WindowHandle = Util::Reference<Util::UI::Window>;
using RenderFrameContextHandle = Util::Reference<RenderFrameContext>;

/**
 * @brief Manages resources and frame submission required for rendering to a window.
 */
class RenderFrameContext : public Util::ReferenceCounter<RenderFrameContext> {
	PROVIDES_TYPE_NAME(RenderFrameContext)
protected:
	RenderFrameContext(const Util::Reference<Util::UI::Window>& window);
public:
	virtual ~RenderFrameContext();

	/**
	 * @brief Refreshes the window surface, e.g., after a resize event.
	 * Needs to be called whenever the window size changes.
	 */
	virtual void refresh() = 0;

	/**
	 * @brief Starts a render frame.
	 * Needs to be called before rendering to a window.
	 */
	virtual void beginFrame() = 0;

	/**
	 * @brief Ends a render frame.
	 * Presents the currently rendered frame to the attached window and swaps render buffers.
	 */
	virtual void endFrame() = 0;

	/**
	 * @brief Get the currently active swapchain image.
	 */
	virtual nvrhi::TextureHandle getCurrentSwapchainImage() const = 0;

	/**
	 * @brief Get the currently active framebuffer.
	 */
	virtual nvrhi::FramebufferHandle getCurrentFramebuffer() const = 0;
protected:
	WindowHandle window;
};


} // Rendering

#endif // RENDERING_FRAMECONTEXT_H_