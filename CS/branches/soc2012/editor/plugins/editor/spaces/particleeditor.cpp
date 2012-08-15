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
    EVT_SIZE  (                       CSPartEditSpace::Space::OnSize)
    EVT_BUTTON(idButtonAddEmitter,    CSPartEditSpace::Space::OnButtonAddEmitter)
    EVT_BUTTON(idButtonRemoveEmitter, CSPartEditSpace::Space::OnButtonRemoveEmitter)
    EVT_BUTTON(idButtonAddEffector,   CSPartEditSpace::Space::OnButtonAddEffector)
    EVT_BUTTON(idButtonAddEffector,   CSPartEditSpace::Space::OnButtonRemoveEffector)
  END_EVENT_TABLE()

  SCF_IMPLEMENT_FACTORY(CSPartEditSpace)
  
  CSPartEditSpace::CSPartEditSpace(iBase* parent) 
  : scfImplementationType(this, parent), object_reg(0) 
  {
    // Setup namespace-scoped pointer to editor, to be used by the static
    // event handler to reach the space
    editorSpace = this;
  }

  CSPartEditSpace::~CSPartEditSpace() 
  {
    delete mainEditor;
  }

  bool CSPartEditSpace::Initialize(iObjectRegistry* obj_reg, iEditor* editor, iSpaceFactory* fact, wxWindow* parent)
  {
    object_reg = obj_reg;
    this->editor = editor;
    factory = fact;
    window = new CSPartEditSpace::Space(this, parent);
    mainSizer = new wxBoxSizer(wxVERTICAL);

    // TODO: method to query the active context; also call here!!!
    // Load PS plugins
    csRef<iPluginManager> pluginManager = csQueryRegistry<iPluginManager> (object_reg);

    // Load translator and document system plugins
    emitterFactory = csLoadPlugin<iParticleBuiltinEmitterFactory>(pluginManager, "crystalspace.mesh.object.particles.emitter");

    effectorFactory = csLoadPlugin<iParticleBuiltinEffectorFactory>(pluginManager, "crystalspace.mesh.object.particles.effector");


    // "crystalspace.mesh.object.particles"
      

    // Setup the event names
    nameRegistry = csEventNameRegistry::GetRegistry (object_reg);
    //addObject = nameRegistry->GetID("crystalspace.editor.context.selection.addselectedobject");
    //clearObjects = nameRegistry->GetID("crystalspace.editor.context.selection.clearselectedobjects");
    activateObject = nameRegistry->GetID("crystalspace.editor.context.selection.setactiveobject");

    // Respond to context events
    //csEventID contextSelect = nameRegistry->GetID("crystalspace.editor.context");
    RegisterQueue (editor->GetContext()->GetEventQueue(), activateObject);
    
    // Prepare translations 
    translator = csQueryRegistry<iTranslator>(object_reg);
    cout << translator->GetMsg("Hello world") << endl;
    
    
    mainEditor = new ModifiableEditor(object_reg, window, idMainEditor, wxDefaultPosition, parent->GetSize(), 0L, wxT("Modifiable editor"));
    mainSizer->Add(mainEditor, 1, wxEXPAND | wxALL, borderWidth);
    window->SetSizer(mainSizer);
    mainSizer->SetSizeHints(window);

    middleSizer = new wxBoxSizer(wxHORIZONTAL);
    middleLSizer = new wxBoxSizer(wxVERTICAL);
    middleRSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->Add(middleSizer, 1, wxEXPAND, borderWidth );
    middleSizer->Add(middleLSizer, 1, wxEXPAND | wxALL, borderWidth);
    middleSizer->Add(middleRSizer, 1, wxEXPAND | wxALL, borderWidth);

    //-- Emitter GUI
    middleLSizer->Add(new wxStaticText(window, wxID_ANY, wxT("Emitters")));
    emitterList = new wxListBox (window,idEmitterList);
    middleLSizer->Add ( emitterList,
                        1,
                        wxALL | wxEXPAND,
                        borderWidth );

    wxButton* but = new wxButton(window, idButtonAddEmitter, wxT("Add"));
    but->SetSize(-1, 32);
    middleLSizer->Add ( but,
                        0,
                        wxALL | wxEXPAND,
                        borderWidth );

    but = new wxButton(window, idButtonRemoveEmitter, wxT("Remove"));
    but->SetSize(-1, 32);
    middleLSizer->Add ( but,
                        0,
                        wxALL | wxEXPAND,
                        borderWidth );

    //-- Effector GUI
    middleRSizer->Add(new wxStaticText(window, wxID_ANY, wxT("Effectors")));
    effectorList = new wxListBox(window, idEffectorList);
    middleRSizer->Add(effectorList,
                      1,
                      wxALL | wxEXPAND,
                      borderWidth);

    but = new wxButton(window, idButtonAddEffector, wxT("Add"));
    but->SetSize(-1, 32);
    middleRSizer->Add(but,
                      0,
                      wxALL | wxEXPAND,
                      borderWidth);

    but = new wxButton(window, idButtonRemoveEffector, wxT("Remove"));
    but->SetSize(-1, 32);
    middleRSizer->Add(but,
                      0,
                      wxALL | wxEXPAND,
                      borderWidth);

    secondaryEditor = new ModifiableEditor(object_reg, window, idSecondaryEditor,
      wxDefaultPosition, wxDefaultSize, 0L, wxT("Secondary editor"));

    mainSizer->Add(secondaryEditor, 1, wxALL | wxEXPAND, borderWidth);

    printf ("\nInitialized property editing panel!\n");

    // Populate with the current active object 
    Populate (); 

    return true;
  }

  void CSPartEditSpace::Update() {
  }

  void CSPartEditSpace::OnSize(wxSizeEvent& event)
  {
    //mainEditor->OnSize(event);
    mainSizer->SetDimension(0, 0, event.GetSize().GetWidth(), event.GetSize().GetHeight());
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

    //csRef<iParticleSystemBase> ipb = scfQueryInterface<iParticleSystemBase>(partSys);

    csRef<iModifiable> modifiable = scfQueryInterface<iModifiable>(fac->GetMeshObjectFactory()); 
    // csRef<iModifiable> modifiable = scfQueryInterface<iModifiable>(ipb->GetEmitter(0)); 
    if (!modifiable)
    {
      // NoModifiable();
      return;
    }
   
    mainEditor->SetModifiable(modifiable);
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
