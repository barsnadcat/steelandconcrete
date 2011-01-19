/*-------------------------------------------------------------------------
This source file is a part of OGRE
(Object-oriented Graphics Rendering Engine)

For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2006 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License (LGPL) as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA or go to
http://www.gnu.org/copyleft/lesser.txt
-------------------------------------------------------------------------*/
#include "MaterialEditorApp.h"

#include <wx/splash.h>

#include "OgreConfigFile.h"
#include "OgreLog.h"
#include "OgreLogManager.h"
#include "OgreResourceManager.h"

#include "IconManager.h"
#include "MaterialEditorFrame.h"
#include "Workspace.h"



IMPLEMENT_APP(MaterialEditorApp)

BEGIN_EVENT_TABLE(MaterialEditorApp, wxOgreApp)
EVT_KEY_DOWN(MaterialEditorApp::OnKeyUp)
END_EVENT_TABLE()

MaterialEditorApp::~MaterialEditorApp()
{
}

void MaterialEditorApp::OnKeyUp(wxKeyEvent& event)
{
	Ogre::LogManager::getSingleton().getDefaultLog()->stream() << char(event.GetKeyCode());
	event.Skip();
}

bool MaterialEditorApp::OnInit()
{
	try
	{
		// Initialize Ogre render system
		m_rsys->LoadPlugin("RenderSystem_GL");
		m_rsys->SelectOgreRenderSystem("OpenGL Rendering Subsystem");
		m_rsys->Initialise();
		Ogre::ArchiveManager::getSingleton().addArchiveFactory(&mFileArchiveFactory);

		wxInitAllImageHandlers();

		// Ensure Workspace is created
		new Workspace();

		// Create the IconManager
		new IconManager();

		MaterialEditorFrame* frame = new MaterialEditorFrame();

		frame->SetIcon(wxIcon(ogre_xpm));
		frame->Show(true);

		SetTopWindow(frame);

		frame->CreateScene();

		return true;
	}
	catch (std::exception& e)
	{
		wxLogError(wxString(e.what(), wxConvUTF8));
		return false;
	}
	catch (...)
	{
		wxLogError(wxT("Exception!"));
		return false;
	}
}

int MaterialEditorApp::OnExit()
{
	// Minimally clean up the IconManager
	delete IconManager::getSingletonPtr();
	return 0;
}
