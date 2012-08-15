#include "cseditor/wxpgslider.h"

//----------------- wxPGSliderEditor ---------------------

WX_PG_IMPLEMENT_EDITOR_CLASS(SliderEditor, wxPGSliderEditor, wxPGEditor)

wxPGWindowList wxPGSliderEditor::CreateControls( wxPropertyGrid*  propgrid,
                                                 wxPGProperty*    property,
                                                 const wxPoint&   pos,
                                                 const wxSize&    size ) const
{
  double v_d = property->GetValue().GetDouble();
  if ( v_d < 0 )
    v_d = 0;
  else if ( v_d > 1 )
    v_d = 1;

  wxSlider *ctrl = new wxSlider();

#ifdef __WXMSW__
  ctrl->Hide();
#endif

  ctrl->Create ( propgrid->GetPanel(),
                 wxPG_SUBID2,
                 (int)(v_d * precision),
                 0,
                 precision,
                 pos,
                 size,
                 wxSL_HORIZONTAL );

  return wxPGWindowList(ctrl);
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

      //static_cast<wxPGSliderProperty*>(property)->GetLabel()
      //  ->SetValue( wxString::Format(wxT("%ld"), val * precision) );
    }
  }
}

bool wxPGSliderEditor::OnEvent ( wxPropertyGrid*  propgrid, 
                                 wxPGProperty*    property,
                                 wxWindow*        wnd,
                                 wxEvent&         event ) const
{
  if(event.GetEventType() == wxEVT_SCROLL_CHANGED)
  {
    // Update the value    
    event.Skip();
    propgrid->EditorsValueWasModified();

    return true;
  }
  
  return false;
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
  wxSlider* ctrl = wxDynamicCast ( wnd, wxSlider );
  if ( ctrl )
  {
    ctrl->SetValue (0);
  }
}

/*
void wxPGSliderEditor::DrawValue ( wxDC&           WXUNUSED(dc),
                                   const wxRect&   WXUNUSED(rect),
                                   wxPGProperty*   property,
                                   const wxString& WXUNUSED(text) ) const
{
  if( ! property->IsVisible())
    return;

  if( ! ctrl->IsShown())
    ctrl->Show();
}//*/