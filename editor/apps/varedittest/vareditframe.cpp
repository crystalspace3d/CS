/*
  Copyright (C) 2012 Christian Van Brussel, Andrei Bârsan

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "cssysdef.h"
#include "vareditframe.h"

//--------------------------
// Main propertygrid header.
#include "cseditor/wx/propgrid/propgrid.h"

// Needed for implementing custom properties.
#include "cseditor/wx/propgrid/propdev.h"

// Extra property classes.
#include "cseditor/wx/propgrid/advprops.h"
// This defines wxPropertyGridManager.
#include "cseditor/wx/propgrid/manager.h"
#include "cseditor/wx/propgrid/editors.h"
//--------------------
#include <wx/panel.h>
#include <wx/toolbar.h>
#include <wx/button.h>
#include <wx/arrstr.h>
#include <wx/combobox.h>
#include <wx/string.h>
#include <wx/wx.h>
#include <wx/variant.h>
#include <wx/event.h>
#include <wx/colour.h>
//*********************************
#include <csutil/cscolor.h>
#include <csgeom/vector3.h>
#include <ivaria/translator.h>
#include "pump.h"

//---------
#include <iostream>
using namespace std;
//----------------


#if wxUSE_DATEPICKCTRL
    #include "wx/datectrl.h"
#endif

#include "wx/artprov.h"
//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo (wxbuildinfoformat format)
{
    wxString wxbuild (wxVERSION_STRING);

    if (format == long_f )
    {
#if defined (__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined (__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined (__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}

//---------------------------------------------------

BEGIN_EVENT_TABLE (ModifiableTestFrame, wxFrame)
    EVT_CLOSE (ModifiableTestFrame::OnClose)
    EVT_MENU (idMenuQuit, ModifiableTestFrame::OnQuit)
    EVT_BUTTON (idBtnTest1, ModifiableTestFrame::OnPopulateClick)
    //EVT_BUTTON(idBtnTest2, ModifiableTestFrame::OnConfirmKeyPress)
    EVT_MENU (idMenuAbout, ModifiableTestFrame::OnAbout)
    EVT_KEY_DOWN (ModifiableTestFrame::OnEsc)
    EVT_KEY_UP (ModifiableTestFrame::OnEsc)
END_EVENT_TABLE ()

// Global frame pointer
csWeakRef<ModifiableTestFrame> frame;

//-------------------------------------------------------------------

ModifiableTestFrame::ModifiableTestFrame ( iObjectRegistry* object_reg )
: wxFrame (NULL, idFrame, wxT ("iModifiable test playground"), wxDefaultPosition, wxSize (600,600)),
  object_reg(object_reg)
{
  mainsizer = new wxBoxSizer (wxHORIZONTAL);
  
  left_vsizer = new wxStaticBoxSizer (wxHORIZONTAL,this,_T("Nodes"));
  right_vsizer = new wxStaticBoxSizer (wxHORIZONTAL,this,_T(""));

  mainsizer->Add (left_vsizer,1,wxALL|wxEXPAND,5);

  modifiableEntities = new csRefArray<iModifiable>();
  focusedIndex = 0;

  frame = this;

  // Initialize the pump driving the CS loop
  Pump* p = new Pump();
  p->s = this;
  p->Start(20);
  
  //-------------------------------------------

#if wxUSE_MENUS
  // create a menu bar
  wxMenuBar* mbar = new wxMenuBar();
  wxMenu* fileMenu = new wxMenu (_T(""));
  fileMenu->Append(idMenuQuit, _ ("&Quit\tAlt-F4"), _("Quit the application"));

  mbar->Append(fileMenu, _("&File"));
  wxMenu* helpMenu = new wxMenu(_T(""));
  helpMenu->Append(idMenuAbout, _("&Helptest\tF1"), _("Show info about this application"));
  mbar->Append(helpMenu, _("&Help"));

	

  SetMenuBar(mbar);
#endif // wxUSE_MENUS
  //------------------------------------------

#if wxUSE_STATUSBAR
  // create a status bar with some information about the used wxWidgets version
  CreateStatusBar ();
  SetStatusText (wxEmptyString);

#endif // wxUSE_STATUSBAR
  //------------------------------------------

  btnCycle = new wxButton (this, idBtnTest1, wxT ("Cycle through nodes"), wxDefaultPosition, wxDefaultSize, 0);
  btnSave = new wxButton (this, idBtnTest2, wxT ("Save"), wxDefaultPosition, wxDefaultSize, 0);

  left_vsizer->Add (btnCycle);
  left_vsizer->Add (btnSave);
  SetSizer (mainsizer);
 
  modifiableEditor = new ModifiableEditor(object_reg,
                                          this,
                                          wxID_ANY,
                                          wxDefaultPosition,
                                          wxDefaultSize,
                                          0L,
                                          wxT("Modifiable editor") );
  mainsizer->Add(modifiableEditor, 1, wxEXPAND | wxALL, 10);
}

bool ModifiableTestFrame::Initialize() {
  if (!csInitializer::SetupEventHandler (object_reg, GeneralEventHandler))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.varedittest",
      "Can't initialize event handler!");

    return false;
  }

  return true;
}

/* static */ bool ModifiableTestFrame :: GeneralEventHandler (iEvent& event) {
  if(frame)
    return frame->TestModifiableEvents(event);

  return true;
}

bool ModifiableTestFrame :: TestModifiableEvents(iEvent& event) {
  
  csRef<iEventNameRegistry> strings = csQueryRegistry<iEventNameRegistry>( object_reg );

  // Ignore the frame event
  if(event.GetName() == strings->GetID("crystalspace.frame"))
    return true;

  printf("\tCaught event: %s (ID #%u)\n",
	 strings->GetString( event.GetName() ),
	 (unsigned int) event.GetName() );
  return true;
}

void ModifiableTestFrame::AddModifiable(iModifiable* modifiable) {
  this->modifiableEntities->Push(modifiable);
}

void ModifiableTestFrame::PushFrame() {
  // No point broadcasting our events if the queue doesn't get to process them
  csRef<iEventQueue> eq(csQueryRegistry<iEventQueue>( object_reg ));
  eq->Process();
}

void ModifiableTestFrame::Populate (iModifiable* dataSource)
{
  modifiableEditor->SetModifiable(dataSource);
}

//---------------------------------------------

void ModifiableTestFrame::OnPopulateClick (wxCommandEvent &event )
{
  // Cycles through the available nodes
  focusedIndex++;
  if(focusedIndex >= modifiableEntities->GetSize()) 
    focusedIndex = 0;

  Populate (modifiableEntities->Get(focusedIndex));
}
//-----------------------------------------------

void ModifiableTestFrame::OnEsc (wxKeyEvent& event)
{
  if ( event.GetKeyCode () == WXK_ESCAPE)
  {
    Destroy();
  }
}

//--------------------------------------------------------

void ModifiableTestFrame::OnAbout (wxCommandEvent &event)
{
  wxString msg = wxbuildinfo (long_f);
  wxMessageBox (msg, _("Welcome to..."));
}

//-------------------------------------------------

ModifiableTestFrame::~ModifiableTestFrame()
{
  delete modifiableEntities;
}

void ModifiableTestFrame::OnClose(wxCloseEvent &event)
{
  Destroy();
}

void ModifiableTestFrame::OnQuit(wxCommandEvent &event)
{
  Destroy();
}
