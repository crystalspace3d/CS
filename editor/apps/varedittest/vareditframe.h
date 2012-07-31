/*
  Copyright (C) 2011 Christian Van Brussel, Eutyche Mukuama, Dodzi de Souza
      Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

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
#ifndef GRAPH_BEHAVIOURMAIN_H
#define GRAPH_BEHAVIOURMAIN_H

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
#include "modifiableimpl.h"

//*********************************
#include <stdarg.h>
#include "csutil/array.h"
#include "csutil/csstring.h"
#include <iutil/event.h>
#include <iutil/objreg.h>
#include <wx/variant.h>
#include <string>
using namespace std;


#if wxUSE_DATEPICKCTRL
    #include <wx/datectrl.h>
#endif

#include <wx/artprov.h>
struct csVariant;
//class Pump;


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
  void OnPropertyGridChanging (wxPropertyGridEvent& event);
  void OnGetNewValue          (wxPGProperty* property);
  void OnEsc                  (wxKeyEvent& event);
	wxPropertyGridManager *     GetManager();	
	
 private:
   /// Generates the GUI based on an iModifiable entity
  void Populate (const iModifiable* dataSource);

  csRefArray<iModifiable> *modifiableEntities;
  size_t focusedIndex;

  // Main window sizer
  wxBoxSizer *mainsizer;
  // Left/ right sizers
  wxStaticBoxSizer *left_vsizer;
  wxStaticBoxSizer *right_vsizer;
  wxPropertyGridManager* m_pPropGridManager;
  wxPropertyGridPage* page;
  wxPropertyGridManager* pgMan;

  wxButton *btnCycle;
  wxButton *btnSave;

  iObjectRegistry* object_reg;
  static bool GeneralEventHandler(iEvent& ev);
  bool TestModifiableEvents(iEvent& ev);
	     
 protected:

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

  void OnClose(wxCloseEvent& event);
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  //void OnPropertyGridSelect( wxPropertyGridEvent& event );

  DECLARE_EVENT_TABLE()
};


class wxVector3f
{
public:
  wxVector3f();
  wxVector3f( double x, double y, double z );
   
  ~wxVector3f();

  double x, y, z;
};

inline bool operator == (const wxVector3f& a, const wxVector3f& b)
{
    return (a.x == b.x && a.y == b.y && a.z == b.z);
}

WX_PG_DECLARE_VARIANT_DATA(wxVector3fVariantData, wxVector3f, wxPG_NO_DECL)

class wxVectorProperty : public wxPGProperty
{
WX_PG_DECLARE_PROPERTY_CLASS(wxVectorProperty)
public:

    wxVectorProperty( const wxString& label = wxPG_LABEL,
                      const wxString& name = wxPG_LABEL,
                      const wxVector3f& value = wxVector3f() );
  
    virtual ~wxVectorProperty();

    WX_PG_DECLARE_PARENTAL_METHODS()
   
protected:
};

///----------------------------------------------------

class wxVector2f
{
public:
  wxVector2f();
  wxVector2f( double x, double y );
 
  ~wxVector2f();

  double x, y;
};

inline bool operator == (const wxVector2f& a, const wxVector2f& b)
{
    return (a.x == b.x && a.y == b.y );
}

WX_PG_DECLARE_VARIANT_DATA(wxVector2fVariantData, wxVector2f, wxPG_NO_DECL)

class wxVector2Property : public wxPGProperty
{
WX_PG_DECLARE_PROPERTY_CLASS(wxVector2Property)
public:

		
    wxVector2Property( const wxString& label = wxPG_LABEL,
		       const wxString& name = wxPG_LABEL,
		       const wxVector2f& value = wxVector2f() );
  
    virtual ~wxVector2Property();

    WX_PG_DECLARE_PARENTAL_METHODS()
   
protected:
};

//--------------------------------------------------

class wxVector4f
{
public:
    wxVector4f();
     wxVector4f( double x, double y, double z, double w );
   
  
   
	~wxVector4f();
	
	

    double x, y, z , w;
};

inline bool operator == (const wxVector4f& a, const wxVector4f& b)
{
    return (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}

WX_PG_DECLARE_VARIANT_DATA(wxVector4fVariantData, wxVector4f, wxPG_NO_DECL)

class wxVector4Property : public wxPGProperty
{
    WX_PG_DECLARE_PROPERTY_CLASS(wxVector4Property)
public:

    wxVector4Property( const wxString& label = wxPG_LABEL,
                      const wxString& name = wxPG_LABEL,
                      const wxVector4f& value = wxVector4f() );
  
    virtual ~wxVector4Property();

    WX_PG_DECLARE_PARENTAL_METHODS()
   
protected:
};



///----------------------------------------------
// WX_PG_DECLARE_PROPERTY(wxSliderProperty, const float& , float)
/*
 class wxSliderProperty : public wxPGProperty
{
  WX_PG_DECLARE_PROPERTY_CLASS()
public:

    wxSliderProperty( const wxString& label = wxPG_LABEL,
                            const wxString& name = wxPG_LABEL,
                            const float value = 0.0 )
    virtual ~wxSliderProperty ();

    //virtual void OnSetValue();  // Override to allow image loading.

    //WX_PG_DECLARE_CHOICE_METHODS()
    //WX_PG_DECLARE_EVENT_METHODS()
    //WX_PG_DECLARE_CUSTOM_PAINT_METHODS()

    
protected:
  value = m_sliderValue;
};
//*/

//------------------------------------------------------------------------------


#endif // GRAPH_BEHAVIOURMAIN_H
