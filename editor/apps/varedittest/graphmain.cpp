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
#include "cssysdef.h"

#include "graphmain.h"
#include "graphedit.h"
#include "graphnode.h"

//--------------------------
// Main propertygrid header.
#include "wx/propgrid/propgrid.h"

// Needed for implementing custom properties.

#include "wx/propgrid/propdev.h"

// Extra property classes.
#include "wx/propgrid/advprops.h"
// This defines wxPropertyGridManager.
#include "wx/propgrid/manager.h"
#include "wx/propgrid/editors.h"
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
//#include <wx/slider.h>
#include <wx/colour.h>
//*********************************
#include <csutil/cscolor.h>
#include <csgeom/vector3.h>
#include "wx/propgrid/advprops.h"
#include "slidereditor.h"

//---------
#include <iostream>
using namespace std;
//----------------
//wxPGRegisterEditorClass( MultiButtonTextCtrlEditor );


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

BEGIN_EVENT_TABLE (Graph_behaviourFrame, wxFrame)
    EVT_CLOSE (Graph_behaviourFrame::OnClose)
    EVT_MENU (idMenuQuit, Graph_behaviourFrame::OnQuit)
    EVT_BUTTON (idBtnTest1, Graph_behaviourFrame::OnPopulateClick)
    //EVT_BUTTON(idBtnTest2, Graph_behaviourFrame::OnConfirmKeyPress)
    EVT_MENU (idMenuAbout, Graph_behaviourFrame::OnAbout)
    EVT_KEY_DOWN (Graph_behaviourFrame::OnEsc)
    EVT_KEY_UP (Graph_behaviourFrame::OnEsc)
    EVT_PG_CHANGED ( pageId, Graph_behaviourFrame::OnPropertyGridChanging )
END_EVENT_TABLE ()



//-------------------------------------------------------------------


Graph_behaviourFrame::Graph_behaviourFrame ()
: wxFrame (NULL, idFrame,wxT ("Graph_edit"),wxDefaultPosition,wxSize (600,600))
{
  mainsizer=new wxBoxSizer (wxHORIZONTAL);
  
  left_vsizer=new wxStaticBoxSizer (wxHORIZONTAL,this,_T("Nodes"));
  right_vsizer=new wxStaticBoxSizer (wxHORIZONTAL,this,_T(""));

  mainsizer->Add (left_vsizer,1,wxALL|wxEXPAND,5);

  modifiableEntities = new csRefArray<iModifiable>();
  focusedIndex = 0;

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

  btnTest1 = new wxButton (this, idBtnTest1, wxT ("Cycle through nodes"), wxDefaultPosition, wxDefaultSize, 0);
  btnTest2 = new wxButton (this, idBtnTest2, wxT ("Save"), wxDefaultPosition, wxDefaultSize, 0);

  left_vsizer->Add (btnTest1);
  left_vsizer->Add (btnTest2);
  SetSizer (mainsizer);
  //-------------------------------------------------------------------------
 
  pgMan = new wxPropertyGridManager (this, pageId,
				    wxDefaultPosition, wxDefaultSize,
				    // These and other similar styles are automatically
				    // passed to the embedded wxPropertyGrid.
				    wxPG_SPLITTER_AUTO_CENTER|
				    // Include description box.
				    wxPG_DESCRIPTION |
				    // Plus defaults.
				    wxPG_EX_HELP_AS_TOOLTIPS);
	
	
  pgMan->SetPropertyAttributeAll (wxPG_BOOL_USE_CHECKBOX,true);
  mainsizer->Add (pgMan,1,wxTOP|wxBOTTOM|wxRIGHT|wxEXPAND,5);
	

  //--------------------------------------------------------
		

} 

void Graph_behaviourFrame::AddModifiable(iModifiable* modifiable) {
  this->modifiableEntities->Push(modifiable);
}

//_---------------------------------
WX_PG_IMPLEMENT_VARIANT_DATA (wxVector3fVariantData, wxVector3f)

WX_PG_IMPLEMENT_PROPERTY_CLASS (wxVectorProperty,wxPGProperty, wxVector3f,const wxVector3f&,TextCtrl)

wxVector3f::wxVector3f ()
{
        x = y = z = 0.0;
}
wxVector3f::wxVector3f (double x, double y, double z ): x (x), y (y), z (z)
{
}

wxVector3f::~wxVector3f ()
{
}

wxVectorProperty::wxVectorProperty(const wxString& label, const wxString& name, const wxVector3f& value ): wxPGProperty (label,name)
{
	wxVariant variant;
	variant << value;
	SetValue(variant);
    //SetValue( wxVector3fToVariant(value) );
	AddPrivateChild( new wxFloatProperty(wxT("X"),wxPG_LABEL,value.x) );
	AddPrivateChild( new wxFloatProperty(wxT("Y"),wxPG_LABEL,value.y) );
	AddPrivateChild( new wxFloatProperty(wxT("Z"),wxPG_LABEL,value.z) );
}

void wxVectorProperty::RefreshChildren ()
{
    if (!GetCount() ) return;
    wxVector3f& vector = wxVector3fFromVariant (m_value);
    Item (0)->SetValue (vector.x);
    Item (1)->SetValue (vector.y);
    Item (2)->SetValue (vector.z);
}

void wxVectorProperty::ChildChanged ( wxVariant& thisValue, int childIndex, wxVariant& childValue) const
{
		
    wxVector3f& vector = wxVector3fFromVariant (thisValue);
    switch (childIndex)
    {
        case 0: vector.x = childValue.GetDouble ();										
				break;
				case 1: vector.y = childValue.GetDouble (); 	 
				break;
        case 2: vector.z = childValue.GetDouble ();				 				
				break;
    }

}

wxVectorProperty::~wxVectorProperty () { }
//__----------------------------------------------------

WX_PG_IMPLEMENT_VARIANT_DATA (wxVector2fVariantData, wxVector2f)

WX_PG_IMPLEMENT_PROPERTY_CLASS (wxVector2Property,wxPGProperty, wxVector2f,const wxVector2f&,TextCtrl)

wxVector2f::wxVector2f ()
{
        x = y = 0.0;
}

wxVector2f::wxVector2f (double x, double y ): x (x), y (y)
{
}

wxVector2f::~wxVector2f ()
{
}

wxVector2Property::wxVector2Property (const wxString& label, const wxString& name,const wxVector2f& value ): wxPGProperty(label,name)
{
    wxVariant variant;
    variant << value;
    SetValue (variant);
    //SetValue( wxVector3fToVariant(value) );
    AddPrivateChild (new wxFloatProperty (wxT ("X"),wxPG_LABEL,value.x) );
    AddPrivateChild (new wxFloatProperty (wxT ("Y"),wxPG_LABEL,value.y) );
}

void wxVector2Property::RefreshChildren ()
{
    if (!GetCount () ) return;
    wxVector2f& vector = wxVector2fFromVariant (m_value);
    Item (0)->SetValue (vector.x);
    Item (1)->SetValue (vector.y);
}

void wxVector2Property::ChildChanged (wxVariant& thisValue, int childIndex, wxVariant& childValue ) const
{
		
    wxVector2f& vector = wxVector2fFromVariant (thisValue);
    switch ( childIndex )
    {
        case 0: vector.x = childValue.GetDouble ();
				break;
      	case 1: vector.y = childValue.GetDouble (); 
				break;
   
    }

}

wxVector2Property::~wxVector2Property () { }


//--------------------------------------------------------
WX_PG_IMPLEMENT_VARIANT_DATA (wxVector4fVariantData, wxVector4f)

WX_PG_IMPLEMENT_PROPERTY_CLASS (wxVector4Property, wxPGProperty, wxVector4f, const wxVector4f&,TextCtrl)

wxVector4f::wxVector4f ()
{
	x = y = z = w = 0.0;
}
wxVector4f::wxVector4f (double x, double y, double z , double w ): x (x), y (y), z (z), w (w)
{
}

wxVector4f::~wxVector4f ()
{
		
}

wxVector4Property::wxVector4Property (const wxString& label, const wxString& name, const wxVector4f& value ): wxPGProperty(label,name)
{

	wxVariant variant;
	variant << value;
	SetValue(variant);
    
	AddPrivateChild( new wxFloatProperty(wxT("X"),wxPG_LABEL,value.x) );
	AddPrivateChild( new wxFloatProperty(wxT("Y"),wxPG_LABEL,value.y) );
	AddPrivateChild( new wxFloatProperty(wxT("Z"),wxPG_LABEL,value.z) );
	AddPrivateChild( new wxFloatProperty(wxT("W"),wxPG_LABEL,value.w) );
}

void wxVector4Property::RefreshChildren()
{
	if (!GetCount() ) return;
	wxVector4f& vector = wxVector4fFromVariant(m_value);
	Item (0)->SetValue (vector.x );
	Item (1)->SetValue (vector.y );
	Item (2)->SetValue (vector.z );
	Item (3)->SetValue (vector.w );
}

void wxVector4Property::ChildChanged (wxVariant& thisValue, int childIndex, wxVariant& childValue ) const
{
		
    wxVector4f& vector = wxVector4fFromVariant (thisValue);
    switch (childIndex)
    {
        case 0: vector.x = childValue.GetDouble ();										
				break;
				case 1: vector.y = childValue.GetDouble (); 	 
				break;
        case 2: vector.z = childValue.GetDouble ();				 				
				break;
				case 3: vector.w = childValue.GetDouble ();				 				
				break;
    }

}

wxVector4Property::~wxVector4Property () { }

//------------------------------------------------------------

void Graph_behaviourFrame::Populate (const iModifiable* dataSource)
{
  // all pages are cleared from the page manager
  pgMan->Clear ();
	
  csRef<iModifiableDescription> description(dataSource->GetDescription());

  pgMan->AddPage (wxT("Properties"));
  pgMan->SetExtraStyle ( wxPG_EX_HELP_AS_TOOLTIPS );

  size_t nPg = pgMan->GetPageCount ();
	
  if (nPg >0)
  { 
    page = pgMan->GetPage (wxT ("Properties"));
    pgMan->SetDescription (wxT ("Page Manager"), wxT ("New Page is added"));

    wxString categoryName (wxT("= entity name here ="), wxConvUTF8);
	  
    wxPropertyCategory * ctgr = new wxPropertyCategory (categoryName);
    page ->Append (ctgr);
    //ctgr -> GetGrid ()-> MakeColumnEditable (0);

    /*
    // This is the old code that iterates over a nodeFactory's parameter list; we're going to
    // do something similar, only for the iModifiableDescription
    nodeFactory = node->GetFactory ();
    for (size_t i = 0; i < nodeFactory->GetParameterCount (); i++)
    {
      const csOptionDescription* option = nodeFactory->GetParameterDescription (i);
      */

    for (size_t i = 0; i< description->GetParameterCount(); i++)
    {
      const iModifiableParameter* param = description->GetParameterByIndex(i);
      csVariant* variant = dataSource->GetParameterValue(param->GetID());

      switch (param->GetType())
      {
      case CSVAR_STRING:
      {
	wxString stringDescription (param->GetDescription(), wxConvUTF8);
	wxString stringName (param->GetName(), wxConvUTF8);			
  
  wxString stringValue (variant->GetString(), wxConvUTF8);
	wxStringProperty* stringP = new wxStringProperty (stringName);
	page->Append (stringP);
	stringP->SetValue (stringValue);
	page->SetPropertyHelpString(stringName, stringDescription);
      }
      break;

      case CSVAR_LONG :
      {
	wxString longDescription (param->GetDescription(), wxConvUTF8);
	wxString longName (param->GetName(), wxConvUTF8);
	wxString longValue = wxString::Format (wxT("%ld"), (int) variant->GetLong());
	wxIntProperty* intP = new wxIntProperty(longName);				
	page->Append(intP);
	intP->SetValue(longValue);					
	pgMan->GetGrid ()->SetPropertyValue (intP, longValue);
	page->SetPropertyHelpString(longName, longDescription);
      }
      break;

      case CSVAR_FLOAT:
      { 
	wxString floatDescription (param->GetDescription(), wxConvUTF8);
	wxString floatName (param->GetName(), wxConvUTF8);
	double value = variant->GetFloat();

  // Generate a homebrewed slider 
  //TODO: complete slider rewrite
  /*
	wxPGSliderProperty* sliderP = 
	  new wxPGSliderProperty (floatName, floatName, value, 0, 10000);
	page->Append (sliderP);
	sliderP->Init ();
  */

  // Temporarily just use a text field
  wxFloatProperty* floatP = new wxFloatProperty(floatName);
  page->Append(floatP);
  floatP->SetValue(value);
  pgMan->GetGrid()->SetPropertyValue(floatP, value);

	page->SetPropertyHelpString (floatName, floatDescription);
      }
      break;

      case CSVAR_BOOL:
      {
	wxString boolDescription (param->GetDescription(), wxConvUTF8);
	wxString boolName (param->GetName(), wxConvUTF8);
	wxBoolProperty* boolP = new wxBoolProperty(boolName);
  boolP->SetValue ( variant->GetBool ());
	page->Append (boolP);
	pgMan->SetPropertyAttribute (boolP, wxPG_BOOL_USE_CHECKBOX, (long)1, wxPG_RECURSE);	
	page->SetPropertyHelpString(boolName,boolDescription );
      }
      break;
	
      case CSVAR_COLOR :
      {
	wxString colorDescription(param->GetDescription(), wxConvUTF8);
	wxString colorName (param->GetName(), wxConvUTF8);
	csColor colorValue(variant->GetColor ());
	int red  = colorValue[0]*255;
	int blue  = colorValue[1]*255;
	int green = colorValue[2]*255;
	wxColourProperty* colorP = new wxColourProperty (colorName,wxPG_LABEL,wxColour (red,blue,green) );
	page->Append (colorP);
	page->SetPropertyHelpString(colorName,colorDescription );

      }
      break;
	
      case CSVAR_VECTOR3 :
      {
        // TODO: fix crash with vector3
	wxString vector3Description(param->GetDescription(), wxConvUTF8);
	wxString vector3Name (param->GetName(), wxConvUTF8);
	csVector3 vector3Value (variant->GetVector3());
	double x = vector3Value.x;
	double y = vector3Value.y;
	double z = vector3Value.z;
	wxVectorProperty *vector3P = new wxVectorProperty(vector3Name, vector3Name, wxVector3f(x,y,z));
	page->Append (vector3P);
	page->SetPropertyHelpString(vector3Name, vector3Description);

      }
      break;
      /*
      case CSVAR_VECTOR2 :
      {
	wxString vector2Description(param->GetDescription(), wxConvUTF8);
	wxString vector2Name (param->GetName(), wxConvUTF8);
	csVector2 vector2Value (variant->GetVector2());
	double x = vector2Value.x;
	double y = vector2Value.y;
	wxVector2Property *vector2P = new wxVector2Property(vector2Name,wxT("csVector2"),wxVector2f(x,y));
	page->Append (vector2P);
	page->SetPropertyHelpString(vector2Name,vector2Description );

      }
      break;
				
      case CSVAR_VECTOR4 :
      {
	wxString vector4Description(param->GetDescription(), wxConvUTF8);
	wxString vector4Name (param->GetName(), wxConvUTF8);
	csVector4 vector4Value (variant->GetVector4());
	double x = vector4Value.x;
	double y = vector4Value.y;
	double z = vector4Value.z;
	double w = vector4Value.w;
	wxVector4Property *vector4P = new wxVector4Property(vector4Name,wxT("csVector4"),wxVector4f(x,y,z,w));
	page->Append (vector4P);
	page->SetPropertyHelpString(vector4Name,vector4Description );

      }
      break;

      //*/
				
	/*
      case CSVAR_VFSPATH :
      {
	wxString pathDescription (param->GetDescription(), wxConvUTF8);
	wxString pathName (param->GetName(), wxConvUTF8);
	wxString pathValue (variant->GetVFSPath (), wxConvUTF8);
	wxFileProperty* pathP = new wxFileProperty (pathName);
	page->Append (pathP );
	//pathP->SetAttribute(wxPG_FILE_SHOW_FULL_PATH,false);
	pathP->SetValue (pathValue);
	page->SetPropertyHelpString(pathName,pathDescription );

      }
      */

      case CSVAR_MATRIX3: 
      {

      }
      break;

      case CSVAR_TRANSFORM:
      {

      }
      break;

      case CSVAR_IBASE:
      {


      }
      break;

      case CSVAR_ARRAY:
      {

      }
      break;

      default:
	pgMan->SetDescription(wxT("Page Manager :"), wxT("Select a property to add a new value"));
      } // end switch
    
    } // end loop through properties
  } 
  else
  {
    pgMan->SetDescription(wxT("Page Manager :"), wxT("Error page no added"));
  }
}

//---------------------------------------------------

void Graph_behaviourFrame::OnGetNewValue (wxPGProperty* property)
{
  wxVariant newValue = property->GetValue ();
  if (newValue.IsNull ())
    return;

  size_t index = property->GetIndexInParent ();

  iModifiable* currentModifiable = modifiableEntities->Get(focusedIndex);
    
  // Store the returned csPtrs in csRefs (they get cleaned up after this fct)
  // It's okay, though, since the GetDescription and GetParameter always allocate
  // new resources.
  csRef<iModifiableDescription> desc = currentModifiable->GetDescription();
  const iModifiableParameter* editedParameter = desc->GetParameterByIndex(index);


  csVariantType compareType = editedParameter->GetType();
  csVariant* variant( currentModifiable->GetParameterValue( editedParameter->GetID()) );
  csVariant oldValue = *variant;
  
  if (compareType == CSVAR_STRING)
  {
    variant->SetString(newValue.GetString().mbc_str());
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_LONG)
  {
    variant->SetLong(newValue.GetLong());    
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_FLOAT)
  {
    variant->SetFloat (newValue.GetDouble ());
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_BOOL)
  {
    variant->SetBool (newValue.GetBool ());
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_COLOR)
  {	
    const wxString variantValue = page->GetPropertyValueAsString (property);
    wxColour txcol (wxString::FromAscii("rgb") + variantValue);
    variant->SetColor ( csColor ((float)txcol.Red ()/255, (float)txcol.Green ()/255, (float)txcol.Blue ()/255) );
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_VECTOR3)
  {
    float valueX = property->Item(0)->GetValue().GetDouble ();
    float valueY = property->Item(1)->GetValue().GetDouble ();
    float valueZ = property->Item(2)->GetValue().GetDouble ();
    variant->SetVector3(csVector3(valueX,valueY,valueZ));
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);
				
  }
  
  else if (compareType == CSVAR_VECTOR4)
  {

    float valueX = property->Item(0)->GetValue().GetDouble ();
    float valueY = property->Item(1)->GetValue().GetDouble ();
    float valueZ = property->Item(2)->GetValue().GetDouble ();
    float valueW = property->Item(3)->GetValue().GetDouble ();
    variant->SetVector4(csVector4(valueX,valueY,valueZ,valueW));
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_VECTOR2)
  {

    float valueX = property->Item(0)->GetValue().GetDouble ();
    float valueY = property->Item(1)->GetValue().GetDouble ();
    variant->SetVector2(csVector2(valueX,valueY));
    currentModifiable->SetParameterValue(editedParameter->GetID(), *variant);				
  }
  else
  {
    pgMan->SetDescription (wxT ("Page Manager :"), wxT ("Message test"));
  }
}

//---------------------------------------------

void Graph_behaviourFrame::OnPopulateClick (wxCommandEvent &event )
{
  // Changes the current active node

  focusedIndex++;
  if(focusedIndex >= modifiableEntities->GetSize()) 
    focusedIndex = 0;

  Populate (modifiableEntities->Get(focusedIndex));

  pgMan->SetFocus ();
}
//-----------------------------------------------

void Graph_behaviourFrame::OnEsc (wxKeyEvent& event)
{
  if ( event.GetKeyCode () == WXK_ESCAPE)
  {
    Destroy();
  }
}

void Graph_behaviourFrame::OnPropertyGridChanging (wxPropertyGridEvent& event)
{
  wxPGProperty* property = event.GetProperty ();
  OnGetNewValue (property);
  
	
}

//--------------------------------------------------------

void Graph_behaviourFrame::OnAbout (wxCommandEvent &event)
{
  wxString msg = wxbuildinfo (long_f);
  wxMessageBox (msg, _("Welcome to..."));
}

//-------------------------------------------------

Graph_behaviourFrame::~Graph_behaviourFrame()
{
  delete modifiableEntities;
}

void Graph_behaviourFrame::OnClose(wxCloseEvent &event)
{
  Destroy();
}

void Graph_behaviourFrame::OnQuit(wxCommandEvent &event)
{
  Destroy();
}


