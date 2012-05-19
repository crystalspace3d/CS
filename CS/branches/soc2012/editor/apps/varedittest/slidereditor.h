// http://wxformbuilder.sourcearchive.com/documentation/3.0.57/objinspect_8cpp-source.html

///////////////////////////////////////////////////////////////////////////////
//
// wxFormBuilder - A Visual Dialog Editor for wxWidgets.
// Copyright (C) 2005 José Antonio Hurtado
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// Written by
//   José Antonio Hurtado - joseantonio.hurtado@gmail.com
//   Juan Antonio Ortega  - jortegalalmolda@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

/*#include "objinspect.h"
  #include "model/objectbase.h"
  #include "utils/debug.h"
  #include "utils/typeconv.h"
  #include "utils/wxfbdefs.h"
  #include "rad/bitmaps.h"
  #include "rad/wxfbevent.h"
  #include "rad/appdata.h"
  #include "model/objectbase.h"

#include <wx/tokenzr.h>
#include <wx/config.h>*/

/*#define WXFB_PROPERTY_GRID 1000
  #define WXFB_EVENT_GRID    1001*/

// -----------------------------------------------------------------------
// wxSlider-based property editor
// -----------------------------------------------------------------------

//
// Implement an editor control that allows using wxSlider to edit value of
// wxFloatProperty (and similar).
//
// Note that new editor classes needs to be registered before use.
// This can be accomplished using wxPGRegisterEditorClass macro.
// Registeration can also be performed in a constructor of a
// property that is likely to require the editor in question.
//
#include "cssysdef.h"
#include <wx/slider.h>
#include <wx/wx.h>
#include "wx/propgrid/propgrid.h"
#include "wx/propgrid/propdev.h"
#include "wx/propgrid/advprops.h"
#include "wx/propgrid/editors.h"


class WXDLLIMPEXP_PG wxPGSliderEditor : public wxPGEditor
{
#ifndef SWIG
  WX_PG_DECLARE_EDITOR_CLASS(wxPGSliderEditor)
#endif

private:
  int precision;

public:
  wxPGSliderEditor (int p = 10000)
    : precision (p)
  {}

  ~wxPGSliderEditor ()
  {}

  wxPG_DECLARE_CREATECONTROLS

  void UpdateControl ( wxPGProperty* property, wxWindow* wnd ) const;
  bool OnEvent ( wxPropertyGrid* propgrid, wxPGProperty* property,
		 wxWindow* wnd, wxEvent& event ) const;
  bool GetValueFromControl ( wxVariant& variant, wxPGProperty* property, wxWindow* wnd ) const;
  void SetValueToUnspecified ( wxPGProperty* property, wxWindow* wnd ) const;
  void DrawValue( wxDC& dc, const wxRect& rect, wxPGProperty* property, const wxString& text ) const;
};


// ******************************************************************

class wxPGSliderProperty : public wxPGProperty
{
  WX_PG_DECLARE_PROPERTY_CLASS(wxPGSliderProperty)
public:

  wxPGSliderProperty( const wxString& label = wxPG_LABEL,
		      const wxString& name = wxPG_LABEL,
		      const double& value = 0.0,
		      const int& min = 0,
		      const int& max = 10000 );

  virtual ~wxPGSliderProperty();

  void Init ();
  wxSlider* GetSlider () const;
  wxRect GetRect (wxPropertyGrid* propgrid) const;

private:
  wxSlider* ctrl;
  int minVal, maxVal;
};
