/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_TEXTRENDERER_H
#define RENDERING_TEXTRENDERER_H

#include <Util/References.h>

#include <memory>
#include <string>

namespace Geometry {
template<typename T_> class _Rect;
typedef _Rect<int> Rect_i;
template<typename T_> class _Vec2;
typedef _Vec2<int> Vec2i;
}
namespace Util {
class Bitmap;
class Color4f;
struct FontInfo;
}
namespace Rendering {
class RenderingContext;

/**
 * @brief Text rendering using bitmap fonts
 * 
 * Display text by using a bitmap that contains pre-rendered glyphs.
 * 
 * @author Benjamin Eikel
 * @date 2013-07-10
 * @ingroup draw
 */
class TextRenderer {
	private:
		// Use Pimpl idiom
		struct Implementation;
		std::unique_ptr<Implementation> impl;

	public:
		/**
		 * Create a text renderer using a glyph bitmap with the associated glyph
		 * mapping.
		 * 
		 * @param glyphBitmap Bitmap containing pre-rendered glyphs
		 * @param fontInfo Information about font metrics and mapping from
		 * characters to information about the glyphs
		 * @see Util::BitmapFont
		 */
		[[deprecated]]
		TextRenderer(const Util::Bitmap & glyphBitmap, const Util::FontInfo & fontInfo);
		TextRenderer(const Util::Reference<Util::Bitmap> & glyphBitmap, const Util::FontInfo & fontInfo) : TextRenderer(*glyphBitmap.get(), fontInfo) {}

		//! Free resources
		~TextRenderer();

		//! Default copy constructor
		TextRenderer(const TextRenderer & other);

		//! Default move constructor
		TextRenderer(TextRenderer && other);

		/**
		 * Draw the given text to the screen.
		 * 
		 * @param context Rendering context that is used for drawing
		 * @param text String that is to be drawn
		 * @param textPosition Screen position in pixels where to place the
		 * text. The position specifies the top left corner of the rendered
		 * text.
		 * @param textColor Color that is used to draw the text
		 * @note the 2D-rendering mode must be enabled ( @see Draw::enable2DMode(...) )
		 */
		void draw(RenderingContext & context, const std::u32string & text, const Geometry::Vec2i & textPosition, const Util::Color4f & textColor) const;

		/**
		 * Calculate the size that would be needed by the text when it was
		 * drawn.
		 * 
		 * @return Rectangle of the text on the screen in pixels
		 */
		Geometry::Rect_i getTextSize(const std::u32string & text) const;

		/**
		 * Return the height of the lower-case character 'x'.
		 * 
		 * @note This is similar to the unit ex in LaTeX.
		 * @return Height of 'x' in pixels, or zero if 'x' is not in the glyph
		 * map
		 */
		int getHeightOfX() const;

		/**
		 * Return the width of the upper-case character 'M'.
		 * 
		 * @note This is similar to the unit em in LaTeX.
		 * @return Width of 'M' in pixels, or zero if 'M' is not in the glyph
		 * map
		 */
		int getWidthOfM() const;
};

}

#endif /* RENDERING_TEXTRENDERER_H */
