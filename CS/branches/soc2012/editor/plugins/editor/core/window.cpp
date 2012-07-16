/*
    Copyright (C) 2011 by Jelle Hellemans

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
#include "csutil/objreg.h"
#include "csutil/scf.h"
#include "ieditor/editor.h"

#include "spacemanager.h"
#include "window.h"

#include <wx/artprov.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/sizer.h>

CS_PLUGIN_NAMESPACE_BEGIN (CSEditor)
{
  
BEGIN_EVENT_TABLE (Window, wxSplitterWindow)
  EVT_SPLITTER_DCLICK (wxID_ANY, Window::OnDClick)
  EVT_SPLITTER_UNSPLIT (wxID_ANY, Window::OnUnsplitEvent)
  EVT_SIZE (Window::OnSize)
END_EVENT_TABLE ()

Window::Window (iObjectRegistry* obj_reg, iEditor* editor, wxWindow* parent,
		bool hor)
  : wxSplitterWindow (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		      wxSP_3D | wxSP_LIVE_UPDATE | wxCLIP_CHILDREN), 
  object_reg (obj_reg), editor (editor), horizontal (hor)
{
  ViewControl* control = new ViewControl (object_reg, editor, this);
  Initialize (control);
}

Window::Window (iObjectRegistry* obj_reg, iEditor* editor, wxWindow* parent,
		ViewControl* control, bool hor)
  : wxSplitterWindow (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		      wxSP_3D | wxSP_LIVE_UPDATE | wxCLIP_CHILDREN), 
  object_reg (obj_reg), editor (editor), horizontal (hor)
{
  control->Reparent (this);
  Initialize (control);
}

Window::~Window ()
{
}

bool Window::Split ()
{
  if (IsSplit ()) return false;
  ViewControl* c = (ViewControl*) GetWindow1 ();
  Window* w1 = new Window (object_reg, editor, this, c, !horizontal);
  Window* w2 = new Window (object_reg, editor, this, !horizontal);
  
  if (horizontal) SplitHorizontally (w1, w2);
  else SplitVertically (w1, w2);

  return true;
}

void Window::OnDClick (wxSplitterEvent& event)
{
  event.Veto ();
  wxWindow* w1 = GetWindow1 ();
  wxWindow* w2 = GetWindow2 ();
  Unsplit ();
  if (GetSplitMode () == wxSPLIT_VERTICAL)
    SplitHorizontally (w1, w2);
  else
    SplitVertically (w1, w2);
}

void Window::OnUnsplitEvent (wxSplitterEvent& event)
{
  wxWindow* w = event.GetWindowBeingRemoved ();
  if (w) w->Destroy ();
}

void Window::OnSize (wxSizeEvent& event)
{
  //SetSize (event.GetSize ());
  Layout ();
  event.Skip ();
}

// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE (ViewControl, wxPanel)
  EVT_SIZE (ViewControl::OnSize)
END_EVENT_TABLE ()

ViewControl::ViewControl (iObjectRegistry* obj_reg, iEditor* editor, wxWindow* parent)
: wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), object_reg (obj_reg),
  editor (editor)
{
  box = new wxBoxSizer (wxVERTICAL);
  
  wxPanel* menuBar = new wxPanel (this, wxID_ANY, wxDefaultPosition, wxSize (-1, -1));
  wxBoxSizer* bar = new wxBoxSizer (wxHORIZONTAL);
  wxToolBar* tb = new wxToolBar (menuBar, wxID_ANY);
      
  // Create the space combo box
  SpaceComboBox* m_combobox = new SpaceComboBox (obj_reg, editor, tb, this);
  tb->AddControl (m_combobox);

  if (space && space->GetwxWindow ())
    box->Add (space->GetwxWindow (), 1, wxEXPAND | wxALL, 0);
      
  tb->Realize ();
  bar->Add (tb, 0, /*wxEXPAND |*/ wxALIGN_LEFT, 0);

  toolbar = new wxPanel (menuBar, wxID_ANY);
  bar->Add (toolbar, 1, wxEXPAND | wxALL, 0);

  {
    wxToolBar* tb = new wxToolBar (menuBar, wxID_ANY);
    tb->AddTool (1, wxT ("Split"), wxArtProvider::GetBitmap (wxART_ADD_BOOKMARK, wxART_TOOLBAR, wxSize (16, 16)));
    tb->Connect (1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler (ViewControl::OnClicked), 0, this);
    tb->AddTool (2, wxT ("Duplicate"), wxArtProvider::GetBitmap (wxART_GO_TO_PARENT, wxART_TOOLBAR, wxSize (16, 16)));
    tb->Connect (2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler (ViewControl::OnClicked), 0, this);
/*
    tb->AddSeparator ();
    tb->AddTool (2, wxT ("Test"), wxArtProvider::GetBitmap (wxART_NEW_DIR, wxART_TOOLBAR, wxSize (16, 16)));
    tb->AddTool (3, wxT ("Test"), wxArtProvider::GetBitmap (wxART_GO_DIR_UP, wxART_TOOLBAR, wxSize (16, 16)));
    tb->AddTool (4, wxT ("Test"), wxArtProvider::GetBitmap (wxART_GO_HOME, wxART_TOOLBAR, wxSize (16, 16)));
    tb->AddTool (5, wxT ("Test"), wxArtProvider::GetBitmap (wxART_FILE_OPEN, wxART_TOOLBAR, wxSize (16, 16)));
*/
    tb->Realize ();
    bar->Add (tb, 0, /*wxEXPAND |*/ wxALIGN_RIGHT, 0);
  }

  menuBar->SetSizer (bar);

  box->Add (menuBar, 0, wxEXPAND);
  SetSizer (box);
  box->SetSizeHints (this);
}

ViewControl::~ViewControl ()
{
}

void ViewControl::OnClicked (wxCommandEvent& event)
{
  if (event.GetId () == 1)
  {
    space->DisableUpdates (true);
    Window* window = (Window*) this->GetParent ();
    window->Split ();
  }
  else
  {
    wxFrame* frame = new wxFrame (this, wxID_ANY, wxT ("3D View"), wxDefaultPosition, GetSize ());
    /*Window* m_splitter =*/ new Window (object_reg, editor, frame);
    frame->Show (true);
  }
}

void ViewControl::OnSize (wxSizeEvent& event)
{
  //SetSize (event.GetSize ());
  Layout ();
  event.Skip ();
}

// ----------------------------------------------------------------------------

SpaceComboBox::SpaceComboBox
  (iObjectRegistry* obj_reg, iEditor* editor, wxWindow* parent, ViewControl* ctrl)
  : wxBitmapComboBox (parent, wxID_ANY, wxEmptyString,wxDefaultPosition,
		      wxSize (50, 20),0, NULL, wxCB_READONLY),
    object_reg (obj_reg), editor (editor), control (ctrl)
{
  // Build the list of menu entries for all spaces
  iSpaceManager* imgr = editor->GetSpaceManager ();
  SpaceManager* mgr = static_cast<SpaceManager*> (imgr);
  csRefArray<SpaceFactory>::ConstIterator spaces =
    mgr->GetSpaceFactories ().GetIterator ();

  size_t i = 0;
  bool instanced = false;
  while (spaces.HasNext ())
  {
    i++;

    // Add the menu entry for this space
    iSpaceFactory* f = spaces.Next ();
    wxString label (f->GetLabel (), wxConvUTF8);
    Append (label, f->GetIcon ());

    // Create the default space if not yet made
    if (instanced) continue;

    if (f->GetMultipleAllowed () || f->GetCount () == 0)
    {
      ctrl->space = f->CreateInstance (control);
      SetSelection (i-1);
      instanced = true;
    }
  }

  // TODO: create a default space if not yet made

  // Listen to the OnSelected event
  Connect (GetId (), wxEVT_COMMAND_COMBOBOX_SELECTED,
	   wxCommandEventHandler (SpaceComboBox::OnSelected), 0, this);  
}

SpaceComboBox::~SpaceComboBox ()
{
}

void SpaceComboBox::OnSelected (wxCommandEvent& event)
{
  // Search the space that is being selected
  iSpaceManager* imgr = editor->GetSpaceManager ();
  SpaceManager* mgr = static_cast<SpaceManager*> (imgr);

  csRefArray<SpaceFactory>::ConstIterator spaces =
    mgr->GetSpaceFactories ().GetIterator ();

  size_t i = 0;
  while (spaces.HasNext ())
  {
    i++;
    iSpaceFactory* f = spaces.Next ();
    wxString label (f->GetLabel (), wxConvUTF8);

    if (GetValue () == label)
    {
      // Create an instance of the selected space
      if (f->GetMultipleAllowed () || f->GetCount () == 0)
      {
        control->layout.Invalidate ();
        control->box->Detach (control->space->GetwxWindow ());
        control->space = f->CreateInstance (control);
        control->box->Insert (0, control->space->GetwxWindow (), 1, wxEXPAND, 0);
        SetSelection (i - 1);
        mgr->ReDraw (control->space);
	control->space->GetwxWindow ()->Layout ();
        editor->GetwxFrame ()->Layout ();
      }

      else
      {
	// TODO: put back the previous selection
      }

      break;
    }
  }
}

}
CS_PLUGIN_NAMESPACE_END (CSEditor)
