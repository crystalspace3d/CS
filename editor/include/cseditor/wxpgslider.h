#ifndef WXPGSLIDER_H
#define WXPGSLIDER_H

#include "cssysdef.h"

// common wx stuff
#include <wx/slider.h>
#include <wx/wx.h>

// for wxPGEditor
#include "cseditor/wx/propgrid/propgrid.h"
#include "cseditor/wx/propgrid/propdev.h"
#include "cseditor/wx/propgrid/advprops.h"
#include "cseditor/wx/propgrid/editors.h"

// the right csVariant
// #include "apps/varedittest/pluginconfig.h"

//----------------- wxPGSliderEditor ---------------------

/**
 * wxSlider for wxPropertyGrid. Contains a value slider.
 */
class WXDLLIMPEXP_PG wxPGSliderEditor : public wxPGEditor 
{
#ifndef SWIG
  WX_PG_DECLARE_EDITOR_CLASS(wxPGSliderEditor)
#endif

public:
  wxPGSliderEditor (int p = 10000)
    : precision(p)
  {
  }

  ~wxPGSliderEditor ()
  {}

  // Macro for the CreateControls method stub
  wxPG_DECLARE_CREATECONTROLS

  void UpdateControl ( wxPGProperty* property, wxWindow* wnd) const;
  bool OnEvent ( wxPropertyGrid* propgrid, wxPGProperty* property, wxWindow* wnd, wxEvent& event) const;
  bool GetValueFromControl ( wxVariant& variant, wxPGProperty* property, wxWindow* wnd) const;
  void SetValueToUnspecified ( wxPGProperty* property, wxWindow* wnd) const;
  //void DrawValue ( wxDC& dc, const wxRect& rect, wxPGProperty* property, const wxString& text) const;

private:
  int precision;
};

#endif
