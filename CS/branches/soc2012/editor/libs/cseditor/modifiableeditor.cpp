/*
    Copyright (C) 2012 by Andrei Bârsan

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
#include "modifiableeditor.h"

#include "imesh/particles.h"
#include "iengine/mesh.h"

// Custom PG slider
#include "cseditor/wxpgslider.h"

// Needed for the properties
#include <csutil/cscolor.h>
#include <csgeom/vector3.h>
#include <wx/variant.h>

BEGIN_EVENT_TABLE(ModifiableEditor, wxPanel)
  EVT_PG_CHANGED(pageId, ModifiableEditor::OnPropertyGridChanging)
  EVT_SIZE(ModifiableEditor::OnSize)
END_EVENT_TABLE()

ModifiableEditor::ModifiableEditor( wxWindow* parent, iEditor* editor, wxWindowID id, const wxPoint& position, const wxSize& size, long style, const wxString& name )
  : wxPanel (parent, id, position, size, style, name)
{
  this->editor = editor;

  // Since the PG components are being dynamically loaded, this function never gets
  // to be run and initialize the wxpg resource module, causing some pretty nasty
  // errors (on Windows, at least)
  // Also, this currently causes *a lot* of first-chance exceptions to get thrown
  // when the app is closing
  wxPGInitResourceModule();

  // Prepare the property grid
  pgMan = new wxPropertyGridManager (this, pageId,
    wxDefaultPosition, size,
    // These and other similar styles are automatically
    // passed to the embedded wxPropertyGrid.
    wxPG_SPLITTER_AUTO_CENTER|
    // Include description box.
    wxPG_DESCRIPTION |
    // Plus defaults.
    wxPG_EX_HELP_AS_TOOLTIPS);

  pgMan->SetPropertyAttributeAll (wxPG_BOOL_USE_CHECKBOX,true);
}

void ModifiableEditor::SetModifiable(iModifiable* modifiable) 
{
  activeModifiable = modifiable;
  // Force a refresh
  Populate();
}

iModifiable* ModifiableEditor::GetModifiable() const 
{
  return activeModifiable;
}

void ModifiableEditor::NoModifiable()
{
  activeModifiable = nullptr;
  pgMan->Clear();
  pgMan->SetDescription(wxT("Error"), wxT("No valid object to be edited"));
}

void ModifiableEditor::Populate ()
{
  // All pages are cleared from the page manager
  pgMan->Clear ();
	
  csRef<iModifiableDescription> description(activeModifiable->GetDescription());

  // Fetch the iTranslator, to attempt to fetch existing translations of
  // the parameter names and descriptions
  // TODO: make translations work again
    
  pgMan->AddPage (wxT("Properties"));
  pgMan->SetExtraStyle ( wxPG_EX_HELP_AS_TOOLTIPS );

  size_t nPg = pgMan->GetPageCount ();
	
  if (nPg >0)
  { 
    page = pgMan->GetPage (wxT ("Properties"));
    pgMan->SetDescription (wxT ("Page Manager"), wxT ("New Page added!"));

    wxString categoryName (wxT("= entity name here ="), wxConvUTF8);
	  
    wxPropertyCategory * ctgr = new wxPropertyCategory (categoryName);
    page ->Append (ctgr);

    for (size_t i = 0; i< description->GetParameterCount(); i++)
    {
      const iModifiableParameter* param = description->GetParameterByIndex(i);
      csVariant* variant = activeModifiable->GetParameterValue(param->GetID());
  
      wxString originalName( param->GetName(), wxConvUTF8 );
      //wxString translation( translator->GetMsg(param->GetName()), wxConvUTF8 );
      wxString translation( originalName );
      wxString description( param->GetDescription(), wxConvUTF8 );

      switch (param->GetType())
      {
      case CSVAR_STRING:
      {
  wxString stringValue (variant->GetString(), wxConvUTF8);
	wxStringProperty* stringP = new wxStringProperty (translation, originalName);
	page->Append (stringP);
	stringP->SetValue (stringValue);
	page->SetPropertyHelpString(originalName, description);
      }
      break;

      case CSVAR_LONG :
      {
	wxString longValue = wxString::Format (wxT("%ld"), (int) variant->GetLong());
	wxIntProperty* intP = new wxIntProperty(translation, originalName);
	page->Append(intP);
	intP->SetValue(longValue);					
	pgMan->GetGrid ()->SetPropertyValue (intP, longValue);
	page->SetPropertyHelpString(originalName, description);
      }
      break;

      case CSVAR_FLOAT:
      { 
	double value = variant->GetFloat();

  // Generate a homebrewed slider 
	wxPGSliderProperty* sliderP = new wxPGSliderProperty (translation, originalName, value, 0, 10000);
	page->Append (sliderP);
	sliderP->Init ();
	page->SetPropertyHelpString (originalName, description);
      }
      break;

      case CSVAR_BOOL:
      {
	wxBoolProperty* boolP = new wxBoolProperty(translation, originalName);
  boolP->SetValue ( variant->GetBool ());
	page->Append (boolP);
	pgMan->SetPropertyAttribute (boolP, wxPG_BOOL_USE_CHECKBOX, (long)1, wxPG_RECURSE);	
	page->SetPropertyHelpString(originalName, description);
      }
      break;
	
      case CSVAR_COLOR :
      {
	csColor colorValue(variant->GetColor ());
	int red   = colorValue[0] * 255;
	int blue  = colorValue[1] * 255;
	int green = colorValue[2] * 255;
	wxColourProperty* colorP = new wxColourProperty (translation, originalName, wxColour (red,blue,green) );
	page->Append (colorP);
	page->SetPropertyHelpString(originalName, description);

      }
      break;
	
      case CSVAR_VECTOR3 :
      {
	csVector3 vector3Value (variant->GetVector3());
	double x = vector3Value.x;
	double y = vector3Value.y;
	double z = vector3Value.z;
	wxVectorProperty *vector3P = new wxVectorProperty(translation, originalName, wxVector3f(x,y,z));
	page->Append (vector3P);
	page->SetPropertyHelpString(originalName, description);

      }
      break;
      
      case CSVAR_VECTOR2 :
      {
	csVector2 vector2Value (variant->GetVector2());
	double x = vector2Value.x;
	double y = vector2Value.y;
	wxVector2Property *vector2P = new wxVector2Property(translation, originalName ,wxVector2f(x,y));
	page->Append (vector2P);
	page->SetPropertyHelpString(originalName,description);

      }
      break;
				
      case CSVAR_VECTOR4 :
      {
	csVector4 vector4Value (variant->GetVector4());
	double x = vector4Value.x;
	double y = vector4Value.y;
	double z = vector4Value.z;
	double w = vector4Value.w;
	wxVector4Property *vector4P = new wxVector4Property(translation, originalName, wxVector4f(x,y,z,w));
	page->Append (vector4P);
	page->SetPropertyHelpString(originalName, description );

      }
      break;

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
    
      delete variant;
    } // end loop through properties
    pgMan->Refresh();
  } 
  else
  {
    pgMan->SetDescription(wxT("Page Manager :"), wxT("Error page no added"));
  }
  //*/
}
  

//---------------------------------------------------

void ModifiableEditor::OnSize(wxSizeEvent& event)
{
  pgMan->SetSize(event.GetSize());
}

void ModifiableEditor::OnGetNewValue (wxPGProperty* property)
{
  wxVariant newValue = property->GetValue ();
  if (newValue.IsNull () || activeModifiable == nullptr)
    return;
  //*
  size_t index = property->GetIndexInParent ();

  csRef<iModifiableDescription> desc = activeModifiable->GetDescription();
  const iModifiableParameter* editedParameter = desc->GetParameterByIndex(index);

  csVariantType compareType = editedParameter->GetType();
  csVariant* variant( activeModifiable->GetParameterValue( editedParameter->GetID()) );
  csVariant oldValue = *variant;

  // TODO: somehow notify iModifiable entity to refresh itself
  //        I think the best way is to post an event; this is currently
  //        just a GUI module - it shouldn't give orders to the objects
  //        directly, let (in this case) the actual particle editor do
  //        the refreshing
  
  if (compareType == CSVAR_STRING)
  {
    variant->SetString(newValue.GetString().mbc_str());
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_LONG)
  {
    variant->SetLong(newValue.GetLong());    
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_FLOAT)
  {
    variant->SetFloat (newValue.GetDouble ());
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_BOOL)
  {
    variant->SetBool (newValue.GetBool ());
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_COLOR)
  {	
    const wxString variantValue = page->GetPropertyValueAsString (property);
    wxColour txcol (wxString::FromAscii("rgb") + variantValue);
    variant->SetColor ( csColor ((float)txcol.Red ()/255, (float)txcol.Green ()/255, (float)txcol.Blue ()/255) );
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_VECTOR3)
  {
    float valueX = property->Item(0)->GetValue().GetDouble ();
    float valueY = property->Item(1)->GetValue().GetDouble ();
    float valueZ = property->Item(2)->GetValue().GetDouble ();
    variant->SetVector3(csVector3(valueX,valueY,valueZ));
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);
				
  }
  
  else if (compareType == CSVAR_VECTOR4)
  {

    float valueX = property->Item(0)->GetValue().GetDouble ();
    float valueY = property->Item(1)->GetValue().GetDouble ();
    float valueZ = property->Item(2)->GetValue().GetDouble ();
    float valueW = property->Item(3)->GetValue().GetDouble ();
    variant->SetVector4(csVector4(valueX,valueY,valueZ,valueW));
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);
  }
  else if (compareType == CSVAR_VECTOR2)
  {

    float valueX = property->Item(0)->GetValue().GetDouble ();
    float valueY = property->Item(1)->GetValue().GetDouble ();
    variant->SetVector2(csVector2(valueX,valueY));
    activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);				
  }
  else if (compareType == CSVAR_MATRIX3)
  {

  }
  else if (compareType == CSVAR_TRANSFORM)
  {

  }
  else if (compareType == CSVAR_IBASE)
  {

  }
  else if (compareType == CSVAR_ARRAY)
  {

  }
  else
  {
    pgMan->SetDescription (wxT ("Page Manager :"), wxT ("Message test"));
  }

  delete variant;
  //*/
}


void ModifiableEditor::OnPropertyGridChanging (wxPropertyGridEvent& event)
{
  wxPGProperty* property = event.GetProperty ();
  OnGetNewValue (property);
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
