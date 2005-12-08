/*
    Copyright (C) 2003 by Odes B. Boatwright.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "cssysdef.h"
#include "csutil/csbaseeventh.h"
#include "csutil/event.h"
#include "csutil/csevent.h"
#include "csutil/objreg.h"

#include "iutil/eventq.h"

csBaseEventHandler::csBaseEventHandler() : 
  scfImplementationType (this),
  object_registry (0),
  self (CS_EVENT_INVALID)
{
  FrameEvent = PreProcess = Process = PostProcess 
    = FinalProcess = CS_EVENT_INVALID;
}

void csBaseEventHandler::Initialize (iObjectRegistry *r)
{
  object_registry = r;
  self = csEventHandlerRegistry::GetID(r, this);
  FrameEvent = csevFrame (r);
  PreProcess = csevPreProcess (r);
  Process = csevProcess (r);
  PostProcess = csevPostProcess (r);
  FinalProcess = csevFinalProcess (r);
}

csBaseEventHandler::~csBaseEventHandler()
{
  if (queue)
    queue->RemoveListener (this);
  if (object_registry)
    csEventHandlerRegistry::ReleaseID(object_registry, this);
}

bool csBaseEventHandler::RegisterQueue (iEventQueue* q, csEventID event)
{
  if (queue)
    queue->RemoveListener (this);
  queue = q;
  if (0 != q)
    q->RegisterListener(this, event);
  return true;
}

bool csBaseEventHandler::RegisterQueue (iEventQueue* q, csEventID events[])
{
  if (queue)
    queue->RemoveListener (this);
  queue = q;
  if (q != 0)
    q->RegisterListener(this, events);
  return true;
}

bool csBaseEventHandler::RegisterQueue (iObjectRegistry* registry, csEventID event)
{
  csRef<iEventQueue> q (CS_QUERY_REGISTRY (registry, iEventQueue));
  if (0 == q)
    return false;
  return RegisterQueue (q, event);
}

bool csBaseEventHandler::RegisterQueue (iObjectRegistry* registry, csEventID events[])
{
  csRef<iEventQueue> q (CS_QUERY_REGISTRY (registry, iEventQueue));
  if (0 == q)
    return false;
  return RegisterQueue (q, events);
}

bool csBaseEventHandler::HandleEvent (iEvent &event)
{
  if (event.Name == FrameEvent)
  {
    Frame();
    return true;
  }
  else if (event.Name == PreProcess)
  {
    PreProcessFrame ();
    return true;
  }
  else if (event.Name == Process)
  {
    ProcessFrame ();
    return true;
  }
  else if (event.Name == PostProcess)
  {
    PostProcessFrame ();
    return true;
  }
  else if (event.Name == FinalProcess)
  {
    FinishFrame ();
    return true;
  }
  else if (CS_IS_KEYBOARD_EVENT(object_registry, event))
    return OnKeyboard(event);
  else if (CS_IS_MOUSE_EVENT(object_registry, event))
  {
    switch(csMouseEventHelper::GetEventType(&event))
    {
    case csMouseEventTypeMove:
      return OnMouseMove(event);
    case csMouseEventTypeUp:
      return OnMouseUp(event);
    case csMouseEventTypeDown:
      return OnMouseDown(event);
    case csMouseEventTypeClick:
      return OnMouseClick(event);
    case csMouseEventTypeDoubleClick:
      return OnMouseDoubleClick(event);
    }
  }
  else if (CS_IS_JOYSTICK_EVENT(object_registry, event))
  {
    if (csJoystickEventHelper::GetButton(&event))
    {
      if (csJoystickEventHelper::GetButtonState(&event))
	return OnJoystickDown(event);
      else
	return OnJoystickUp(event);
    }
    else
    {
      return OnJoystickMove(event);
    }
  }
  return  OnUnhandledEvent(event);
}

#define DefaultTrigger(trigger)			   \
  bool csBaseEventHandler::trigger (iEvent &event) \
  { return false; }

DefaultTrigger ( OnUnhandledEvent );
DefaultTrigger ( OnKeyboard );
DefaultTrigger ( OnMouseMove );
DefaultTrigger ( OnMouseDown );
DefaultTrigger ( OnMouseUp );
DefaultTrigger ( OnMouseClick );
DefaultTrigger ( OnMouseDoubleClick );
DefaultTrigger ( OnJoystickMove );
DefaultTrigger ( OnJoystickDown );
DefaultTrigger ( OnJoystickUp );

#define DefaultVoidTrigger(trigger)   \
  void csBaseEventHandler::trigger () \
  { return; }

DefaultVoidTrigger ( Frame );
DefaultVoidTrigger ( PreProcessFrame );
DefaultVoidTrigger ( ProcessFrame );
DefaultVoidTrigger ( PostProcessFrame );
DefaultVoidTrigger ( FinishFrame );
