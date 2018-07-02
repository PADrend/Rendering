/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STATHANDLER_SGUNI_H_
#define RENDERING_STATHANDLER_SGUNI_H_
namespace Rendering {

class RenderingStatus;
class Shader;

namespace StatusHandler_UBO{

void apply(Shader* shader, RenderingStatus & target, const RenderingStatus & actual, bool forced);

}
}
#endif /* RENDERING_STATHANDLER_SGUNI_H_ */
