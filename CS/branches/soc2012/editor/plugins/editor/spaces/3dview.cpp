/*
    Copyright (C) 2012 by Christian Van Brussel

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
#include "cstool/csview.h"
#include "cstool/initapp.h"
#include "csutil/scf.h"
#include "ieditor/context.h"
#include "iengine/camera.h"
#include "iengine/campos.h"
#include "iengine/sector.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "ivideo/wxwin.h"

#include "3dview.h"

#include <wx/wx.h>

CS_PLUGIN_NAMESPACE_BEGIN(CSEditor)
{

BEGIN_EVENT_TABLE(CS3DSpace::Space, wxPanel)
  EVT_SIZE(CS3DSpace::Space::OnSize)
END_EVENT_TABLE()

SCF_IMPLEMENT_FACTORY (CS3DSpace)

CS3DSpace::CS3DSpace (iBase* parent)
  : scfImplementationType (this, parent), object_reg (0),
  frameListener (nullptr)
{
}

CS3DSpace::~CS3DSpace()
{
  delete frameListener;
}

bool CS3DSpace::Initialize (iObjectRegistry* obj_reg, iEditor* editor,
			    iSpaceFactory* fact, wxWindow* parent)
{
  object_reg = obj_reg;
  this->editor = editor;
  factory = fact;
  
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  engine = csQueryRegistry<iEngine> (object_reg);

  disabled = false;

  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->AllowResize (true);

  wxwin = scfQueryInterface<iWxWindow> (g2d);
  if( !wxwin )
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.editor",
              "Canvas is no iWxWindow plugin!");
    return false;
  }

  window = new CS3DSpace::Space
    (this, parent, -1, wxPoint (0,0), wxSize (-1,-1));
  wxwin->SetParent (window);
  
  //window->SetDropTarget (new MyDropTarget (this));

  // Setup the view and the camera context
  view = csPtr<iView> (new csView (engine, g3d));
  view->SetAutoResize (false);
  view->SetRectangle (0, 0, g2d->GetWidth (), g2d->GetHeight ());

  csRef<iContextCamera> cameraContext =
    scfQueryInterface<iContextCamera> (editor->GetContext ());
  cameraContext->SetCamera (view->GetCamera ());

  // Register this event handler to the editor events
  iEventNameRegistry* registry =
    csEventNameRegistry::GetRegistry (object_reg);
  eventSetCollection = 
    registry->GetID ("crystalspace.editor.context.setcollection");
  RegisterQueue (editor->GetContext ()->GetEventQueue (),
		 eventSetCollection);

  // Register a frame listener to the global event queue
  frameListener = new CS3DSpace::FrameListener (this);

  // Setup the camera manager
  csRef<iPluginManager> pluginManager =
    csQueryRegistry<iPluginManager> (object_reg);

  cameraManager = csLoadPlugin<CS::Utility::iCameraManager>
    (pluginManager, "crystalspace.utilities.cameramanager");
  if (!cameraManager)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.editor.space.3dview",
              "Failed to locate camera manager!");
    return false;
  }
  cameraManager->SetCamera (view->GetCamera ());

  return true;
}

wxWindow* CS3DSpace::GetwxWindow ()
{
  return window;
}

bool CS3DSpace::HandleEvent (iEvent& ev)
{
  if (ev.Name == eventSetCollection)
  {
    csRef<iContextFileLoader> fileLoaderContext =
      scfQueryInterface<iContextFileLoader> (editor->GetContext ());
    const iCollection* collection = fileLoaderContext->GetCollection ();

    if (!collection)
    {
      view->GetCamera ()->SetSector (nullptr);
      return false;
    }

    // TODO: read all of this from the context's collection
    // If there are no sectors then invalidate the camera
    if (!engine->GetSectors ()->GetCount ())
    {
      view->GetCamera ()->SetSector (nullptr);
      return false;
    }

    // Move the camera to the starting sector/position
    csRef<iSector> room;
    csVector3 pos;
  
    if (engine->GetCameraPositions ()->GetCount () > 0)
    {
      // There is a valid starting position defined in the level file.
      iCameraPosition* campos = engine->GetCameraPositions ()->Get (0);
      room = engine->GetSectors ()->FindByName (campos->GetSector ());
      pos = campos->GetPosition ();
    }

    if (!room)
    {
      // We didn't find a valid starting position. So we default
      // to going to the sector called 'room', or the first sector,
      // at position (0,0,0).
      room = engine->GetSectors ()->FindByName ("room");
      if (!room) room = engine->GetSectors ()->Get (0);
      pos = csVector3 (0, 0, 0);
    }

    view->GetCamera ()->SetSector (room);
    view->GetCamera ()->GetTransform ().SetOrigin (pos);

    // Put back the focus on the window
    csRef<iWxWindow> wxwin = scfQueryInterface<iWxWindow> (g3d->GetDriver2D ());
    if (wxwin->GetWindow ())
      wxwin->GetWindow ()->SetFocus ();

    return false;
  }

  DisableUpdates (false);

  return false;
}

void CS3DSpace::UpdateFrame ()
{
  if (!disabled) { // TODO: or check for the availability of a parent?
    // Tell 3D driver we're going to display 3D things.
    if (!g3d->BeginDraw (CSDRAW_CLEARSCREEN | CSDRAW_3DGRAPHICS))
      return;

    // Tell the camera to render into the frame buffer.
    view->Draw ();

    // Finish the drawing
    g3d->FinishDraw ();
    g3d->Print (0);
  }

  else disabled = false;
}

void CS3DSpace::Update ()
{
}

void CS3DSpace::OnSize (wxSizeEvent& event)
{
  if (!g3d.IsValid () || !g3d->GetDriver2D () || !view.IsValid ())
    return;
  
  wxSize size = event.GetSize ();
  iGraphics2D* g2d = g3d->GetDriver2D ();
  
  // Also resize the wxWindow
  csRef<iWxWindow> wxwin = scfQueryInterface<iWxWindow> (g2d);
  if (!wxwin->GetWindow ())
  {
    g2d->Resize (size.x, size.y);
    return;
  }
  
  wxwin->GetWindow()->SetSize (size);
  
  // Update the space ratio
  // TODO: check perspective
  view->GetPerspectiveCamera ()->SetFOV ((float) (size.y) / (float) (size.x), 1.0f);
  view->SetRectangle (0, 0, size.x, size.y);

  event.Skip();
}

CS3DSpace::FrameListener::FrameListener (CS3DSpace* space)
  : space (space)
{
  csRef<iEventQueue> eventQueue =
    csQueryRegistry<iEventQueue> (space->object_reg);
  RegisterQueue (eventQueue, csevFrame (space->object_reg));
}

bool CS3DSpace::FrameListener::HandleEvent (iEvent &event)
{
  space->UpdateFrame ();
  return false;
}

}
CS_PLUGIN_NAMESPACE_END(CSEditor)

