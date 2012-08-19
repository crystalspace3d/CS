/*
    Copyright (C) 2012 by Andrei Bârsan

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNUa
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

/// Helper method that extracts a csVariant from a wxPGProperty
csVariant* wxToCsVariant(const csVariantType& type, const wxVariant& original)
{
  csVariant* variant = new csVariant;
  if (type == CSVAR_STRING)
  {
    variant->SetString(original.GetString().mbc_str());
  }
  else if (type == CSVAR_LONG)
  {
    variant->SetLong(original.GetLong());    
  }
  else if (type == CSVAR_FLOAT)
  {
    variant->SetFloat (original.GetDouble ());
  }
  else if (type == CSVAR_BOOL)
  {
    variant->SetBool (original.GetBool ());
  }
  else if (type == CSVAR_COLOR)
  {
    wxColour col; col << original;
    variant->SetColor ( csColor ((float)col.Red ()/255, (float)col.Green ()/255, (float)col.Blue ()/255) );
  }
  else if (type == CSVAR_VECTOR3)
  {
    wxVector3f v3f; v3f << original;
    variant->SetVector3(csVector3(v3f.x, v3f.y, v3f.z));
  }
  else if (type == CSVAR_VECTOR4)
  {
    wxVector4f v4f; v4f << original;
    variant->SetVector4(csVector4(v4f.x, v4f.y, v4f.z, v4f.w));
  }
  else if (type == CSVAR_VECTOR2)
  {
    wxVector2f v2f; v2f << original;
    variant->SetVector2(csVector2(v2f.x, v2f.y));
  }
  else
  {
    CS_ASSERT_MSG("Failed to convert wxVariant to csVariant!!!", false);
    return nullptr;
  }

  return variant;
}

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
    CS_ASSERT_MSG("iModifiable object must return a value for each valid parameter id!", variant != nullptr);
    wxString originalName( param->GetName(), wxConvUTF8 );
    wxString translation( translator->GetMsg(param->GetName()), wxConvUTF8 );
    wxString description( param->GetDescription(), wxConvUTF8 );

    AppendVariant(root, variant, param->GetConstraint(), originalName, translation, description);

    delete variant;
  }
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
          const csConstraintEnum* ec = dynamic_cast<const csConstraintEnum*>(constraint);
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
        float min = 0, max = 100;
        if(constraint != nullptr && constraint->GetType() == MODIFIABLE_CONSTRAINT_BOUNDED) {
          // Set the slider limits
          const csConstraintBounded* bc = dynamic_cast<const csConstraintBounded*>(constraint);
          if(bc->HasMinimum()) min = bc->GetMinimum().GetFloat();
          if(bc->HasMaximum()) max = bc->GetMaximum().GetFloat();
        }
        wxPGEditor* rHandle = wxPropertyGrid::RegisterEditorClass(new wxPGSliderEditor(min, max), wxT("SliderEditor"));
        fp->SetEditor(rHandle);
        page->AppendIn(categoryID, fp);
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
  // nb: When this function is reached, the values are considered valid
  wxVariant newValue = property->GetValue ();
  if (newValue.IsNull () || activeModifiable == nullptr)
    return;

  size_t index = property->GetIndexInParent ();
  csRef<iModifiableDescription> desc = activeModifiable->GetDescription();
  // This FAILS with nested properties, such as in the case of arrays,
  // or nested iModifiables
  const iModifiableParameter* editedParameter = desc->GetParameterByIndex(index);
  csVariant *variant = wxToCsVariant(editedParameter->GetType(), newValue);
  activeModifiable->SetParameterValue(editedParameter->GetID(), *variant);

  delete variant;
}

void ModifiableEditor::OnPropertyGridChanging (wxPropertyGridEvent& event)
{
  // Perform validation
  wxVariant newValue = event.GetValue();
  
  csRef<iModifiableDescription> description = activeModifiable->GetDescription();
  const iModifiableParameter* param = description->GetParameterByIndex(event.GetProperty()->GetIndexInParent());
  const iModifiableConstraint* constraint = param->GetConstraint();

  if(constraint != nullptr) {
    csVariant* newCsValue = wxToCsVariant(param->GetType(), newValue);
    
    if( ! constraint->Validate(newCsValue))
    {
      delete newCsValue;
      event.Veto();
      return;
    }

    delete newCsValue;
  }
}

void ModifiableEditor::OnPropertyGridChanged (wxPropertyGridEvent& event)
{
  wxPGProperty* property = event.GetProperty ();
  OnGetNewValue (property);
}

void ModifiableEditor::SetMessage( const wxString& title, const wxString& message )
{
  pgMan->SetDescription(title, message);
}

void ModifiableEditor::Clear()
{
  pgMan->Clear();
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
