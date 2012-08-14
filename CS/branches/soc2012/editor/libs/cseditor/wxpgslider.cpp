#include "cseditor/wxpgslider.h"

//----------------- wxPGSliderEditor ---------------------

WX_PG_IMPLEMENT_EDITOR_CLASS(SliderEditor, wxPGSliderEditor, wxPGEditor)

wxPGWindowList wxPGSliderEditor::CreateControls( wxPropertyGrid*  propgrid,
                                                 wxPGProperty*    property,
                                                 const wxPoint&   pos,
                                                 const wxSize&    size ) const
{
  wxSlider* slider = ((wxPGSliderProperty*) property)->GetSlider ();
  //((wxWindow*) slider)->Hide ();

  double v_d = property->GetValue().GetDouble();
  if ( v_d < 0 )
    v_d = 0;
  else if ( v_d > 1 )
    v_d = 1;

  // TODO: find why it's being re-created
  /*
  wxSlider* ctrl = new wxSlider ();
  ctrl->Create ( propgrid->GetPanel(),
                 wxPG_SUBID1,
                 (int)(v_d * precision),
                 0,
                 precision,
                 pos,
                 size,
                 wxSL_HORIZONTAL );
                 //*/

  // Setup event handler
  propgrid->Connect ( wxPG_SUBID1, wxEVT_SCROLL_THUMBTRACK, 
                      (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction)
                      &wxPropertyGrid::OnCustomEditorEvent, NULL, propgrid );

  //return ctrl;
   return slider;
}

void wxPGSliderEditor::UpdateControl ( wxPGProperty* property, wxWindow* wnd ) const
{
  wxSlider* ctrl = wxDynamicCast ( wnd, wxSlider );
  if ( ctrl )
  {
    double val;
    if (wxPGVariantToDouble (property->DoGetValue(), &val))
    {
      if ( val < 0 )
        val = 0;
      else if ( val > 1 )
        val = 1;
      ctrl->SetValue ( (int)(val * precision) );

      static_cast<wxPGSliderProperty*>(property)->GetLabel()
        ->SetValue( wxString::Format(wxT("%ld"), val * precision) );
    }
  }
}

bool wxPGSliderEditor::OnEvent ( wxPropertyGrid*  WXUNUSED(propgrid), 
                                 wxPGProperty*    WXUNUSED(property),
                                 wxWindow*        WXUNUSED(wnd),
                                 wxEvent&         event ) const
{
  return event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK;
}

bool wxPGSliderEditor::GetValueFromControl ( wxVariant&     variant,
                                             wxPGProperty*  property,
                                             wxWindow*      wnd ) const
{
  wxSlider* ctrl = wxDynamicCast ( wnd, wxSlider );
  if ( ctrl )
  {
    variant = wxVariant ( (double)(ctrl->GetValue())/(double)(precision) );
    property->SetValue ( variant );
  }

  return true;
}

void wxPGSliderEditor::SetValueToUnspecified ( wxPGProperty* property, wxWindow* wnd) const
{
  // TODO
  wxSlider* ctrl = wxDynamicCast ( wnd, wxSlider );
  if ( ctrl ) {
    ctrl->SetValue (0);
  }
}

void wxPGSliderEditor::DrawValue ( wxDC&           dc,
                                   const wxRect&   rect,
                                   wxPGProperty*   property,
                                   const wxString& WXUNUSED(text) ) const
{
  wxSlider* slider = ( (wxPGSliderProperty*)property )->GetSlider();
  int curr = (int) ((double) property->GetValue() * precision);
  slider-> SetValue (curr);

  wxPanel* pan = static_cast<wxPGSliderProperty*>(property)->GetPanel();
  if( !pan->IsShown()) {
    printf("Panel wasn't shown... Odd...\n");
    pan->Show();
  }

  wxWindow* win = (wxWindow*) slider;
  if (!win-> IsShown ())
    win-> Show ();

  wxTextCtrl* textCtrl = ( static_cast<wxPGSliderProperty*>(property) )->GetLabel();
  if (!textCtrl->IsShown() )
    textCtrl->Show();
}

//----------------- wxPGSliderProperty ---------------------

WX_PG_IMPLEMENT_PROPERTY_CLASS( wxPGSliderProperty, wxPGProperty,
                                double, const double&, SliderEditor )

wxPGSliderProperty::wxPGSliderProperty ( const wxString&  label,
                                         const wxString&  name,
                                         const double&    value,
                                         const int&       min,
                                         const int&       max )
  : wxPGProperty  ( label, name ),
    ctrl          (NULL),
    minVal        (min),
    maxVal        (max)
{
  SetValue ( wxVariant(value) );

  // Calls DoRegisterEditorClass; mandatory for custom properties
  wxPGEditor* editorPointer = wxPropertyGrid::RegisterEditorClass( new wxPGSliderEditor (max), wxT("SliderEditor") );
  /*
  wxPropertyGrid* grid = GetGrid();
  wxRect rect = GetRect(grid);
  editorPointer->CreateControls(grid, this, wxPoint(rect.x, rect.y), rect.GetSize());
  */
  SetEditor ( editorPointer );
}

wxPGSliderProperty::~wxPGSliderProperty () 
{}

void wxPGSliderProperty::Init ()
{
  wxPropertyGrid* propgrid = GetGrid();
  wxRect rect = GetRect(propgrid);

  wrapper = new wxPanel(propgrid->GetPanel(), 
                        rect.x,
                        rect.y,
                        rect.width,
                        rect.height
                        );

  wrapperSizer = new wxBoxSizer(wxHORIZONTAL);
  wrapperSizer->SetDimension(0, 0, rect.width, rect.height);
  wrapper->SetSizer(wrapperSizer);
  
  ctrl = new wxSlider();
  double v_d = GetValue().GetDouble();
  ctrl->Create( wrapper,
                wxPG_SUBID1,
                (int)(v_d * maxVal),
                minVal,
                maxVal,
                wxPoint(rect.width / 2 + 1, -1),
                wxDefaultSize
                );
  wrapperSizer->Add(ctrl, 1, wxALL, 4);

  textCtrl = new wxTextCtrl();
  textCtrl->Create( wrapper,
                    wxPG_SUBID1,
                    wxString::Format(wxT("%ld"), v_d),
                    wxDefaultPosition,
                    wxDefaultSize);
  wrapperSizer->Add(textCtrl, 1, wxALL, 4);

  GetEditorClass()->CreateControls(propgrid, this, rect.GetPosition(), rect.GetSize());
  // Read-only for now
  // Afterwards, setup event handlers so that each edit (slider, text) triggers
  // the update of the other representation.
  textCtrl->SetEditable(false);
}

wxSlider* wxPGSliderProperty::GetSlider () const
{
  return ctrl;
}

wxTextCtrl* wxPGSliderProperty::GetLabel () const 
{
  return textCtrl;
}

wxPanel* wxPGSliderProperty::GetPanel() const 
{
  return wrapper;
}

wxRect wxPGSliderProperty::GetRect (wxPropertyGrid* propgrid) const
{
  wxRect rect = propgrid->GetPropertyRect(  dynamic_cast<const wxPGProperty*>(this), 
                                            dynamic_cast<const wxPGProperty*>(this) );
  wxSize size = rect.GetSize();
  wxPoint pos = rect.GetPosition();
  int splitterPos = propgrid->GetSplitterPosition() + 2;
  pos.x += splitterPos;
  size.x -= splitterPos;
  size.y -= 1;

  return wxRect (pos, size);
}

wxString wxPGSliderProperty::GetValueAsString ( int argflags ) const
{
  return wxString::Format(wxT("%ld"), GetSlider()->GetValue() * maxVal);
}

wxVariant wxPGSliderProperty::GetValue() const {
  return wxVariant(GetSlider()->GetValue() * maxVal);
}

void wxPGSliderProperty::OnScroll(wxEvent& event)
{

}
