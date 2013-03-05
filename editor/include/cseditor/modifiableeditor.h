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
#ifndef __CSEDITOR_MODIFIABLE_EDITOR_H__
#define __CSEDITOR_MODIFIABLE_EDITOR_H__

// Misc
#include "cssysdef.h"
#include "csutil/hash.h"
#include "csutil/ref.h"

// Property grid manager
#include "cseditor/wx/propgrid/manager.h"
#include "cseditor/wx/propgrid/advprops.h"
#include "cseditor/wx/propgrid/propdev.h"
#include "cseditor/wx/propgrid/editors.h"

class csVariant;
struct iModifiable;
struct iModifiableConstraint;
struct iModifiableDescription;
struct iTranslator;

namespace CS {
namespace EditorApp {

class /*CS_CRYSTALSPACE_EXPORT*/ ModifiableEditor : public wxPanel 
{
public:
  /// Constructor
  ModifiableEditor (iObjectRegistry* object_reg, 
		    wxWindow* parent,
		    wxWindowID id,
		    const wxPoint& position,
		    const wxSize& size,
		    long style,
		    const wxString& name);

  /// Sets the current active modifiable entity in the grid
  virtual void SetModifiable (iModifiable* modifiable);
  /// Gets the currently active modifiable entity in the grid
  virtual iModifiable* GetModifiable () const;
  /// Sets the property grid's message
  void SetMessage (const wxString& title, const wxString& message);
  // Clears all the data from the editor
  void Clear ();

private:
  /// Gets called by the space owning this component
  virtual void OnSize (wxSizeEvent& event);

  /// Appends the element to the property grid, in its own category
  void Append (wxPropertyCategory* category, const iModifiableDescription* description, size_t offset);

  /**
   * Actually does the work of taking the variant, its name, translation and translated
   * description and generating the corresponding GUI element for each of them.
   */
  void AppendVariant (wxPGPropArg categoryID, csVariant* variant, size_t index,
		      const iModifiableConstraint* param,
		      const wxString& originalName, const wxString& translatedName,
		      const wxString& translatedDescription);

  void OnPropertyGridChanging (wxPropertyGridEvent& event);
  void OnPropertyGridChanged (wxPropertyGridEvent& event);

  enum
  {
    pageId = 1
  };

  iObjectRegistry* object_reg;
  csRef<iModifiable> modifiable;
  csRef<iModifiableDescription> description;

  wxPropertyGridPage* page;
  wxPropertyGridManager* pgMananager;

  csHash<size_t, wxPGProperty*> indexes;

  csRef<iTranslator> translator;

  DECLARE_EVENT_TABLE ();
};

} // namespace EditorApp
} // namespace CS

#endif
