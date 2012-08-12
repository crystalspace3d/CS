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


#include "cssysdef.h"
#include "particleeditor.h"

// Needed for implementing custom properties.
// #include "apps/tools/editor/wx/propgrid/propdev.h"

#include "csutil/ref.h"

#include "cstool/initapp.h"
#include "csutil/scf.h"

#include "cstool/enginetools.h"
#include "csgeom/plane3.h"
#include "csgeom/math3d.h"
#include "csutil/event.h"
#include "csutil/sysfunc.h"
#include "cstool/csview.h"
#include "iutil/csinput.h"
#include "iutil/eventq.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/virtclk.h"
#include "iengine/campos.h"
#include "iengine/sector.h"
#include "iengine/scenenode.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"
#include "iengine/engine.h"
#include "iengine/camera.h"
#include "imesh/object.h"
#include "imesh/objmodel.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "ivideo/natwin.h"
#include "ivideo/wxwin.h"

// Translator
#include <ivaria/translator.h>

// More editor stuff
#include "ieditor/context.h"
#include "ieditor/operator.h"

// Particle stuff
#include "imesh/particles.h"

#include <wx/wx.h>

#include "wx/artprov.h"

CS_PLUGIN_NAMESPACE_BEGIN(CSEditor)
{
  CSPartEditSpace* editorSpace;

  BEGIN_EVENT_TABLE(CSPartEditSpace::Space, wxPanel)
    EVT_SIZE(CSPartEditSpace::Space::OnSize)
    EVT_BUTTON(idButtonAdd, CSPartEditSpace::Space::OnButtonAdd)
    EVT_BUTTON(idButtonRemove, CSPartEditSpace::Space::OnButtonRemove)
  END_EVENT_TABLE()

  SCF_IMPLEMENT_FACTORY(CSPartEditSpace)
  
  CSPartEditSpace::CSPartEditSpace(iBase* parent) 
  : scfImplementation1(this, parent), object_reg(0) 
  {
    // Setup namespace-scoped pointer to editor, to be used by the static
    // event handler to reach the space
    editorSpace = this;
  }

  CSPartEditSpace::~CSPartEditSpace() 
  {
    delete modifiableEditor;
  }

  bool CSPartEditSpace::Initialize(iObjectRegistry* obj_reg, iEditor* editor, iSpaceFactory* fact, wxWindow* parent)
  {
    object_reg = obj_reg;
    this->editor = editor;
    factory = fact;
    window = new CSPartEditSpace::Space(this, parent);
    mainsizer = new wxBoxSizer(wxVERTICAL);

    // TODO: method to query the active context; also call here!!!

    // Setup the event names
    nameRegistry = csEventNameRegistry::GetRegistry (object_reg);
    addObject = nameRegistry->GetID("crystalspace.editor.context.addselectedobject");
    clearObjects = nameRegistry->GetID("crystalspace.editor.context.clearselectedobjects");
    activateObject = nameRegistry->GetID("crystalspace.editor.context.setactiveobject");

    // Respond to context events
    //csEventID contextSelect = nameRegistry->GetID("crystalspace.editor.context");
    RegisterQueue (editor->GetContext()->GetEventQueue(), activateObject);
    
    // Prepare translations 
    translator = csQueryRegistry<iTranslator>(object_reg);
    cout << translator->GetMsg("Hello world!") << endl;
    
    
    modifiableEditor = new ModifiableEditor(window, editor, wxID_ANY, wxDefaultPosition, parent->GetSize(), 0L, wxT("Modifiable editor"));
    mainsizer->Add(modifiableEditor, 1, wxEXPAND | wxALL, borderWidth);
    window->SetSizer(mainsizer);
    mainsizer->SetSizeHints(window);

    wxButton* but = new wxButton(window, idButtonAdd, wxT("Add"));
    but->SetSize(-1, 32);
    mainsizer->Add(but,
      0,
      wxALL | wxEXPAND,
      borderWidth);

    but = new wxButton(window, idButtonRemove, wxT("Remove"));
    but->SetSize(-1, 32);
    mainsizer->Add(but,
      0,
      wxALL | wxEXPAND,
      borderWidth);

    printf ("\nInitialized property editing panel!\n");
    return true;
  }

  void CSPartEditSpace::Update() {
  }

  void CSPartEditSpace::OnSize(wxSizeEvent& event)
  {
    //modifiableEditor->OnSize(event);
    mainsizer->SetDimension(0, 0, event.GetSize().GetWidth(), event.GetSize().GetHeight());
    event.Skip();
  }

  void CSPartEditSpace::Populate()
  {
    csRef<iContextObjectSelection> objectSelectionContext =
      scfQueryInterface<iContextObjectSelection> (editor->GetContext ());

    // Search for the iModifiable interface of the particle factory
    iObject* result = objectSelectionContext->GetActiveObject ();
    if (!result)
    {
      //NoModifiable();
      return;
    }

    csRef<iMeshFactoryWrapper> fac = scfQueryInterface<iMeshFactoryWrapper>(result);
    if (!fac)
    {
      //NoModifiable();
      return;
    }

    csRef<iParticleSystemFactory> partSys =
      scfQueryInterface<iParticleSystemFactory>(fac->GetMeshObjectFactory()); 
    if (!partSys)
    {
      //NoModifiable();
      return;
    }

    printf("It is a particle system factory!\n");

    csRef<iModifiable> modifiable = scfQueryInterface<iModifiable>(fac->GetMeshObjectFactory()); 
    if (!modifiable)
    {
      // NoModifiable();
      return;
    }
   
    modifiableEditor->SetModifiable(modifiable);
  }

  wxWindow* CSPartEditSpace::GetwxWindow ()
  {
    return window;
  }

  bool CSPartEditSpace::GetEnabled() const {
    return true;
  }

  void CSPartEditSpace::SetEnabled(bool enabled) {
    // todo
  }

  bool CSPartEditSpace::HandleEvent (iEvent& event)
  {
    csRef<iEventNameRegistry> strings = csQueryRegistry<iEventNameRegistry>( object_reg );

    printf("\tCaught event: %s (ID #%u)\n",
      strings->GetString( event.GetName() ),
      (unsigned int) event.GetName() );

    if (event.Name == activateObject) {
      printf("You selected something!\n");
      Populate();
    }

    return false;
  }
}
CS_PLUGIN_NAMESPACE_END(CSEditor)