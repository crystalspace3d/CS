/*
    Copyright (C) 2000-2001 by Jorrit Tyberghein

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
#include "walktest/wentity.h"
#include "walktest/walktest.h"
#include "csgeom/matrix3.h"
#include "iengine/light.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"

extern WalkTest* Sys;

SCF_IMPLEMENT_IBASE_EXT (csWalkEntity)
  SCF_IMPLEMENTS_INTERFACE (csWalkEntity)
SCF_IMPLEMENT_IBASE_EXT_END

//--------------------------------------------------------------------------

csDoor::csDoor (iMeshWrapper* p)
{
  is_open = false;
  transition = 0;
  tparent = p;
}

void csDoor::Activate ()
{
printf ("Activate Door!\n");
  is_open = !is_open;
  // We do 1-transition here to make sure that when we
  // activate the door while in mid-transition it will
  // just go back from that point.
  transition = 1-transition;
  // Push ourselves on to the busy list if we're not already there.
  int idx = Sys->busy_entities.Find (this);
  if (idx != -1) Sys->busy_entities.Delete (idx);
  Sys->busy_entities.Push (this);
}

void csDoor::NextFrame (float elapsed_time)
{
  if (!transition)
  {
    int idx = Sys->busy_entities.Find (this);
    if (idx != -1) Sys->busy_entities.Delete (idx);
printf ("Done opening door.\n");
    return;
  }
  transition -= elapsed_time/1000.;
  if (transition < 0) transition = 0;
  csYRotMatrix3 mat (HALF_PI * transition);
  mat.Invert ();
  tparent->GetMovable ()->SetTransform (mat);
  tparent->GetMovable ()->UpdateMove ();
}

//--------------------------------------------------------------------------


csRotatingObject::csRotatingObject (iObject* p)
{
  always = true;
  tparent = p;
  angles.Set (90, 0, 0);
  remaining = 0;
  iMeshWrapper *mw = SCF_QUERY_INTERFACE (p, iMeshWrapper);
  if (mw)
  {
    movable = mw->GetMovable ();
    mw->DecRef ();
  }
}

void csRotatingObject::Activate ()
{
  if (always) return;
  // Push ourselves on to the busy list if we're not already there.
  int idx = Sys->busy_entities.Find (this);
  if (idx != -1) Sys->busy_entities.Delete (idx);
  Sys->busy_entities.Push (this);
  remaining = 10000;
}

void csRotatingObject::NextFrame (float elapsed_time)
{
  if (remaining)
  {
    remaining -= elapsed_time;
    if (remaining <= 0)
    {
      remaining = 0;
      int idx = Sys->busy_entities.Find (this);
      if (idx != -1) Sys->busy_entities.Delete (idx);
    }
  }
  else if (!always) return;

  float trans = (TWO_PI / 360.) * (elapsed_time / 1000.);
  csXRotMatrix3 matx (angles.x * trans);
  csYRotMatrix3 maty (angles.y * trans);
  csZRotMatrix3 matz (angles.z * trans);
  csMatrix3 mat = matz * maty * matx;
  movable->Transform (mat);
  movable->UpdateMove ();
}

//--------------------------------------------------------------------------

csLightObject::csLightObject (iLight* p)
{
  light = p;
  act_time = 1000;
  cur_time = 0;
}

void csLightObject::Activate ()
{
  // Push ourselves on to the busy list if we're not already there.
  int idx = Sys->busy_entities.Find (this);
  if (idx != -1) Sys->busy_entities.Delete (idx);
  Sys->busy_entities.Push (this);
  cur_time = act_time;
}

void csLightObject::NextFrame (float elapsed_time)
{
  if (cur_time <= 0) return;
  cur_time -= elapsed_time;
  if (cur_time <= 0)
  {
    cur_time = 0;
    int idx = Sys->busy_entities.Find (this);
    if (idx != -1) Sys->busy_entities.Delete (idx);
  }

  csColor s_color (start_color);
  csColor e_color (end_color);
  s_color *= cur_time/act_time;
  e_color *= (act_time-cur_time)/act_time;
  light->SetColor (s_color+e_color);
}

//--------------------------------------------------------------------------

struct AnimPortalCallback : public iPortalCallback
{
  AnimPortalCallback ();
  csAnimatedPortal* animportal;
  SCF_DECLARE_IBASE;
  virtual bool Traverse (iPortal* portal, iBase* context);
};

SCF_IMPLEMENT_IBASE (AnimPortalCallback)
  SCF_IMPLEMENTS_INTERFACE (iPortalCallback)
SCF_IMPLEMENT_IBASE_END

AnimPortalCallback::AnimPortalCallback ()
{
  SCF_CONSTRUCT_IBASE (NULL);
}

bool AnimPortalCallback::Traverse (iPortal*, iBase* )
{
  animportal->visible = true;
  return true;
}

csAnimatedPortal::csAnimatedPortal (iPortal* p,
	int xyz, float max_angle, float speed)
{
  portal = p;
  AnimPortalCallback* cb = new AnimPortalCallback ();
  cb->animportal = this;
  portal->SetPortalCallback (cb);
  cb->DecRef ();
  csAnimatedPortal::xyz = xyz;
  csAnimatedPortal::max_angle = max_angle;
  csAnimatedPortal::speed = speed;
  orig_trans = portal->GetWarp ();
  cur_angle = 0;
  cur_dir = 1;
  visible = false;
}

void csAnimatedPortal::Activate ()
{
  // Push ourselves on to the busy list if we're not already there.
  int idx = Sys->busy_entities.Find (this);
  if (idx != -1) Sys->busy_entities.Delete (idx);
  Sys->busy_entities.Push (this);
}

void csAnimatedPortal::NextFrame (float elapsed_time)
{
  if (!visible) return;
  visible = false;

  if (cur_dir == 1)
  {
    cur_angle += elapsed_time/speed;
    if (cur_angle > max_angle) { cur_angle = max_angle; cur_dir = -1; }
  }
  else
  {
    cur_angle -= elapsed_time/speed;
    if (cur_angle < -max_angle) { cur_angle = -max_angle; cur_dir = 1; }
  }

  csReversibleTransform trans = orig_trans;
  switch (xyz)
  {
    case 1:
      trans *= csTransform (csXRotMatrix3 (cur_angle), csVector3 (0));
      break;
    case 2:
      trans *= csTransform (csYRotMatrix3 (cur_angle), csVector3 (0));
      break;
    case 3:
      trans *= csTransform (csZRotMatrix3 (cur_angle), csVector3 (0));
      break;
  }
  portal->SetWarp (trans);
}


//--------------------------------------------------------------------------

