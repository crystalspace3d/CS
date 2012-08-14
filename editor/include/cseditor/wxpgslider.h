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
#include "apps/varedittest/pluginconfig.h"

//----------------- wxPGSliderEditor ---------------------

/**
 * Re-implemented wxSlider for wxPropertyGrid. Contains a value slider, as well as
 * a text-field for displaying the exact value.
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
  void DrawValue ( wxDC& dc, const wxRect& rect, wxPGProperty* property, const wxString& text) const;

private:
  int precision;
};

//----------------- wxPGSliderProperty ---------------------
/// The double property corresponding to the slider.
class wxPGSliderProperty : public wxPGProperty
{
  WX_PG_DECLARE_PROPERTY_CLASS(wxPGSliderProperty)

public:
  wxPGSliderProperty( const wxString& label = wxPG_LABEL,
                      const wxString& name  = wxPG_LABEL,
                      const double&   value = 0.0,
                      const int&      min   = 0,
                      const int&      max   = 10000 );

  virtual ~wxPGSliderProperty ();

  /// Initializes the property by creating it's controller, the slider.
  void Init ();

  /// Gets the property's corresponding slider.
  wxSlider* GetSlider () const;

  /// Returns the text control meant to show the actual value of the slider
  wxTextCtrl* GetLabel() const;

  /// Gets the wrapping panel
  wxPanel* GetPanel() const;

  /// Gets the property slider's bounding rectangle.
  wxRect GetRect (wxPropertyGrid* propgrid) const;

  wxString GetValueAsString ( int argflags ) const;
  wxVariant GetValue() const;

private:
  wxSlider* ctrl;
  wxTextCtrl* textCtrl;
  wxPanel* wrapper;
  wxSizer* wrapperSizer;

  int minVal, maxVal;
  static const int labelWidth = 32;

  // Not used at the moment
  void OnScroll(wxEvent& event);
};

#endif
