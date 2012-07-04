/*
    Copyright (C) 2011-2012 by Jelle Hellemans, Christian Van Brussel

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
#include "csutil/cseventq.h"
#include "csutil/objreg.h"
#include "csutil/scf.h"
#include "iengine/camera.h"
#include "iengine/collection.h"
#include "iutil/event.h"
#include "iutil/eventq.h"
#include "iutil/object.h"
#include "ivaria/reporter.h"

#include "context.h"

namespace CS {
namespace EditorApp {

Context2::Context2 (iObjectRegistry* obj_reg)
  : scfImplementationType (this), object_reg (obj_reg)
{
  // Create a new event queue (and hack around the fact that a new
  // csEventQueue instance will be registered to the object registry,
  // hence overwriting the global event queue).
  csRef<iEventQueue> mainEventQueue =
    csQueryRegistry<iEventQueue> (object_reg);
  eventQueue.AttachNew (new csEventQueue (object_reg));
  object_reg->Register (mainEventQueue);

  // Initialize the event ID's
  csRef<iEventNameRegistry> registry = csQueryRegistry<iEventNameRegistry> (object_reg);
  eventSetCamera = registry->GetID ("crystalspace.editor.context.setcamera");
  eventSetCollection = registry->GetID ("crystalspace.editor.context.setcollection");
}

Context2::~Context2 ()
{
}

iObjectRegistry* Context2::GetObjectRegistry ()
{
  return object_reg;
}

iEventQueue* Context2::GetEventQueue ()
{
  return eventQueue;
}

void Context2::RegisterData (csStringID id, iBase* data)
{
  ContextData cdata;
  cdata.data = data;
  cdata.eventHandler = new wxEvtHandler ();
  contextData.PutUnique (id, cdata);
}

void Context2::SetData (csStringID id, iBase* data)
{
  ContextData* cdata = contextData.GetElementPointer (id);

#ifdef CS_DEBUG
  if (!cdata)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
	      "crystalspace.editor.context.base",
	      "Invalid context data ID");
    return;
  }
#endif // CS_DEBUG

  cdata->data = data;
  // TODO: callbacks
}

iBase* Context2::GetData (csStringID id)
{
  ContextData* cdata = contextData.GetElementPointer (id);

#ifdef CS_DEBUG
  if (!cdata)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
	      "crystalspace.editor.context.base",
	      "Invalid context data ID");
    return nullptr;
  }
#endif // CS_DEBUG

  return cdata->data;
}

void Context2::PostEvent (csEventID eventID)
{
  csRef<iEvent> event = eventQueue->CreateEvent (eventID);
  eventQueue->Post (event);
  eventQueue->Process ();
}

iObject* Context2::GetActiveObject ()
{
  return active;
}

const csWeakRefArray<iObject>& Context2::GetSelectedObjects () const
{
  return selection;
}

void Context2::AddSelectedObject (iObject* obj)
{
  if (selection.Find(obj) == csArrayItemNotFound)
  {
    selection.Push(obj);
  }
  active = obj;
}

void Context2::RemoveSelectedObject (iObject* obj)
{
  selection.Delete(obj);
  active = obj;
}

void Context2::ClearSelectedObjects ()
{
  selection.DeleteAll();
}

iCamera* Context2::GetCamera ()
{
  return camera;
}

void Context2::SetCamera (iCamera* cam)
{
  camera = cam;
  PostEvent (eventSetCamera);
}

void Context2::SetPath (const char* path)
{
  this->path = path;
}

const char* Context2::GetPath ()
{
  return path;
}

void Context2::SetFilename (const char* filename)
{
  this->filename = filename;
}

const char* Context2::GetFilename ()
{
  return filename;
}

void Context2::SetCollection (iCollection* collection)
{
  this->collection = collection;
  PostEvent (eventSetCollection);
}

iCollection* Context2::GetCollection () const
{
  return collection;
}

} // namespace EditorApp
} // namespace CS
