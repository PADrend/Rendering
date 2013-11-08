/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef NDEBUG

#include "QuadtreeMeshBuilder.h"
#include "../Texture/Texture.h"

#include <Util/IO/FileName.h>
#include <Util/Graphics/BitmapUtils.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Serialization/Serialization.h>
#include <Util/Utils.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>

namespace Rendering {
namespace MeshUtils {

static void drawLine(Util::PixelAccessor * pixelAccessor, const Util::Color4ub & color, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	// Using Bresenham's algorithm. See http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	const int32_t dx = std::abs(x2 - x1);
	const int32_t dy = std::abs(y2 - y1);
	const int8_t sx = (x1 < x2) ? 1 : -1;
	const int8_t sy = (y1 < y2) ? 1 : -1;
	int32_t err = dx - dy;

	while(x1 != x2 || y1 != y2) {
		pixelAccessor->writeColor(x1, y1, color);
		const int32_t err2 = 2 * err;
		if (err2 > -dy) {
			err -= dy;
			x1 += sx;
		}
		if (err2 < dx) {
			err += dx;
			y1 += sy;
		}
	}
}

void QuadtreeMeshBuilder::createDebugOutput(const std::deque<QuadtreeMeshBuilder::QuadTree *> & leaves, Util::PixelAccessor * sourceDepth, Util::PixelAccessor * sourceColor) {
	const uint32_t bitmapWidth = static_cast<uint32_t> (sourceDepth->getWidth());
	const uint32_t bitmapHeight = static_cast<uint32_t> (sourceDepth->getHeight());
	Util::Reference<Util::Bitmap> depthDebugBitmap = new Util::Bitmap(bitmapWidth, bitmapHeight, Util::PixelFormat::MONO_FLOAT);
	Util::Reference<Util::Bitmap> quadTreeDebugBitmap = new Util::Bitmap(bitmapWidth, bitmapHeight, Util::PixelFormat::RGBA);
	Util::Reference<Util::PixelAccessor> destQuadTree(Util::PixelAccessor::create(quadTreeDebugBitmap.get()));
	float depthMin = std::numeric_limits<float>::max();
	float depthMax = std::numeric_limits<float>::lowest();
	{
		for (uint_fast32_t y = 0; y < bitmapHeight; ++y) {
			for (uint_fast32_t x = 0; x < bitmapWidth; ++x) {
				const float depthValue = sourceDepth->readSingleValueFloat(x, y);
				if (depthValue < depthMin) {
					depthMin = depthValue;
				}
				if (depthValue > depthMax) {
					depthMax = depthValue;
				}
			}
		}
		Util::Reference<Util::PixelAccessor> destDepth(Util::PixelAccessor::create(depthDebugBitmap.get()));
		const float depthScale = depthMax - depthMin;
		for (uint_fast32_t y = 0; y < bitmapHeight; ++y) {
			for (uint_fast32_t x = 0; x < bitmapWidth; ++x) {
				const float depthValue = sourceDepth->readSingleValueFloat(x, y);
				destDepth->writeColor(x, bitmapHeight - y - 1, Util::Color4f((depthValue - depthMin) / depthScale, 0.0f, 0.0f, 0.0f));
				destQuadTree->writeColor(x, bitmapHeight - y - 1, Util::Color4ub(0, 0, 0, 0));
			}
		}
	}
	for (const auto & leaf : leaves) {
		const uint16_t xMin = leaf->getX();
		const uint16_t yMin = leaf->getY();
		const uint16_t xMax = leaf->getWidth() + xMin;
		const uint16_t yMax = leaf->getHeight() + yMin;
		for (uint_fast32_t x = xMin; x < xMax; ++x) {
			destQuadTree->writeColor(x, bitmapHeight - yMin - 1, Util::Color4ub(255, 255, 255, 127));
		}
		for (uint_fast32_t y = yMin; y < yMax; ++y) {
			destQuadTree->writeColor(xMin, bitmapHeight - y - 1, Util::Color4ub(255, 255, 255, 127));
		}
		const uint16_t xHalf = xMin + leaf->getWidth() / 2;
		const uint16_t yHalf = yMin + leaf->getHeight() / 2;

		const Util::Color4ub errorColor(255, 0, 255, 255);

		Util::Color4ub drawColor(255, 0, 0, 127);
		const QuadtreeMeshBuilder::QuadTree * west = leaf->getWestNeighbor();
		if(west != nullptr) {
			const uint16_t westXHalf = west->getX() + west->getWidth() / 2;
			const uint16_t westYHalf = west->getY() + west->getHeight() / 2;
			// Check if neighbor is correct.
			if(west->getHeight() < leaf->getHeight()) {
				drawColor = errorColor;
			}
			if(west->getX() + west->getWidth() != xMin) {
				drawColor = errorColor;
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1 - 1, westXHalf, bitmapHeight - westYHalf - 1 - 1);
		} else {
			// Check if neighbor should be missing.
			if(xMin != 0) {
				drawColor = errorColor;
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1 - 1, static_cast<int32_t>(0.75 * xMin + 0.25 * xMax), bitmapHeight - yHalf - 1 - 1);
		}

		drawColor = Util::Color4ub(255, 255, 0, 127);
		const QuadtreeMeshBuilder::QuadTree * east = leaf->getEastNeighbor();
		if(east != nullptr) {
			const uint16_t eastXHalf = east->getX() + east->getWidth() / 2;
			const uint16_t eastYHalf = east->getY() + east->getHeight() / 2;
			// Check if neighbor is correct.
			if(east->getHeight() < leaf->getHeight()) {
				drawColor = errorColor;
			}
			if(xMax != east->getX()) {
				drawColor = errorColor;
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1 + 1, eastXHalf, bitmapHeight - eastYHalf - 1 + 1);
		} else {
			// Check if neighbor should be missing.
			if(xMax != bitmapWidth - 1) {
				drawColor = errorColor; // Error
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1 + 1, static_cast<int32_t>(0.25 * xMin + 0.75 * xMax), bitmapHeight - yHalf - 1 + 1);
		}

		drawColor = Util::Color4ub(0, 0, 255, 127);
		const QuadtreeMeshBuilder::QuadTree * north = leaf->getNorthNeighbor();
		if(north != nullptr) {
			const uint16_t northXHalf = north->getX() + north->getWidth() / 2;
			const uint16_t northYHalf = north->getY() + north->getHeight() / 2;
			// Check if neighbor is correct.
			if(north->getWidth() < leaf->getWidth()) {
				drawColor = errorColor;
			}
			if(north->getY() + north->getHeight() != yMin) {
				drawColor = errorColor;
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1, northXHalf, bitmapHeight - northYHalf - 1);
		} else {
			// Check if neighbor should be missing.
			if(yMin != 0) {
				drawColor = errorColor; // Error
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1, xHalf, static_cast<int32_t>(bitmapHeight - (0.75 * yMin + 0.25 * yMax) - 1));
		}

		drawColor = Util::Color4ub(0, 255, 255, 127);
		const QuadtreeMeshBuilder::QuadTree * south = leaf->getSouthNeighbor();
		if(south != nullptr) {
			const uint16_t southXHalf = south->getX() + south->getWidth() / 2;
			const uint16_t southYHalf = south->getY() + south->getHeight() / 2;
			// Check if neighbor is correct.
			if(south->getWidth() < leaf->getWidth()) {
				drawColor = errorColor;
			}
			if(yMax != south->getY()) {
				drawColor = errorColor;
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1, southXHalf, bitmapHeight - southYHalf - 1);
		} else {
			// Check if neighbor should be missing.
			if(yMax != bitmapHeight - 1) {
				drawColor = errorColor; // Error
			}
			drawLine(destQuadTree.get(), drawColor, xHalf, bitmapHeight - yHalf - 1, xHalf, static_cast<int32_t>(bitmapHeight - (0.25 * yMin + 0.75 * yMax) - 1));
		}

		destQuadTree->writeColor(xHalf, bitmapHeight - yHalf - 1, Util::Color4ub(0, 0, 0, 127));
	}
	const std::string currentTime = Util::Utils::createTimeStamp();
	if(sourceColor != nullptr) {
		Util::Reference<Util::Bitmap> colorDebugBitmap = new Util::Bitmap(bitmapWidth, bitmapHeight, Util::PixelFormat::RGB);
		Util::Reference<Util::PixelAccessor> destColor(Util::PixelAccessor::create(colorDebugBitmap.get()));
		for (uint_fast32_t y = 0; y < bitmapHeight; ++y) {
			for (uint_fast32_t x = 0; x < bitmapWidth; ++x) {
				destColor->writeColor(x, bitmapHeight - y - 1, sourceColor->readColor4f(x, y));
			}
		}
		Util::Serialization::saveBitmap(*colorDebugBitmap.get(), Util::FileName("screens/QuadTreeMeshBuilder_" + currentTime + "_Color.png"));
	}
	Util::Serialization::saveBitmap(*depthDebugBitmap.get(), Util::FileName("screens/QuadTreeMeshBuilder_" + currentTime + "_Depth.png"));
	Util::Serialization::saveBitmap(*quadTreeDebugBitmap.get(), Util::FileName("screens/QuadTreeMeshBuilder_" + currentTime + "_QuadTree.png"));
	std::ofstream textDebug("screens/QuadTreeMeshBuilder_Information.txt", std::ios_base::out | std::ios_base::app);
	textDebug << currentTime << '\t' << leaves.size() << '\t' << depthMin << '\t' << depthMax << '\n';
//	sleep(1);
}

}
}

#endif /* not NDEBUG */
