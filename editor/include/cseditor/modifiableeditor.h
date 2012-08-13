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
                      iEditor*          editor,
                      wxWindowID        id,
                      const wxPoint&    position,
                      const wxSize&     size,
                      long              style,
                      const wxString&   name 
                    );

    /// Sets the current active modifiable entity in the grid
    void SetModifiable            (iModifiable*);
    /// Gets the currently active modifiable entity in the grid
    iModifiable* GetModifiable    () const;
    /// Gets called by the space owning this component
    void OnSize                   (wxSizeEvent& event);
    
    DECLARE_EVENT_TABLE();

  private:
    /// Called to display a message that the current object cannot be edited
    void NoModifiable();
    /// Populates the property grid with the editable traits of the modifiable object
    void Populate ();

    void OnGetNewValue            (wxPGProperty* property);
    void OnPropertyGridChanging   (wxPropertyGridEvent& event);
    void OnPropertyGridChanged    (wxPropertyGridEvent& event);

    enum
    {
      pageId = 1
    };

    iObjectRegistry*          object_reg;
    iModifiable*              activeModifiable;

    // Needed for the property grid to display right
    // wxPropertyGridManager*  m_pPropGridManager;
    wxPropertyGridPage*       page;
    wxPropertyGridManager*    pgMan;

    csRef<iTranslator>        translator;
    iEditor*                  editor;
};

} // namespace EditorApp
} // namespace CS

#endif
