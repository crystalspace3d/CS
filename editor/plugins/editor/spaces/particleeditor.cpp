/*
    Copyright (C) 2012 by Andrei B�rsan

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

  /// Helper function; apparently this functionality isn't built into wx
  bool ListBoxHasSelection(wxListBox* listBox) {
    wxArrayInt sel;
    listBox->GetSelections(sel);
    return sel.size() != 0;
  }

  // This table triggers a bunch of MFP conversion warnings, but due to
  // the fact that the wxPanel is the first class being inherited, there
  // shouldn't be any problems.
  BEGIN_EVENT_TABLE(CSPartEditSpace, wxPanel)
    EVT_SIZE  (                       CSPartEditSpace::OnSize)
    EVT_BUTTON(idButtonAddEmitter,    CSPartEditSpace::OnButtonAddEmitter)
    EVT_BUTTON(idButtonRemoveEmitter, CSPartEditSpace::OnButtonRemoveEmitter)
    EVT_BUTTON(idButtonAddEffector,   CSPartEditSpace::OnButtonAddEffector)
    EVT_BUTTON(idButtonRemoveEffector,CSPartEditSpace::OnButtonRemoveEffector)
    EVT_LISTBOX(idEmitterList,        CSPartEditSpace::OnEmitterSelect)
    EVT_LISTBOX(idEffectorList,       CSPartEditSpace::OnEffectorSelect)
  END_EVENT_TABLE()

  SCF_IMPLEMENT_FACTORY(CSPartEditSpace)
  
  CSPartEditSpace::CSPartEditSpace(iBase* parent) 
  : scfImplementationType(this, parent),
    object_reg(0) 
  {
    // Setup namespace-scoped pointer to editor, to be used by the static
    // event handler to reach the space
    editorSpace = this;
  }

  CSPartEditSpace::~CSPartEditSpace() 
  {
    delete mainEditor;
    delete secondaryEditor;
  }

  bool CSPartEditSpace::Initialize(iObjectRegistry* obj_reg, iEditor* editor, iSpaceFactory* fact, wxWindow* parent)
  {
    object_reg = obj_reg;
    this->editor = editor;
    spaceFactory = fact;
    mainSizer = new wxBoxSizer(wxVERTICAL);

    // Initializes the wxPanel part of the space
    Create(parent);

    // TODO: method to query the active context; also call here!!!
    // Load PS plugins
    csRef<iPluginManager> pluginManager = csQueryRegistry<iPluginManager> (object_reg);

    // Load translator and document system plugins
    emitterFactory = csLoadPlugin<iParticleBuiltinEmitterFactory>(pluginManager, "crystalspace.mesh.object.particles.emitter");

    effectorFactory = csLoadPlugin<iParticleBuiltinEffectorFactory>(pluginManager, "crystalspace.mesh.object.particles.effector");      

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
    
    // Prepare modifiable editors
    mainEditor = new ModifiableEditor(object_reg, this, idMainEditor, wxDefaultPosition, parent->GetSize(), 0L, wxT("Modifiable editor"));
    mainSizer->Add(mainEditor, 1, wxEXPAND | wxALL, borderWidth);
    SetSizer(mainSizer);
    mainSizer->SetSizeHints(this);

    middleSizer = new wxBoxSizer(wxHORIZONTAL);
    middleLSizer = new wxBoxSizer(wxVERTICAL);
    middleRSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->Add(middleSizer, 1, wxEXPAND, borderWidth );
    middleSizer->Add(middleLSizer, 1, wxEXPAND | wxALL, borderWidth);
    middleSizer->Add(middleRSizer, 1, wxEXPAND | wxALL, borderWidth);

    //-- Emitter GUI
    middleLSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Emitters")));
    emitterList = new wxListBox (this,idEmitterList);
    middleLSizer->Add ( emitterList,
                        1,
                        wxALL | wxEXPAND,
                        borderWidth );

    wxButton* but = new wxButton(this, idButtonAddEmitter, wxT("Add"));
    but->SetSize(-1, 32);
    middleLSizer->Add ( but,
                        0,
                        wxALL | wxEXPAND,
                        borderWidth );

    but = new wxButton(this, idButtonRemoveEmitter, wxT("Remove"));
    but->SetSize(-1, 32);
    middleLSizer->Add ( but,
                        0,
                        wxALL | wxEXPAND,
                        borderWidth );

    //-- Effector GUI
    middleRSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Effectors")));
    effectorList = new wxListBox(this, idEffectorList);
    middleRSizer->Add(effectorList,
                      1,
                      wxALL | wxEXPAND,
                      borderWidth);

    but = new wxButton(this, idButtonAddEffector, wxT("Add"));
    but->SetSize(-1, 32);
    middleRSizer->Add(but,
                      0,
                      wxALL | wxEXPAND,
                      borderWidth);

    but = new wxButton(this, idButtonRemoveEffector, wxT("Remove"));
    but->SetSize(-1, 32);
    middleRSizer->Add(but,
                      0,
                      wxALL | wxEXPAND,
                      borderWidth);

    secondaryEditor = new ModifiableEditor(object_reg, this, idSecondaryEditor,
      wxDefaultPosition, wxDefaultSize, 0L, wxT("Secondary editor"));

    mainSizer->Add(secondaryEditor, 1, wxALL | wxEXPAND, borderWidth);

    printf ("\nInitialized particle editing panel!\n");

    // Populate with the current active object 
    Populate (); 

    return true;
  }

  void CSPartEditSpace::Update() {
  }

  void CSPartEditSpace::OnSize(wxSizeEvent& event)
  {
    mainSizer->SetDimension(0, 0, event.GetSize().GetWidth(), event.GetSize().GetHeight());
    event.Skip();
  }

  void CSPartEditSpace::Populate()
  {
    // Get the object from the context
    csRef<iContextObjectSelection> objectSelectionContext =
      scfQueryInterface<iContextObjectSelection> (editor->GetContext ());

    // Search for the iModifiable interface of the particle factory
    iObject* result = objectSelectionContext->GetActiveObject ();
    if (!result)
    {
      //NoModifiable();
      return;
    }

    csString entityName(result->GetName());

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

    csRef<iModifiable> modifiable = scfQueryInterface<iModifiable>(fac->GetMeshObjectFactory());  
    if (!modifiable)
    {
      // NoModifiable();
      return;
    }

    // Caches a casted pointer to the factory
    factory = partSys;
   
    // Updates the GUI
    mainEditor->SetModifiableLabel(modifiable, entityName);
    UpdateEmitterList();
    UpdateEffectorList();
  }

  wxWindow* CSPartEditSpace::GetwxWindow()
  {
    return this;
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

#ifdef CS_DEBUG
    printf("\tCaught event: %s (ID #%u)\n",
      strings->GetString( event.GetName() ),
      (unsigned int) event.GetName() );
#endif

    if (event.Name == activateObject) {
      // The user activated (double-clicked) something!
      Populate();
    }

    return false;
  }

  void CSPartEditSpace::OnButtonAddEmitter( wxCommandEvent &event )
  {
    // TODO: context menu to pick
    csRef<iParticleBuiltinEmitterBox> em = emitterFactory->CreateBox();
    factory->AddEmitter(em);
    UpdateEmitterList();
  }

  void CSPartEditSpace::OnButtonRemoveEmitter( wxCommandEvent &event )
  {
    if(!ListBoxHasSelection(emitterList)) return;

    factory->RemoveEmitter(event.GetSelection());
    UpdateEmitterList();
  }

  void CSPartEditSpace::OnButtonAddEffector( wxCommandEvent &event )
  {
    // TODO: context menu to pick
    csRef<iParticleBuiltinEffectorForce> eff = effectorFactory->CreateForce();
    factory->AddEffector(eff);
    UpdateEmitterList();
  }

  void CSPartEditSpace::OnButtonRemoveEffector( wxCommandEvent &event )
  {
    if(!ListBoxHasSelection(effectorList)) return;

    factory->RemoveEffector(event.GetSelection());
    UpdateEffectorList();
  }

  void CSPartEditSpace::OnEmitterSelect( wxCommandEvent& event )
  {
    effectorList->DeselectAll();
    iParticleEmitter* emt = static_cast<iParticleEmitter*>(emitterList->GetClientData(event.GetSelection()));
    csRef<iModifiable> mod = scfQueryInterface<iModifiable>(emt);

    if(mod.IsValid()) {
      csString label(csString(emitterList->GetString(event.GetSelection()).GetData()));
      secondaryEditor->SetModifiableLabel(mod, label);
    } else {
      csReport(object_reg, CS_REPORTER_SEVERITY_ERROR,
        "crystalspace.editor.space.partedit",
        "Tried to edit emitter not implementing iModifiable.");

      return;
    }
  }

  void CSPartEditSpace::OnEffectorSelect( wxCommandEvent& event )
  {
    emitterList->DeselectAll();
    iParticleEffector* eff = static_cast<iParticleEffector*>(effectorList->GetClientData(event.GetSelection()));
    csRef<iModifiable> mod = scfQueryInterface<iModifiable>(eff);

    if(mod.IsValid()) {
      csString label(csString(effectorList->GetString(event.GetSelection()).GetData()));
      secondaryEditor->SetModifiableLabel(mod, label);
    } else {
      csReport(object_reg, CS_REPORTER_SEVERITY_ERROR,
        "crystalspace.editor.space.partedit",
        "Tried to edit effector not implementing iModifiable.");

      return;
    }
  }

  void CSPartEditSpace::UpdateEmitterList()
  {
    emitterList->Clear();
    for(size_t i = 0; i < factory->GetEmitterCount(); i++) {
      iParticleEmitter* em = factory->GetEmitter(i);
      emitterList->Append(wxString::Format(wxT("Emitter #%d"), i), (void*)em);
    }
  }

  void CSPartEditSpace::UpdateEffectorList()
  {
    effectorList->Clear();
    for(size_t i = 0; i < factory->GetEffectorCount(); i++) {
      iParticleEffector* em = factory->GetEffector(i);
      effectorList->Append(wxString::Format(wxT("Effector #%d"), i), (void*)em);
    }
  }
}
CS_PLUGIN_NAMESPACE_END(CSEditor)
