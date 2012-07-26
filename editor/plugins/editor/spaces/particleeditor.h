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

#ifndef __SPACE_PARTICLEEDITOR_H__
#define __SPACE_PARTICLEEDITOR_H__

#include "cstool/collider.h"
#include "csutil/csbaseeventh.h"
#include "csutil/eventnames.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "ieditor/editor.h"
#include "ieditor/space.h"
#include "iutil/event.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "ivaria/cameramanager.h"


#ifndef WX_PRECOMP
  #include <wx/wx.h>
  #include <iostream>
#endif

// TODO: maybe relocate slider code
// TODO: sort the fustercluck of includes

#include "include/cseditor/wxpgslider.h"
#include <csutil/refarr.h>
#include "csutil/ref.h"

// Contains iWxWindow
#include "ivideo/wxwin.h"

// iGraphics3D
#include "ivideo/graph3d.h"

// iView
#include "ivaria/view.h"

#include <csutil/weakref.h>

// Main propertygrid header.
// Verbose paths are in place until the jamfiles are updated
// to generate msvc project files with the proper include dirs
#include "apps/tools/editor/wx/propgrid/propgrid.h"
#include "apps/tools/editor/wx/propgrid/editors.h"

// Needed for implementing custom properties.
#include "apps/tools/editor/wx/propgrid/propdev.h"
#include <wx/panel.h>

// Extra property classes.
#include "apps/tools/editor/wx/propgrid/advprops.h"

// This defines wxPropertyGridManager.
#include "apps/tools/editor/wx/propgrid/manager.h"

// For the gist - displaying modifiable stuff in a GUI
#include "iutil/modifiable.h"

#include "ivaria/translator.h"

#include <stdarg.h>
#include "csutil/array.h"
#include "csutil/csstring.h"
#include <iutil/event.h>
#include <iutil/objreg.h>
#include <wx/variant.h>
#include <string>

#include <wx/dnd.h>
#include <wx/event.h>

CS_PLUGIN_NAMESPACE_BEGIN(CSEditor)
{
  using namespace CS::EditorApp;
  using namespace std;

  class CSPartEditSpace : public scfImplementation1<CSPartEditSpace, iSpace>, public csBaseEventHandler
  {
  public:
    CSPartEditSpace(iBase* parent);
    virtual ~CSPartEditSpace();

    // iSpace
    virtual bool Initialize (iObjectRegistry* obj_reg, iEditor* editor, iSpaceFactory* fact, wxWindow* parent);
    virtual iSpaceFactory* GetFactory () const { return factory; }
    virtual wxWindow* GetwxWindow ();
    virtual void SetEnabled (bool enabled);
    virtual bool GetEnabled () const;
    virtual void Update ();
    
    // iEventHandler 
    bool HandleEvent(iEvent &event);

    void OnSize (wxSizeEvent& event);
    void OnGetNewValue (wxPGProperty* property);
    void OnPropertyGridChanging(wxPropertyGridEvent& event);

  protected:
    enum
    {
      pageId = 1
    };

  private:
    /// Populates the property grid with the editable traits of the modifiable object
    void Populate(iModifiable* modifiable);

    /// Called to display a message that the current object cannot be edited
    void NoModifiable();

    iObjectRegistry*      object_reg;
    csRef<iSpaceFactory>  factory;
    wxWindow*             window;
    iModifiable*          activeModifiable;

    // Needed for the property grid to display right
    // wxPropertyGridManager*  m_pPropGridManager;
    wxPropertyGridPage*     page;
    wxPropertyGridManager*  pgMan;
    wxSizer*                mainsizer;

    // Main ref to the editor
    csRef<iEditor> editor;

    csRef<iEngine>      engine;
    csRef<iGraphics3D>  g3d;
    csRef<iEventQueue>  queue;
    csRef<iView>        view;
    csRef<iTranslator>  translator;

    // Various event ids
    iEventNameRegistry* nameRegistry;
    csStringID          addObject;
    csStringID          clearObjects;
    csStringID          activateObject;
    

    /// Panel where all the dials and knobs for editing things are placed
    class Space : public wxPanel
    {
    public:
      Space(CSPartEditSpace* p, wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize)
        : wxPanel (parent, id, pos, size),
          space(p)
      {}

      virtual void OnSize (wxSizeEvent& ev) {
        if (space) space->OnSize (ev);
      }
      void OnPropertyGridChanging (wxPropertyGridEvent& event) {
        if (space) space->OnPropertyGridChanging(event);
      }

    private:
      CSPartEditSpace* space;

      DECLARE_EVENT_TABLE()
    };
  };


  

}
CS_PLUGIN_NAMESPACE_END(CSEditor)


//----------------- Custom properties for the property grid ---------------------
// Note: I've moved them to the global namespace to allow wx to 'see' them, because otherwise I 
// was getting a lot of compilation errors. (CS_PLUGIN_NAMESPACE wasn't helping)
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

#endif