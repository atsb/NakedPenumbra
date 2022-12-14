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
#include "scene/Scene.h"
#include "game/Updater.h"
#include "system/LowLevelSystem.h"

#include "graphics/Graphics.h"
#include "resources/Resources.h"
#include "sound/Sound.h"
#include "sound/LowLevelSound.h"
#include "sound/SoundHandler.h"
#include "scene/Camera3D.h"
#include "scene/World3D.h"
#include "graphics/Renderer3D.h"
#include "graphics/RendererPostEffects.h"
#include "graphics/GraphicsDrawer.h"
#include "graphics/RenderList.h"
#include "resources/ScriptManager.h"

#include "physics/Physics.h"

#include "resources/FileSearcher.h"
#include "resources/MeshLoaderHandler.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cScene::cScene(cGraphics *apGraphics,cResources *apResources, cSound* apSound,cPhysics *apPhysics,
					cSystem *apSystem, cAI *apAI, cHaptic *apHaptic)
		: iUpdateable("HPL_Scene")
	{
		mpGraphics = apGraphics;
		mpResources = apResources;
		mpSound = apSound;
		mpPhysics = apPhysics;
		mpSystem = apSystem;
		mpAI = apAI;
		mpHaptic = apHaptic;

		mpCurrentWorld3D = NULL;

		mbCameraIsListener = true;

		mbDrawScene = true;
		mbUpdateMap = true;

		mpActiveCamera = NULL;
	}

	//-----------------------------------------------------------------------

	cScene::~cScene()
	{
		Log("Exiting Scene Module\n");
		Log("--------------------------------------------------------\n");

		STLDeleteAll(mlstWorld3D);
		STLDeleteAll(mlstCamera);

		Log("--------------------------------------------------------\n\n");

	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cCamera3D* cScene::CreateCamera3D(eCameraMoveMode aMoveMode)
	{
		cCamera3D *pCamera = hplNew( cCamera3D, () );
		pCamera->SetAspect(mpGraphics->GetLowLevel()->GetScreenSize().x /
							mpGraphics->GetLowLevel()->GetScreenSize().y);

		//Add Camera to list
		mlstCamera.push_back(pCamera);

		return pCamera;
	}


	//-----------------------------------------------------------------------

	void cScene::DestroyCamera(iCamera* apCam)
	{
		for(tCameraListIt it=mlstCamera.begin(); it!=mlstCamera.end();it++)
		{
			if(*it == apCam){
				hplDelete(*it);
				mlstCamera.erase(it);
				break;
			}
		}
	}

	//-----------------------------------------------------------------------
	void cScene::SetCamera(iCamera* pCam)
	{
		mpActiveCamera = pCam;
	}

	//-----------------------------------------------------------------------

	cScriptVar* cScene::CreateLocalVar(const tString& asName)
	{
		cScriptVar* pVar;
		pVar= GetLocalVar(asName);
		if(pVar==NULL)
		{
			cScriptVar Var;
			Var.msName = asName;
			m_mapLocalVars.insert(tScriptVarMap::value_type(cString::ToLowerCase(asName),Var));
			pVar= GetLocalVar(asName);
			if(pVar==NULL)FatalError("Very strange error when creating script var!\n");
		}
		return pVar;
	}

	//-----------------------------------------------------------------------

	cScriptVar* cScene::GetLocalVar(const tString& asName)
	{
		tScriptVarMapIt it = m_mapLocalVars.find(cString::ToLowerCase(asName));
		if(it==m_mapLocalVars.end()) return NULL;

		return &it->second;
	}

	//-----------------------------------------------------------------------

	tScriptVarMap* cScene::GetLocalVarMap()
	{
		return &m_mapLocalVars;
	}

	//-----------------------------------------------------------------------

	cScriptVar* cScene::CreateGlobalVar(const tString& asName)
	{
		cScriptVar* pVar;
		pVar= GetGlobalVar(asName);
		if(pVar==NULL)
		{
			cScriptVar Var;
			Var.msName = asName;
			m_mapGlobalVars.insert(tScriptVarMap::value_type(cString::ToLowerCase(asName),Var));
			pVar= GetGlobalVar(asName);
			if(pVar==NULL)FatalError("Very strange error when creating script var!\n");
		}
		return pVar;
	}

	//-----------------------------------------------------------------------

	cScriptVar* cScene::GetGlobalVar(const tString& asName)
	{
		tScriptVarMapIt it = m_mapGlobalVars.find(cString::ToLowerCase(asName));
		if(it==m_mapGlobalVars.end()) return NULL;

		return &it->second;
	}

	//-----------------------------------------------------------------------

	tScriptVarMap* cScene::GetGlobalVarMap()
	{
		return &m_mapGlobalVars;
	}

	//-----------------------------------------------------------------------

	void cScene::UpdateRenderList(float afFrameTime)
	{
		if(mbDrawScene && mpActiveCamera)
		{
			cCamera3D* pCamera3D = static_cast<cCamera3D*>(mpActiveCamera);

			if(mpCurrentWorld3D)
				mpGraphics->GetRenderer3D()->UpdateRenderList(mpCurrentWorld3D, pCamera3D,afFrameTime);
		}
	}

	//-----------------------------------------------------------------------

	void cScene::SetDrawScene(bool abX)
	{
		mbDrawScene = abX;
		mpGraphics->GetRenderer3D()->GetRenderList()->Clear();
	}

	//-----------------------------------------------------------------------

	void cScene::Render(cUpdater* apUpdater, float afFrameTime)
	{
		if(mbDrawScene && mpActiveCamera)
		{
			cCamera3D* pCamera3D = static_cast<cCamera3D*>(mpActiveCamera);

			if(mpCurrentWorld3D)
			{
				START_TIMING(RenderWorld)
				mpGraphics->GetRenderer3D()->RenderWorld(mpCurrentWorld3D, pCamera3D,afFrameTime);
				STOP_TIMING(RenderWorld)
			}

			START_TIMING(PostSceneDraw)
			apUpdater->OnPostSceneDraw();
			STOP_TIMING(PostSceneDraw)

			START_TIMING(PostEffects)
			mpGraphics->GetRendererPostEffects()->Render();
			STOP_TIMING(PostEffects)
		}
		else
		{
			apUpdater->OnPostSceneDraw();
			//S
			//mpGraphics->GetLowLevel()->SetClearColor(cColor(0,1));
			//mpGraphics->GetLowLevel()->ClearScreen();
		}
		mpGraphics->GetDrawer()->DrawAll();

		apUpdater->OnPostGUIDraw();
	}

	//-----------------------------------------------------------------------

	void cScene::Update(float afTimeStep)
	{
		if(mpActiveCamera==NULL)return;

		if(mbCameraIsListener)
		{
			cCamera3D* pCamera3D = static_cast<cCamera3D*>(mpActiveCamera);
			mpSound->GetLowLevel()->SetListenerAttributes(
					pCamera3D->GetPosition(),
					cVector3f(0,0,0),
					pCamera3D->GetForward()*-1.0f,
					pCamera3D->GetUp());
		}

		if(mbUpdateMap && mpCurrentWorld3D)
		{
			mpCurrentWorld3D->Update(afTimeStep);

			if(mpCurrentWorld3D->GetScript())
			{
				mpCurrentWorld3D->GetScript()->Run("OnUpdate()");
			}
		}
	}

	//-----------------------------------------------------------------------

	void cScene::Reset()
	{
		m_mapLocalVars.clear();
		m_mapGlobalVars.clear();
		m_setLoadedMaps.clear();
	}

	//-----------------------------------------------------------------------

	cWorld3D* cScene::LoadWorld3D(const tString& asFile, bool abLoadScript, tWorldLoadFlag aFlags)
	{
		//Clear the local script
		m_mapLocalVars.clear();

		///////////////////////////////////
		// Load the map file
		tString asPath = mpResources->GetFileSearcher()->GetFilePath(asFile);
		if(asPath == ""){
			Error("World '%s' doesn't exist\n",asFile.c_str());
			return NULL;
		}

		cWorld3D* pWorld = mpResources->GetMeshLoaderHandler()->LoadWorld(asPath, aFlags);
		if(pWorld==NULL){
			Error("Couldn't load world from '%s'\n",asPath.c_str());
			return NULL;
		}

		////////////////////////////////////////////////////////////
		//Load the script
		cScriptModule* pScript = NULL;
		if(abLoadScript)
		{
			tString sScriptFile = cString::SetFileExt(asFile,"hps");
			pScript = mpResources->GetScriptManager()->CreateScript(sScriptFile);
			if(pScript==NULL){
				Error("Couldn't load script '%s'\n",sScriptFile.c_str());
			}
			else
			{
				pWorld->SetScript(pScript);
			}
		}

		SetWorld3D(pWorld);

		////////////////////////////
		//Add to loaded maps
		tString sName = cString::ToLowerCase(cString::SetFileExt(asFile,""));
		tStringSetIt it = m_setLoadedMaps.find(sName);
		if(it == m_setLoadedMaps.end())
		{
			m_setLoadedMaps.insert(sName);
		}

		////////////////////////////////////////////////////////////
		//Run script start functions
		/*if(pScript)
		{
			//Check if the map has been loaded before, if not run OnStart script.
			tString sName = cString::ToLowerCase(cString::SetFileExt(asFile,""));
			tStringSetIt it = m_setLoadedMaps.find(sName);
			if(it == m_setLoadedMaps.end())
			{
				m_setLoadedMaps.insert(sName);
				pScript->Run("OnStart()");
			}
		}*/

		return pWorld;
	}

	//-----------------------------------------------------------------------

	cWorld3D* cScene::CreateWorld3D(const tString& asName)
	{
		cWorld3D* pWorld = hplNew( cWorld3D, (asName,mpGraphics,mpResources,mpSound,mpPhysics,this,
										mpSystem,mpAI,mpHaptic) );

		mlstWorld3D.push_back(pWorld);

		return pWorld;
	}

	//-----------------------------------------------------------------------

	void cScene::DestroyWorld3D(cWorld3D* apWorld)
	{
		STLFindAndDelete(mlstWorld3D,apWorld);
	}

	//-----------------------------------------------------------------------

	void cScene::SetWorld3D(cWorld3D* apWorld)
	{
		mpCurrentWorld3D = apWorld;

		//Set the world the sound handler uses.
		mpSound->GetSoundHandler()->SetWorld3D(mpCurrentWorld3D);
		//Set the world for physics.
		mpPhysics->SetGameWorld(mpCurrentWorld3D);

	}

	//-----------------------------------------------------------------------

	bool cScene::HasLoadedWorld(const tString &asFile)
	{
		tString sName = cString::ToLowerCase(cString::SetFileExt(asFile,""));
		tStringSetIt it = m_setLoadedMaps.find(sName);

		if(it == m_setLoadedMaps.end())
			return false;
		else
			return true;
	}

	//-----------------------------------------------------------------------
}
