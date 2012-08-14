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
#ifndef __VAREDITFRAME_H__
#define __VAREDITFRAME_H__

#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <iostream>
#endif

#include "vareditapp.h"
#include "cseditor/wxpgslider.h"
#include <csutil/refarr.h>
#include <csutil/weakref.h>

// Main propertygrid header.
#include "cseditor/wx/propgrid/propgrid.h"
#include "cseditor/wx/propgrid/editors.h"

// Needed for implementing custom properties.
#include "cseditor/wx/propgrid/propdev.h"
#include <wx/panel.h>

// Extra property classes.
#include "cseditor/wx/propgrid/advprops.h"

// This defines wxPropertyGridManager.
#include "cseditor/wx/propgrid/manager.h"

// Includes basic iModifiable data types
#include "cseditor/modifiableimpl.h"
#include "cseditor/modifiableeditor.h"

#include <stdarg.h>
#include "csutil/array.h"
#include "csutil/csstring.h"
#include "cstool/initapp.h"
#include "iutil/document.h"
#include "ivaria/translator.h"
#include "iutil/eventq.h"
#include <iutil/event.h>
#include <iutil/objreg.h>
#include <wx/variant.h>
#include <string>

using namespace std;
using namespace CS::EditorApp;

#if wxUSE_DATEPICKCTRL
    #include <wx/datectrl.h>
#endif

#include <wx/artprov.h>
struct csVariant;

// We need to implement WeakReferenced for the pump's weakref
class ModifiableTestFrame : public wxFrame,
                            public CS::Utility::WeakReferenced
{
 public:
  ModifiableTestFrame         (iObjectRegistry* object_reg);
  ~ModifiableTestFrame ();

  void AddModifiable          (iModifiable* modifiable);
  bool Initialize ();
  void PushFrame ();

  void OnPopulateClick        (wxCommandEvent &event);
  void OnEsc                  (wxKeyEvent& event);
	wxPropertyGridManager *     GetManager();	
	
 private:
   /// Generates the GUI based on an iModifiable entity
  void Populate (iModifiable* dataSource);

  csRefArray<iModifiable> *modifiableEntities;
  size_t focusedIndex;

  // Main window sizer
  wxBoxSizer *mainsizer;
  // Left/ right sizers
  wxStaticBoxSizer *left_vsizer;
  wxStaticBoxSizer *right_vsizer;
  
  wxButton *btnCycle;
  wxButton *btnSave;

  iObjectRegistry* object_reg;
  static bool GeneralEventHandler(iEvent& ev);
  bool TestModifiableEvents(iEvent& ev);
	     
private:

  enum
  {
    idFrame = 1000,
    idMenuQuit = 1001,
    idMenuAbout = 1002,
    idgrid = 1003,
    idBtnTest1 = 1004,
    idBtnTest2 = 2000,
    idMenuEdit = 1006,
    idMenuAddprop = 1007,
    idCombo = 1008,
    pageId = 1
  };

  // Container of the iModifiable editing gui
  ModifiableEditor* modifiableEditor;

  void OnClose(wxCloseEvent& event);
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);

  DECLARE_EVENT_TABLE()
};


#endif // __VAREDITFRAME_H__
