/*
-------------------------------------------------------------------------
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
-------------------------------------------------------------------------
*/
#include "MaterialEditorFrame.h"

#include <wx/bitmap.h>
#include <wx/notebook.h>
#include <wx/treectrl.h>
#include <wx/wxscintilla.h>
#include <wx/wizard.h>
#include <wx/aui/auibook.h>
#include <wx/aui/framemanager.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/advprops.h>
#include <wx/imaglist.h>

#include "OgreCamera.h"
#include "OgreColourValue.h"
#include "OgreConfigFile.h"
#include "OgreMaterial.h"
#include "OgreMaterialManager.h"
#include "OgreRoot.h"
#include "OgreString.h"
#include "OgreStringConverter.h"
#include "OgreVector3.h"

#include "CgEditor.h"
#include "DocPanel.h"
#include "EditorEventArgs.h"
#include "EditorManager.h"
#include "IconManager.h"
#include "LogPanel.h"
#include "MaterialPropertyGridPage.h"
#include "MaterialScriptEditor.h"
#include "PropertiesPanel.h"
#include "TechniquePropertyGridPage.h"
#include "PassPropertyGridPage.h"
#include <Workspace.h>
#include <wx/ogre/ogre.h>

using Ogre::Camera;
using Ogre::ColourValue;
using Ogre::RenderSystemList;
using Ogre::Root;
using Ogre::String;
using Ogre::Vector3;

const long ID_FILE_MENU_OPEN = wxNewId();
const long ID_FILE_MENU_SAVE = wxNewId();
const long ID_FILE_MENU_SAVE_AS = wxNewId();
const long ID_FILE_MENU_EXIT = wxNewId();

const long ID_EDIT_MENU_UNDO = wxNewId();
const long ID_EDIT_MENU_REDO = wxNewId();
const long ID_EDIT_MENU_CUT = wxNewId();
const long ID_EDIT_MENU_COPY = wxNewId();
const long ID_EDIT_MENU_PASTE = wxNewId();

const long ID_RESOURCE_TREE = wxNewId();
const long ID_FILE_TREE = wxNewId();

// Image list
const int WORKSPACE_IMAGE = 0;
const int GROUP_IMAGE = 1;
// Archive types
const int UNKNOWN_ARCHIVE_IMAGE = 2;
const int FILE_SYSTEM_IMAGE = 3;
const int ZIP_IMAGE = 4;
// Resource types
const int UNKNOWN_RESOURCE_IMAGE = 5;
const int COMPOSITE_IMAGE = 6;
const int MATERIAL_IMAGE = 7;
const int MESH_IMAGE = 8;
const int ASM_PROGRAMM_IMAGE = 9;
const int HL_PROGRAMM_IMAGE = 10;
const int TEXTURE_IMAGE = 11;
const int SKELETON_IMAGE = 12;
const int FONT_IMAGE = 13;

Ogre::Log::Stream GetLog()
{
    return  Ogre::LogManager::getSingleton().stream();
}

BEGIN_EVENT_TABLE(MaterialEditorFrame, wxFrame)
    // File Menu
    EVT_MENU (ID_FILE_MENU_OPEN,		 MaterialEditorFrame::OnFileOpen)
    EVT_MENU (ID_FILE_MENU_SAVE,		 MaterialEditorFrame::OnFileSave)
    EVT_MENU (ID_FILE_MENU_SAVE_AS,		 MaterialEditorFrame::OnFileSaveAs)
    EVT_MENU (ID_FILE_MENU_EXIT,		 MaterialEditorFrame::OnFileExit)
    // Edit Menu
    EVT_MENU (ID_EDIT_MENU_UNDO,  MaterialEditorFrame::OnEditUndo)
    EVT_MENU (ID_EDIT_MENU_REDO,  MaterialEditorFrame::OnEditRedo)
    EVT_MENU (ID_EDIT_MENU_CUT,	  MaterialEditorFrame::OnEditCut)
    EVT_MENU (ID_EDIT_MENU_COPY,  MaterialEditorFrame::OnEditCopy)
    EVT_MENU (ID_EDIT_MENU_PASTE, MaterialEditorFrame::OnEditPaste)
    // Resource tree
    EVT_TREE_SEL_CHANGED(ID_FILE_TREE, MaterialEditorFrame::OnFileSelected)
    EVT_TREE_SEL_CHANGED(ID_RESOURCE_TREE, MaterialEditorFrame::OnResourceSelected)
END_EVENT_TABLE()

MaterialEditorFrame::MaterialEditorFrame(wxWindow* parent) :
    wxFrame(parent, - 1, wxT("Ogre Material Editor"), wxDefaultPosition, wxSize(800, 600), wxDEFAULT_FRAME_STYLE),
    mMenuBar(0),
    mFileMenu(0),
    mEditMenu(0),
    mWindowMenu(0),
    mHelpMenu(0),
    mAuiManager(0),
    mAuiNotebook(0),
    mInformationNotebook(0),
    mFileTree(0),
    mPropertiesPanel(0),
    mLogPanel(0),
    mDocPanel(0),
    mOgreControl(0)
{
    createAuiManager();
    createMenuBar();

    CreateToolBar();
    CreateStatusBar();

    /*
    ** We have to create the OgrePanel first
    ** since some of the other panels rely on Ogre.
    */
    createAuiNotebookPane();
    createOgrePane();
    createInformationPane();
    createManagementPane();
    createPropertiesPane();

    mAuiManager->Update();
}

MaterialEditorFrame::~MaterialEditorFrame()
{
    mLogPanel->detachLog(Ogre::LogManager::getSingleton().getDefaultLog());

    if(mAuiManager)
    {
        mAuiManager->UnInit();
        delete mAuiManager;
    }
}

void MaterialEditorFrame::createAuiManager()
{
    mAuiManager = new wxAuiManager();
    mAuiManager->SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_ALLOW_ACTIVE_PANE | wxAUI_MGR_TRANSPARENT_DRAG);
    mAuiManager->SetManagedWindow(this);

    wxAuiDockArt* art = mAuiManager->GetArtProvider();
    art->SetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE, 1);
    art->SetMetric(wxAUI_DOCKART_SASH_SIZE, 4);
    art->SetMetric(wxAUI_DOCKART_CAPTION_SIZE, 17);
    art->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR, wxColour(49, 106, 197));
    art->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR, wxColour(90, 135, 208));
    art->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR, wxColour(255, 255, 255));
    art->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR, wxColour(200, 198, 183));
    art->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR, wxColour(228, 226, 209));
    art->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR, wxColour(0, 0, 0));

    mAuiManager->Update();
}

void MaterialEditorFrame::createAuiNotebookPane()
{
    mAuiNotebook = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxNO_BORDER);

    // Create EditorManager singleton
    new EditorManager(mAuiNotebook);

    mAuiManager->AddPane(mAuiNotebook, wxLEFT);
}

void MaterialEditorFrame::OnResourceSelected(wxTreeEvent& event)
{
    if (event.GetItem() != mScriptTree->GetRootItem())
    {
        if (const MaterialMap* materials = GetMaterialMap(mFileTree->GetSelection()))
        {
            MaterialMap::const_iterator it = materials->find(Ogre::String(mScriptTree->GetItemText(event.GetItem()).mb_str()));
            Ogre::Entity* ent = m_sm->getEntity("Display");
            ent->setMaterial(it->second);
            mOgreControl->Refresh();
            mPropertiesPanel->MaterialSelected(it->second);
        }
    }
}

const MaterialMap* MaterialEditorFrame::GetMaterialMap(const wxTreeItemId& id)
{
    if (mFileTree->GetItemImage(id) == MATERIAL_IMAGE)
    {
        Ogre::String file(mFileTree->GetItemText(id).mb_str());

        const wxTreeItemId archiveId = mFileTree->GetItemParent(id);
        Ogre::String archive(mFileTree->GetItemText(archiveId).mb_str());

        const wxTreeItemId groupId = mFileTree->GetItemParent(archiveId);
        Ogre::String group(mFileTree->GetItemText(groupId).mb_str());

        return &mGroupMap[group][archive][file];
    }
    else
        return NULL;
}


void MaterialEditorFrame::OnFileSelected(wxTreeEvent& event)
{
    mScriptTree->DeleteAllItems();

    if (const MaterialMap* materials = GetMaterialMap(event.GetItem()))
    {
        wxTreeItemId root = mScriptTree->AddRoot(mFileTree->GetItemText(event.GetItem()));
        for (MaterialMap::const_iterator it = materials->begin(); it != materials->end(); ++it)
        {
            const wxTreeItemId id = mScriptTree->AppendItem(root, wxString(it->second->getName().c_str(), wxConvUTF8));
            if (it == materials->begin())
            {
                mScriptTree->SelectItem(id, true);
            }
        }
    }
}

void MaterialEditorFrame::createManagementPane()
{
    {
        mScriptTree = new wxTreeCtrl(this, ID_RESOURCE_TREE, wxDefaultPosition, wxDefaultSize,
                                     wxNO_BORDER | wxTR_EDIT_LABELS | wxTR_FULL_ROW_HIGHLIGHT |
                                     wxTR_HAS_BUTTONS | wxTR_SINGLE);
        mScriptTree->AddRoot(wxT("Heh!"));

        wxAuiPaneInfo info;
        info.Caption(wxT("Script browser"));
        info.MaximizeButton(true);
        info.BestSize(256, 512);
        info.Left();
        info.Layer(1);

        mAuiManager->AddPane(mScriptTree, info);
    }

    {

        mFileTree = new wxTreeCtrl(this, ID_FILE_TREE, wxDefaultPosition, wxDefaultSize,
                                   wxNO_BORDER | wxTR_EDIT_LABELS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_HAS_BUTTONS | wxTR_SINGLE);

        wxImageList* mImageList = new wxImageList(16, 16, true, 13);
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::WORKSPACE));// WORKSPACE_IMAGE = 0;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::PROJECT));// GROUP
        // Archive types
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::UNKNOWN));// UNKNOW_ARCHIVE_IMAGE = 1;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::FILE_SYSTEM));// FILESYTEM_IMAGE = 2;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::ZIP));// ZIP_IMAGE = 3;
        // Resource types
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::UNKNOWN));// UNKNOW_RESOURCE_IMAGE = 4;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::PROJECT));// COMPOSITE_IMAGE = 5;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::MATERIAL));// MATERIAL_IMAGE = 6;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::MESH));// MESH_IMAGE = 7;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::PROGRAM_SCRIPT));// ASM_PROGRAMM_IMAGE = 8;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::PROGRAM_SCRIPT));// HL_PROGRAMM_IMAGE = 9;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::TEXTURE));// TEXTURE_IMAGE = 10;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::TECHNIQUE));// SKELETON_IMAGE = 11;
        mImageList->Add(IconManager::getSingleton().getIcon(IconManager::FONT));// FONT_IMAGE = 12;

        mFileTree->AssignImageList(mImageList);

        wxAuiPaneInfo info;
        info.Caption(wxT("Resource browser"));
        info.MaximizeButton(true);
        info.BestSize(256, 512);
        info.Left();
        info.Layer(1);

        mAuiManager->AddPane(mFileTree, info);
    }


}

void MaterialEditorFrame::createInformationPane()
{
    mInformationNotebook = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS | wxNO_BORDER);

    mLogPanel = new LogPanel(mInformationNotebook);
    mInformationNotebook->AddPage(mLogPanel, wxT("Log"));
    mLogPanel->attachLog(Ogre::LogManager::getSingleton().getDefaultLog());

    mDocPanel = new DocPanel(mInformationNotebook);
    mInformationNotebook->AddPage(mDocPanel, wxT("Documentation"));

    wxAuiPaneInfo info;
    info.Caption(wxT("Information"));
    info.MaximizeButton(true);
    info.BestSize(256, 128);
    info.Bottom();

    mAuiManager->AddPane(mInformationNotebook, info);
}

void MaterialEditorFrame::createPropertiesPane()
{
    mPropertiesPanel = new PropertiesPanel(this);

    wxAuiPaneInfo info;
    info.Caption(wxT("Properties"));
    info.MaximizeButton(true);
    info.BestSize(256, 512);
    info.Right();
    info.Layer(1);

    mAuiManager->AddPane(mPropertiesPanel, info);
}

void MaterialEditorFrame::CreateScene()
{
    m_sm = mOgreControl->CreateSceneManager(Ogre::ST_GENERIC);

    Ogre::Entity* ent = m_sm->createEntity("Display", Ogre::SceneManager::PT_CUBE);
    Ogre::SceneNode* no = m_sm->getRootSceneNode()->createChildSceneNode();

    no->setPosition(0, 0, -200);
    no->attachObject(ent);

    mOgreControl->Refresh();

    struct stat stFileInfo;
    const char* resources = "resources.cfg";
    if (!stat(resources, &stFileInfo))
    {
        Workspace::OpenConfigFile(resources);
        FillResourceTree();
    }
}

void MaterialEditorFrame::createOgrePane()
{

    mOgreControl = new wxOgreControl(this, wxID_ANY, wxDefaultPosition, this->GetClientSize());

    mAuiManager->AddPane(mOgreControl, wxCENTER);
}

void MaterialEditorFrame::createMenuBar()
{
    mMenuBar = new wxMenuBar();

    createFileMenu();
    createEditMenu();
    createWindowMenu();
    createHelpMenu();

    SetMenuBar(mMenuBar);
}

void MaterialEditorFrame::createFileMenu()
{
    mFileMenu = new wxMenu();
    wxMenuItem* menuItem = NULL;

    // New sub menu
    menuItem = new wxMenuItem(mFileMenu, ID_FILE_MENU_OPEN, wxT("&Open"));
    mFileMenu->Append(menuItem);

    menuItem = new wxMenuItem(mFileMenu, ID_FILE_MENU_SAVE, wxT("&Save"));
    menuItem->SetBitmap(IconManager::getSingleton().getIcon(IconManager::SAVE));
    mFileMenu->Append(menuItem);

    menuItem = new wxMenuItem(mFileMenu, ID_FILE_MENU_SAVE_AS, wxT("Save &As..."));
    menuItem->SetBitmap(IconManager::getSingleton().getIcon(IconManager::SAVE_AS));
    mFileMenu->Append(menuItem);

    mFileMenu->AppendSeparator();

    menuItem = new wxMenuItem(mFileMenu, ID_FILE_MENU_EXIT, wxT("E&xit"));
    mFileMenu->Append(menuItem);
    mFileMenu->UpdateUI();

    mMenuBar->Append(mFileMenu, wxT("&File"));
}

void MaterialEditorFrame::createEditMenu()
{
    mEditMenu = new wxMenu();
    mEditMenu->Append(ID_EDIT_MENU_UNDO, wxT("Undo"));
    mEditMenu->Append(ID_EDIT_MENU_REDO, wxT("Redo"));
    mEditMenu->AppendSeparator();
    mEditMenu->Append(ID_EDIT_MENU_CUT, wxT("Cut"));
    mEditMenu->Append(ID_EDIT_MENU_COPY, wxT("Copy"));
    mEditMenu->Append(ID_EDIT_MENU_PASTE, wxT("Paste"));

    mMenuBar->Append(mEditMenu, wxT("&Edit"));
}

void MaterialEditorFrame::createWindowMenu()
{
    mWindowMenu = new wxMenu();
    mMenuBar->Append(mWindowMenu, wxT("&Window"));
}

void MaterialEditorFrame::createHelpMenu()
{
    mHelpMenu = new wxMenu();
    mMenuBar->Append(mHelpMenu, wxT("&Help"));
}

void MaterialEditorFrame::FillResourceTree()
{
    mFileTree->DeleteAllItems();
    wxTreeItemId mRootId = mFileTree->AddRoot(wxString(Workspace::GetFileName().c_str(), wxConvUTF8), WORKSPACE_IMAGE);
    Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();
    Ogre::StringVector groups = rgm.getResourceGroups();
    for (Ogre::StringVector::iterator groupIt = groups.begin(); groupIt != groups.end(); ++groupIt)
    {
        wxTreeItemId groupId = mFileTree->AppendItem(mRootId, wxString(groupIt->c_str(), wxConvUTF8), GROUP_IMAGE);
        mFileTree->SelectItem(groupId, true); //This is some kind of hack for windows
        Ogre::FileInfoListPtr fileInfoList = rgm.listResourceFileInfo(*groupIt, false);

        // Collect archives. In Ogre 1.7, this will be removed, and archives queried directly
        std::set< Ogre::Archive* > archives;
        for (Ogre::FileInfoList::iterator fileIt = fileInfoList->begin(); fileIt != fileInfoList->end(); ++fileIt)
        {
            archives.insert(fileIt->archive);
        }

        for (std::set< Ogre::Archive* >::iterator archiveIt = archives.begin(); archiveIt != archives.end(); ++archiveIt)
        {
            Ogre::Archive* archive = *archiveIt;
            wxTreeItemId archiveId = mFileTree->AppendItem(groupId, wxString(archive->getName().c_str(), wxConvUTF8), FILE_SYSTEM_IMAGE);

            Ogre::StringVectorPtr fileList;
            // Materials
            fileList = archive->find("*.material");
            for (Ogre::StringVector::iterator fileNameIt = fileList->begin(); fileNameIt != fileList->end(); ++fileNameIt)
            {
                wxTreeItemId id = mFileTree->AppendItem(archiveId, wxString(fileNameIt->c_str(), wxConvUTF8), MATERIAL_IMAGE);
            }

            // Meshes
            fileList = archive->find("*.mesh");
            for (Ogre::StringVector::iterator fileNameIt = fileList->begin(); fileNameIt != fileList->end(); ++fileNameIt)
            {
                mFileTree->AppendItem(archiveId, wxString(fileNameIt->c_str(), wxConvUTF8), MESH_IMAGE);
            }

            // Programs
            fileList = archive->find("*.program");
            for (Ogre::StringVector::iterator fileNameIt = fileList->begin(); fileNameIt != fileList->end(); ++fileNameIt)
            {
                mFileTree->AppendItem(archiveId, wxString(fileNameIt->c_str(), wxConvUTF8), HL_PROGRAMM_IMAGE);
            }
        }
    }

    // Now get materials
    Ogre::ResourceManager::ResourceMapIterator it = Ogre::MaterialManager::getSingleton().getResourceIterator();
    while (it.hasMoreElements())
    {
        Ogre::MaterialPtr material = it.getNext();
        const Ogre::String& origin = material->getOrigin();
        if (!origin.empty())
        {
            const Ogre::String& group = material->getGroup();
            const size_t pos = origin.rfind(":");
            Ogre::String archive = origin.substr(0, pos);

            Ogre::String file = origin.substr(pos + 1, origin.size() - pos);
            GetLog() << group + ":" + archive + ":" + file + ":" + material->getName();
            mGroupMap[group][archive][file][material->getName()] = material;
        }
    }

}

void MaterialEditorFrame::OnFileOpen(wxCommandEvent& event)
{
    wxFileDialog * openDialog = new wxFileDialog(this, wxT("Choose a file to open"), wxEmptyString, wxEmptyString,
            wxT("Resource configuration (*.cfg)|*.cfg|All Files (*.*)|*.*"));

    if(openDialog->ShowModal() == wxID_OK)
    {
        wxString path = openDialog->GetPath();
        Workspace::OpenConfigFile(Ogre::String(path.mb_str()));
        FillResourceTree();
    }
}

void MaterialEditorFrame::OnFileSave(wxCommandEvent& event)
{
    EditorBase* editor = EditorManager::getSingletonPtr()->getActiveEditor();
    if(editor != NULL) editor->save();

    // TODO: Support project & workspace save
}

void MaterialEditorFrame::OnFileSaveAs(wxCommandEvent& event)
{
    EditorBase* editor = EditorManager::getSingletonPtr()->getActiveEditor();
    if(editor != NULL) editor->saveAs();

    // TODO: Support project & workspace saveAs
}

void MaterialEditorFrame::OnFileExit(wxCommandEvent& event)
{
    Close();
}

void MaterialEditorFrame::OnEditUndo(wxCommandEvent& event)
{
    EditorBase* editor = EditorManager::getSingletonPtr()->getActiveEditor();
    if(editor != NULL) editor->undo();
}

void MaterialEditorFrame::OnEditRedo(wxCommandEvent& event)
{
    EditorBase* editor = EditorManager::getSingletonPtr()->getActiveEditor();
    if(editor != NULL) editor->redo();
}

void MaterialEditorFrame::OnEditCut(wxCommandEvent& event)
{
    EditorBase* editor = EditorManager::getSingletonPtr()->getActiveEditor();
    if(editor != NULL) editor->cut();
}

void MaterialEditorFrame::OnEditCopy(wxCommandEvent& event)
{
    EditorBase* editor = EditorManager::getSingletonPtr()->getActiveEditor();
    if(editor != NULL) editor->copy();
}

void MaterialEditorFrame::OnEditPaste(wxCommandEvent& event)
{
    EditorBase* editor = EditorManager::getSingletonPtr()->getActiveEditor();
    if(editor != NULL) editor->paste();
}
