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
#include "resources/AnimationManager.h"
#include "system/String.h"
#include "system/System.h"
#include "resources/Resources.h"
#include "graphics/Mesh.h"
#include "graphics/Animation.h"
#include "system/LowLevelSystem.h"
#include "resources/MeshLoaderHandler.h"
#include "resources/FileSearcher.h"


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cAnimationManager::cAnimationManager(cGraphics* apGraphic,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpGraphics = apGraphic;
		mpResources = apResources;
	}

	cAnimationManager::~cAnimationManager()
	{
		DestroyAll();

		Log(" Done with animations\n");
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cAnimation* cAnimationManager::CreateAnimation(const tString& asName)
	{
		tString sPath;
		cAnimation *pAnimation=NULL;
		tString asNewName;

		BeginLoad(asName);

		asNewName = asName;

		//If the file is missing an extension, search for an existing file.
		if(cString::GetFileExt(asNewName) == "")
		{
			bool bFound = false;
			tStringVec *pTypes = mpResources->GetMeshLoaderHandler()->GetSupportedTypes();
			for(size_t i=0; i< pTypes->size(); i++)
			{
				asNewName = cString::SetFileExt(asNewName, (*pTypes)[i]);
				sPath = mpResources->GetFileSearcher()->GetFilePath(asNewName);
				if(sPath != "")
				{
					bFound = true;
					break;
				}
			}

			if(bFound == false){
				Error("Couldn't create mesh '%s'\n",asName.c_str());
				EndLoad();
				return NULL;
			}
		}

		pAnimation = static_cast<cAnimation*>(this->FindLoadedResource(asNewName,sPath));

		if(pAnimation==NULL && sPath!="")
		{
			cMeshLoaderHandler *pMeshLoadHandler = mpResources->GetMeshLoaderHandler();

			//try to load animation from mesh
			cMesh *pTempMesh = pMeshLoadHandler->LoadMesh(sPath,0);
			if(pTempMesh==NULL){
				Error("Couldn't load animation from '%s'\n",sPath.c_str());
				EndLoad();
				return NULL;
			}

			if(pTempMesh->GetAnimationNum()<=0)
			{
				Error("No animations found in '%s'\n",sPath.c_str());
				hplDelete(pTempMesh);
				EndLoad();
				return NULL;
			}

			pAnimation = pTempMesh->GetAnimation(0);
			pTempMesh->ClearAnimations(false);

			hplDelete(pTempMesh);

			AddResource(pAnimation);
		}

		if(pAnimation) pAnimation->IncUserCount();
		else Error("Couldn't create animation '%s'\n",asNewName.c_str());

		EndLoad();
		return pAnimation;
	}

	//-----------------------------------------------------------------------

	iResourceBase* cAnimationManager::Create(const tString& asName)
	{
		return CreateAnimation(asName);
	}

	//-----------------------------------------------------------------------

	void cAnimationManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			hplDelete(apResource);
		}
	}

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------


	//-----------------------------------------------------------------------
}
