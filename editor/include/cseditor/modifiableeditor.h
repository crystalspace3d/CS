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

#ifndef __MODIFIABLE_EDITOR_H__
#define __MODIFIABLE_EDITOR_H__

// Misc
#include "cssysdef.h"
#include "csutil/ref.h"
#include <wx/wx.h>

// Translator
#include <ivaria/translator.h>

// Needed for the properties
#include <csutil/cscolor.h>
#include <csgeom/vector3.h>

// Property grid manager
#include "cseditor/wx/propgrid/manager.h"
#include "cseditor/wx/propgrid/advprops.h"
#include "cseditor/wx/propgrid/propdev.h"
#include "cseditor/wx/propgrid/editors.h"

// Modifiable
#include "iutil/modifiable.h"

// Editor stuff
#include "ieditor/editor.h"
#include "ieditor/context.h"
#include "ieditor/operator.h"
#include "ieditor/space.h"

// iObject
#include "iutil/object.h"

namespace CS {
namespace EditorApp {

class /*CS_CRYSTALSPACE_EXPORT*/ ModifiableEditor : public wxPanel 
{
public:
  ModifiableEditor  ( iObjectRegistry*  object_reg, 
                      wxWindow*         parent,
                      wxWindowID        id,
                      const wxPoint&    position,
                      const wxSize&     size,
                      long              style,
                      const wxString&   name 
                    );

  virtual ~ModifiableEditor() {}

  /// Sets the current active modifiable entity in the grid
  virtual void SetModifiable            (iModifiable* modifiable);
  /// Sets the current active modifiable, as well as its name
  virtual void SetModifiableLabel       (iModifiable* modifiable, csString& label);
  /// Gets the currently active modifiable entity in the grid
  virtual iModifiable* GetModifiable    () const;
  /// Gets called by the space owning this component
  virtual void OnSize                   (wxSizeEvent& event);
  /// Sets the property grid's message
  void SetMessage                       (const wxString& title, const wxString& message);
  // Clears all the data from the editor
  void Clear                            ();
    
  DECLARE_EVENT_TABLE();

private:
  /// Populates the property grid with the editable traits of the active modifiable object
  void Populate ();
  /// Appends the element to the property grid, in its own category
  void Append   (iModifiable* element, csString& name);

  /**
    * Actually does the work of taking the variant, its name, translation and translated
    * description and generating the corresponding GUI element for each of them.
    */
  void AppendVariant(wxPGPropArg categoryID, csVariant* variant, const iModifiableConstraint* param, const wxString& originalName, const wxString& translatedName, const wxString& translatedDescription);

  void OnGetNewValue            (wxPGProperty* property);
  void OnPropertyGridChanging   (wxPropertyGridEvent& event);
  void OnPropertyGridChanged    (wxPropertyGridEvent& event);

  enum
  {
    pageId = 1
  };

  iObjectRegistry*          object_reg;
  iModifiable*              activeModifiable;
  csString                  activeModifiableName;

  wxPropertyGridPage*       page;
  wxPropertyGridManager*    pgMan;

  csRef<iTranslator>        translator;
};

} // namespace EditorApp
} // namespace CS

#endif
