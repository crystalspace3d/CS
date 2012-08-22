/*
    Copyright (C) 2011-2012 by Jelle Hellemans, Christian Van Brussel
    Copyright (C) 2007 by Seth Yastrov

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "cssysdef.h"
#include "cstool/initapp.h"
#include "csutil/scf.h"
#include "csutil/stringquote.h"
#include "iutil/virtclk.h"

#include "actionmanager.h"
#include "editor.h"
#include "menubar.h"
#include "operatormanager.h"
#include "perspective.h"
#include "spacemanager.h"
#include "statusbar.h"
#include "window.h"

#include <wx/log.h>

CS_PLUGIN_NAMESPACE_BEGIN (CSEditor)
{

//------------------------------------  EditorManager  ------------------------------------

SCF_IMPLEMENT_FACTORY (EditorManager)

EditorManager::EditorManager (iBase* parent)
  : scfImplementationType (this, parent), pump (nullptr)
{
}

EditorManager::~EditorManager ()
{
  delete pump;

  // The deletion of the iEditor instances is managed automatically
  // by wxWidgets
}

bool EditorManager::ReportError (const char* description, ...) const
{
  va_list arg;
  va_start (arg, description);
  csReportV (object_reg, CS_REPORTER_SEVERITY_ERROR,
	     "crystalspace.editor.core",
	     description, arg);
  va_end (arg);
  return false;
}

bool EditorManager::ReportWarning (const char* description, ...) const
{
  va_list arg;
  va_start (arg, description);
  csReportV (object_reg, CS_REPORTER_SEVERITY_WARNING,
	     "crystalspace.editor.core",
	     description, arg);
  va_end (arg);
  return false;
}

bool EditorManager::Initialize (iObjectRegistry* reg)
{
  object_reg = reg;
  //StartEngine ();

  // Find references to the virtual clock and the main event queue
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  eventQueue = csQueryRegistry<iEventQueue> (object_reg);

  return true;
}

bool EditorManager::StartEngine ()
{
  // Request every standard plugin of Crystal Space
  if (!csInitializer::RequestPlugins (object_reg,
        CS_REQUEST_VFS,
        CS_REQUEST_FONTSERVER,
        CS_REQUEST_REPORTER,
        CS_REQUEST_END))
  {
    wxLogError (wxT ("Can't initialize standard Crystal Space plugins!"));
    return false;
  }

  return true;
}

bool EditorManager::StartApplication ()
{
  // Open the main system. This will open all the previously loaded plug-ins.
  if (!csInitializer::OpenApplication (object_reg))
    return ReportError ("Error opening system!");

  // TODO: remove this hack?
  for (size_t i = 0; i < editors.GetSize (); i++)
    editors[i]->Init ();

  // Initialize the pump (the refresh rate is in millisecond)
  pump = new Pump (this);
  pump->Start (20);

  return true;
}

iEditor* EditorManager::CreateEditor (const char* name, const char* title, iContext* context)
{
  if (!context)
  {
    ReportError ("No context provided when creating an editor instance!");
    return nullptr;
  }

  Editor* editor = new Editor (this, name, title, context);
  editors.Push (editor);

  return editor;
}

void EditorManager::RemoveEditor (iEditor* editor)
{
  editor->GetwxFrame ()->Close (false);
}

iEditor* EditorManager::FindEditor (const char* name)
{
  for (size_t i = 0; i < editors.GetSize (); i++)
    if (editors[i]->name == name)
      return editors[i];
  return nullptr;
}

iEditor* EditorManager::GetEditor (size_t index)
{
  return editors[index];
}

size_t EditorManager::GetEditorCount () const
{
  return editors.GetSize ();
}

void EditorManager::Update ()
{
  // Update the virtual clock and the global event queue
  vc->Advance ();
  eventQueue->Process ();
  //wxYield();

  // Update each editor instance
  for (size_t i = 0; i < editors.GetSize (); i++)
    editors[i]->Update ();
}

//------------------------------------  Editor  ------------------------------------

Editor::Editor (EditorManager* manager, const char* name, const char* title,
		iContext* context)
  // TODO: use size from CS config
  : scfImplementationType (this),
  wxFrame (nullptr, -1, wxString::FromAscii (title), wxDefaultPosition,
	   wxSize (1024, 768)),
    name (name), manager (manager), context (context), statusBar (nullptr)
{
  // Create the main objects and managers
  actionManager.AttachNew (new ActionManager (manager->object_reg, this));
  menuManager.AttachNew (new MenuManager (this));
  operatorManager.AttachNew (new OperatorManager (manager->object_reg, this));
  perspectiveManager.AttachNew (new PerspectiveManager (manager->object_reg, this));
  spaceManager.AttachNew (new SpaceManager (this));

  // Create the status bar
  statusBar = new StatusBar (this);
  SetStatusBar (statusBar);

  //PositionStatusBar ();
  statusBar->Show ();

  // Make this window visible
  Show (true);
}

Editor::~Editor ()
{
  // Remove ourself from the list maintained by the editor manager
  manager->editors.Delete (this);

  delete statusBar;
}

iContext* Editor::GetContext () const
{
  return context;
}

iEditorManager* Editor::GetManager () const
{
  return manager;
}

iActionManager* Editor::GetActionManager () const
{
  return actionManager;
}

iMenuManager* Editor::GetMenuManager () const
{
  return menuManager;
}

iOperatorManager* Editor::GetOperatorManager () const
{
  return operatorManager;
}

iPerspectiveManager* Editor::GetPerspectiveManager () const
{
  return perspectiveManager;
}

iSpaceManager* Editor::GetSpaceManager () const
{
  return spaceManager;
}

csPtr<iProgressMeter> Editor::CreateProgressMeter () const
{
  csRef<iProgressMeter> meter;
  meter.AttachNew (new StatusBarProgressMeter (statusBar));
  return csPtr<iProgressMeter> (meter);
}

void Editor::ReportStatus (const char* text)
{
  SetStatusText (wxString::FromUTF8 (text));
  csReport (manager->object_reg, CS_REPORTER_SEVERITY_NOTIFY,
	    "status", text, nullptr);
}

wxFrame* Editor::GetwxFrame ()
{
  return this;
}

void Editor::Save (iDocumentNode* node) const
{
}

bool Editor::Load (iDocumentNode* node)
{
  return false;
}

void Editor::Init ()
{
  // Setup the current perspective or create a default one
  iPerspective* perspective = perspectiveManager->GetCurrentPerspective ();
  if (!perspective) perspective = perspectiveManager->CreatePerspective ("Default");

  // Create the main splitter window
  Window* window = perspectiveManager->CreateWindow (perspective);

  wxBoxSizer* box = new wxBoxSizer (wxHORIZONTAL);
  box->Add (window, 1, wxEXPAND | wxALL, 0);
  SetSizer (box);

  // Reset the window size
  SetSize (wxSize (1024, 768));
  PositionStatusBar ();

  ReportStatus ("Ready");
}

void Editor::Update ()
{
  spaceManager->Update ();
}

}
CS_PLUGIN_NAMESPACE_END (CSEditor)
