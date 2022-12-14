/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "graphics/Material_FontNormal.h"
#include "graphics/LowLevelGraphics.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cMaterial_FontNormal::cMaterial_FontNormal(cImageManager* apImageManager)
		: iOldMaterial(eOldMaterialType_FontNormal, apImageManager)
	{
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	void cMaterial_FontNormal::StartRendering(iLowLevelGraphics* apLowLevelGraphics) const
	{
		apLowLevelGraphics->SetBlendActive(true);
		apLowLevelGraphics->SetBlendFunc(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha);

		apLowLevelGraphics->SetTexture(0, GetTexture());
	}

	//-----------------------------------------------------------------------

	void cMaterial_FontNormal::EndRendering(iLowLevelGraphics* apLowLevelGraphics) const {
	}

	//-----------------------------------------------------------------------
}
