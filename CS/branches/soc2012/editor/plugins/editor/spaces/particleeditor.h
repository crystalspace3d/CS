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

//#include "cstool/collider.h"
#include "csutil/csbaseeventh.h"
#include "csutil/eventnames.h"
#include "csutil/scf_implementation.h"
//#include "csutil/weakref.h"
#include "ieditor/editor.h"
#include "ieditor/space.h"
#include "iutil/event.h"
//#include "iutil/eventh.h"
#include "iutil/comp.h"
//#include "ivaria/cameramanager.h"

#ifndef WX_PRECOMP
  #include <wx/wx.h>
  #include <iostream>
#endif

// TODO: maybe relocate slider code
// TODO: sort the fustercluck of includes

#include <csutil/refarr.h>
#include "csutil/ref.h"

// Contains iWxWindow
#include "ivideo/wxwin.h"

// iGraphics3D
#include "ivideo/graph3d.h"

#include <csutil/weakref.h>

// For the gist - displaying modifiable stuff in a GUI
#include "iutil/modifiable.h"

// The particle stuff
#include <imesh/particles.h>

#include "ivaria/translator.h"

// TODO: maybe move this file
#include "libs/cseditor/modifiableeditor.h"

#include <stdarg.h>
#include "csutil/array.h"
#include "csutil/csstring.h"
#include <iutil/event.h>
#include <iutil/objreg.h>
#include <string>

#include <wx/dnd.h>
#include <wx/event.h>

CS_PLUGIN_NAMESPACE_BEGIN(CSEditor)
{
  using namespace CS::EditorApp;
  using namespace std;

  // TODO: move all the propgrid stuff to the modifiable editor,
  // and just keep a ref to that modifiable editor
  class CSPartEditSpace : public csBaseEventHandler, public scfImplementation1<CSPartEditSpace, iSpace>
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

    /**
     * Checks the current editor context and, if available, gets the selected
     * iModifiable, passing it to the modifiable editor and updating the rest of
     * the GUI.
     */
    void Populate();

    // Various event ids
    iEventNameRegistry* nameRegistry;
    csStringID          addObject;
    csStringID          clearObjects;
    csStringID          activateObject;

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

      void OnButtonAdd(wxCommandEvent &event) {
        // it's ok to hardcouple a link with the PS here
        csRef<iParticleSystemFactory> ps(scfQueryInterface<iParticleSystemFactory>(space->modifiableEditor->GetModifiable()));
        CS_ASSERT_MSG("This should be safe", ps.IsValid());

        csRef<iParticleBuiltinEmitterFactory> factory(csQueryRegistry<iParticleBuiltinEmitterFactory>(space->object_reg));
        csRef<iParticleBuiltinEmitterBox> em(factory->CreateBox());
        ps->AddEmitter(em);
        // TODO: update ps here
      }

      void OnButtonRemove(wxCommandEvent &event) {

      }

    private:
      CSPartEditSpace* space;

      DECLARE_EVENT_TABLE()
    };

  private:
    static const int borderWidth = 4; 

    iObjectRegistry*      object_reg;
    csRef<iSpaceFactory>  factory;
    wxWindow*             window;

    // TODO: actually use this
    wxBoxSizer*           mainsizer;

    // Main ref to the editor
    csRef<iEditor> editor;
    csRef<iEventQueue>  queue;
    csRef<iTranslator>  translator;

    // Ref to the modifiable editor holding the propgrid
    ModifiableEditor*   modifiableEditor;

      enum {
        idButtonAdd = 42,
        idButtonRemove
    };
  };
}
CS_PLUGIN_NAMESPACE_END(CSEditor)

#endif
