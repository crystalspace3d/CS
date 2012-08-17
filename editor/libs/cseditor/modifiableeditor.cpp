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
#include "iutil/objreg.h"
#include "cseditor/modifiableimpl.h"
#include "cseditor/modifiableeditor.h"

#include "imesh/particles.h"
#include "iengine/mesh.h"

// Custom PG slider
#include "cseditor/wxpgslider.h"

// Needed for the properties
#include <csutil/cscolor.h>
#include <csgeom/vector3.h>
#include <wx/variant.h>


//----------------- Custom properties for the property grid ---------------------

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

using namespace CS::EditorApp;

BEGIN_EVENT_TABLE(ModifiableEditor, wxPanel)
  EVT_PG_CHANGING (pageId,  ModifiableEditor::OnPropertyGridChanging)
  EVT_PG_CHANGED  (pageId,  ModifiableEditor::OnPropertyGridChanged)
  EVT_SIZE        (         ModifiableEditor::OnSize)
END_EVENT_TABLE()

ModifiableEditor::ModifiableEditor( iObjectRegistry* object_reg, wxWindow* parent, wxWindowID id, const wxPoint& position, const wxSize& size, long style, const wxString& name )
  : wxPanel (parent, id, position, size, style, name)
{
  this->object_reg = object_reg;

  // Since the PG components are being dynamically loaded, this function never gets
  // to be run and initialize the wxpg resource module, causing some pretty nasty
  // errors (on Windows, at least)
  // Also, this currently causes *a lot* of first-chance exceptions to get thrown
  // when the app is closing
#ifndef wxPG_USE_WXMODULE
  wxPGInitResourceModule();
#endif
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

  pgMan->SetPropertyAttributeAll (wxPG_BOOL_USE_CHECKBOX, true);
  pgMan->SetDescBoxHeight( 56, true );

  // Get the iTranslator, to attempt to fetch existing translations of
  // the parameter names and descriptions
  translator = csQueryRegistry<iTranslator>(object_reg);

  // Temporary
  /*
  wxPropertyGrid::RegisterAdditionalEditors();
  wxIntProperty* prop = new wxIntProperty(wxT("LABLEEL"), wxT("NAME"), 42);
  prop->SetEditor(wxPG_EDITOR(SpinCtrl));
  pgMan->GetGrid()->Append(prop);
  */
}

void ModifiableEditor::SetModifiable(iModifiable* modifiable) 
{
  activeModifiable = modifiable;
  activeModifiableName = "Unnamed entity";
  // Force a refresh
  Populate();
}

void ModifiableEditor::SetModifiableLabel( iModifiable* modifiable, csString& label )
{
  activeModifiable = modifiable;
  activeModifiableName = label;
  Populate();
}

iModifiable* ModifiableEditor::GetModifiable() const 
{
  return activeModifiable;
}

void ModifiableEditor::Populate ()
{
  // All pages are cleared from the page manager
  pgMan->Clear ();
  pgMan->AddPage (wxT("Properties"));
  pgMan->SetExtraStyle ( wxPG_EX_HELP_AS_TOOLTIPS );
	// Append the main Item as the root
  Append(activeModifiable, activeModifiableName);
}

//---------------------------------------------------
void ModifiableEditor::Append(iModifiable* element, csString& name) {

  csRef<iModifiableDescription> description(element->GetDescription());

  page = pgMan->GetPage (wxT ("Properties"));
  pgMan->SetDescription (wxT ("Page Manager"), wxT ("New Page added!"));

  wxString categoryName (name, wxConvUTF8);

  wxPropertyCategory *root = new wxPropertyCategory (categoryName);
  page ->Append (root);

  for (size_t i = 0; i< description->GetParameterCount(); i++)
  {
    const iModifiableParameter* param = description->GetParameterByIndex(i);
    csVariant* variant = element->GetParameterValue(param->GetID());

    wxString originalName( param->GetName(), wxConvUTF8 );
    wxString translation( translator->GetMsg(param->GetName()), wxConvUTF8 );
    wxString description( param->GetDescription(), wxConvUTF8 );

    AppendVariant(root, variant, param->GetConstraint(), originalName, translation, description);

    delete variant;
  } // end loop through properties
  pgMan->Refresh();
}

//---------------------------------------------------

void ModifiableEditor::AppendVariant(wxPGPropArg categoryID, csVariant* variant, const iModifiableConstraint* constraint, const wxString& originalName, const wxString& translatedName, const wxString& translatedDescription)
{
  switch (variant->GetType())
    {
    case CSVAR_STRING:
      {
        wxString stringValue (variant->GetString(), wxConvUTF8);

        wxStringProperty* stringP = new wxStringProperty (translatedName, originalName);
        page->AppendIn(categoryID, stringP);
        stringP->SetValue (stringValue);

      }
      break;

    case CSVAR_LONG :
      {
        if( constraint != nullptr
          && constraint->GetType() == MODIFIABLE_CONSTRAINT_ENUM)
        {
          // Generate a combo-box based on enum values
          const csEnumConstraint* ec = dynamic_cast<const csEnumConstraint*>(constraint);
          csStringArray* csLabels = ec->GetLabels();
          wxArrayString labels;
          for(csStringArray::Iterator i = csLabels->GetIterator(); i.HasNext(); ) 
          {
            labels.Add( wxString( i.Next(), wxConvUTF8) );
          }

          csArray<long>* csValues = ec->GetValues();
          wxArrayInt values;
          for(csArray<long>::Iterator i = csValues->GetIterator(); i.HasNext(); )
          {
            values.Add(i.Next());
          }

          wxEnumProperty* enumP = new wxEnumProperty( translatedName,
            originalName,
            labels,
            values,
            (int)variant->GetLong() ); 
          page->AppendIn(categoryID, enumP);
        }
        else 
        {
          // Plain old text field
          wxString longValue = wxString::Format (wxT("%ld"), (int) variant->GetLong());
          wxIntProperty* intP = new wxIntProperty(translatedName, originalName);
          page->AppendIn(categoryID, intP);
          // Needed to actually refresh the grid and show the value
          pgMan->GetGrid ()->SetPropertyValue (intP, longValue);
        }

        
      }
      break;

    case CSVAR_FLOAT:
      { 
        double value = variant->GetFloat();

        // Generate a homebrewed slider
        wxFloatProperty* fp = new wxFloatProperty(translatedName, originalName, value);
        wxPGEditor* rHandle = wxPropertyGrid::RegisterEditorClass(new wxPGSliderEditor(), wxT("SliderEditor"));
        fp->SetEditor(rHandle);
        page->AppendIn(categoryID, fp);

        // Hack to allow the property's controls to get created; doesn't grab focus,
        // doesn't hurt anyone
        pgMan->GetGrid()->SelectProperty(fp, false);
      }
      break;

    case CSVAR_BOOL:
      {
        wxBoolProperty* boolP = new wxBoolProperty(translatedName, originalName);
        boolP->SetValue ( variant->GetBool ());
        page->AppendIn(categoryID, boolP);
        pgMan->SetPropertyAttribute (boolP, wxPG_BOOL_USE_CHECKBOX, (long)1, wxPG_RECURSE);	
      }
      break;

    case CSVAR_COLOR :
      {
        csColor colorValue(variant->GetColor ());
        int red   = colorValue[0] * 255;
        int blue  = colorValue[1] * 255;
        int green = colorValue[2] * 255;
        wxColourProperty* colorP = new wxColourProperty (translatedName, originalName, wxColour (red,blue,green) );
        page->AppendIn(categoryID, colorP);
      }
      break;

    case CSVAR_VECTOR3 :
      {
        csVector3 vector3Value (variant->GetVector3());
        double x = vector3Value.x;
        double y = vector3Value.y;
        double z = vector3Value.z;
        wxVectorProperty *vector3P = new wxVectorProperty(translatedName, originalName, wxVector3f(x,y,z));
        page->AppendIn(categoryID, vector3P);
      }
      break;

    case CSVAR_VECTOR2 :
      {
        csVector2 vector2Value (variant->GetVector2());
        double x = vector2Value.x;
        double y = vector2Value.y;
        wxVector2Property *vector2P = new wxVector2Property(translatedName, originalName ,wxVector2f(x,y));
        page->AppendIn(categoryID, vector2P);
      }
      break;

    case CSVAR_VECTOR4 :
      {
        csVector4 vector4Value (variant->GetVector4());
        double x = vector4Value.x;
        double y = vector4Value.y;
        double z = vector4Value.z;
        double w = vector4Value.w;
        wxVector4Property *vector4P = new wxVector4Property(translatedName, originalName, wxVector4f(x,y,z,w));
        page->AppendIn(categoryID, vector4P);
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

      /*
    case CSVAR_IBASE:
      {
        iBase* object = variant->GetIBase();
        csRef<iModifiable> modifiable = scfQueryInterface<iModifiable>(object);
        if(modifiable.IsValid()) {
          //Append(modifiable, csString("Nested modifiable"));
        }

      }
      break;

    case CSVAR_ARRAY:
      {
        wxPGProperty* catProp = new wxStringProperty(originalName, wxT("<composed>"));
        catProp->SetFlag(wxPG_PROP_DISABLED);
        page->Append(catProp);
        csArray<csVariant> array = variant->GetArray();
        for(auto i = array.GetIterator(); i.HasNext(); ) {
          
          // An array may have a constraint that matches the type
          // of the element within; if not, it's ignored
          //csVariant var = i.Next();
          AppendVariant(catProp->GetId(), &(i.Next()), constraint, wxString::Format(wxT("%s[%d]"), originalName, i),
            wxString::Format(wxT("%d"), i), wxString());
        }
      }
      break;
      //*/
    default:
      pgMan->SetDescription(wxT("Page Manager :"), wxT("Select a property to add a new value"));

      } // end switch
      
      // Doesn't help
      //pgMan->RefreshGrid();


      if(page->GetProperty(originalName))
        page->SetPropertyHelpString(originalName, translatedDescription );
      else {
        // Needed in order to print out the wide char type from GetData
        wprintf(wxT("Couldn't find property with name: %s \n"), originalName.GetData());
      }
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
   
  // This FAILS with nested properties, such as in the case of arrays, or nested 
  // iModifiables
  const iModifiableParameter* editedParameter = desc->GetParameterByIndex(index);

  csVariantType compareType = editedParameter->GetType();
  csVariant* variant( activeModifiable->GetParameterValue( editedParameter->GetID()) );
  csVariant oldValue = *variant;
  
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
  /*
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
    // maybe just generate unique IDs for each array element?
    printf("Edited an array! ID=%d\n", editedParameter->GetID());
    //activeModifiable->SetParameterValue(, *variant);
  }
  //*/
  else
  {
    pgMan->SetDescription (wxT ("Page Manager :"), wxT ("Message test"));
  }

  delete variant;
  //*/
}

void ModifiableEditor::OnPropertyGridChanging (wxPropertyGridEvent& event)
{
  // Perform validation
  // TODO: perform each one
  csRef<iModifiableDescription> description = activeModifiable->GetDescription();
  const iModifiableParameter* param = description->GetParameterByIndex(event.GetProperty()->GetIndexInParent());
  const iModifiableConstraint* constraint = param->GetConstraint();

  if(constraint != nullptr) {
    // The better option, imho is to just rely on
    // dynamic binding to do the work of that switch
    // and simply call:
    if(!constraint->Validate(activeModifiable->GetParameterValue(param->GetID())))
    {
      event.Veto();
      return;
    }

      /*
    switch(constraint->GetType()) {
    case MODIFIABLE_CONSTRAINT_BOUNDED:
      break;

    case MODIFIABLE_CONSTRAINT_ENUM:
      break;

    case MODIFIABLE_CONSTRAINT_VFS_FILE:
      break;

    case MODIFIABLE_CONSTRAINT_VFS_DIR:
      break;

    case MODIFIABLE_CONSTRAINT_VFS_PATH:
      break;

    case MODIFIABLE_CONSTRAINT_TEXT_ENTRY:
      break;

    case MODIFIABLE_CONSTRAINT_TEXT_BLOB:
      break;

    case MODIFIABLE_CONSTRAINT_BITMASK:
      break;
    }
    //*/
  }
}

void ModifiableEditor::OnPropertyGridChanged (wxPropertyGridEvent& event)
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