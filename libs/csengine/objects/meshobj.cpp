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
#include "qsqrt.h"
#include "csgeom/sphere.h"
#include "igeom/objmodel.h"
#include "csengine/sector.h"
#include "csengine/meshobj.h"
#include "csengine/meshlod.h"
#include "csengine/light.h"
#include "csengine/engine.h"
#include "iengine/portal.h"
#include "csutil/debug.h"
#include "iengine/rview.h"
#include "ivideo/graph3d.h"


// ---------------------------------------------------------------------------
// csMeshWrapper
// ---------------------------------------------------------------------------
SCF_IMPLEMENT_IBASE_EXT(csMeshWrapper)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iMeshWrapper)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iImposter)
  SCF_IMPLEMENTS_INTERFACE(csMeshWrapper)
  SCF_IMPLEMENTS_INTERFACE(iVisibilityObject)
SCF_IMPLEMENT_IBASE_EXT_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csMeshWrapper::MeshWrapper)
  SCF_IMPLEMENTS_INTERFACE(iMeshWrapper)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csMeshWrapper::MeshImposter)
  SCF_IMPLEMENTS_INTERFACE(iImposter)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csMeshWrapper::csMeshWrapper (iMeshWrapper *theParent, iMeshObject *meshobj) :
    csObject ()
{
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiMeshWrapper);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiImposter);
  DG_TYPE (this, "csMeshWrapper");

  movable.scfParent = (iBase*)(csObject*)this;
  visnr = 0;
  cached_lod_visnr = ~0;
  wor_bbox_movablenr = -1;
  movable.SetMeshWrapper (this);
  Parent = theParent;
  if (Parent)
  {
    csParent = ((csMeshWrapper::MeshWrapper*)Parent)->scfParent;
    movable.SetParent (Parent->GetMovable ());
  }
  else
  {
    csParent = 0;
  }

  render_priority = csEngine::current_engine->GetObjectRenderPriority ();

  defered_num_lights = 0;
  defered_lighting_flags = 0;
  last_anim_time = 0;

  csMeshWrapper::meshobj = meshobj;
  if (meshobj)
  {
    light_info = SCF_QUERY_INTERFACE (meshobj, iLightingInfo);
    shadow_receiver = SCF_QUERY_INTERFACE (meshobj, iShadowReceiver);
    portal_container = SCF_QUERY_INTERFACE (meshobj, iPortalContainer);
  }
  factory = 0;
  zbufMode = CS_ZBUF_USE;
  children.SetMesh (this);
  imposter_active = false;
  imposter_mesh = 0;
  cast_hardware_shadow = true;
  draw_after_fancy_stuff = false;
}

csMeshWrapper::csMeshWrapper (iMeshWrapper *theParent) :
  csObject ()
{
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiMeshWrapper);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiImposter);
  DG_TYPE (this, "csMeshWrapper");

  movable.scfParent = (iBase*)(csObject*)this;
  visnr = 0;
  cached_lod_visnr = ~0;
  wor_bbox_movablenr = -1;
  movable.SetMeshWrapper (this);
  Parent = theParent;
  if (Parent)
  {
    csParent = ((csMeshWrapper::MeshWrapper*)Parent)->scfParent;
    movable.SetParent (Parent->GetMovable ());
  }
  else
  {
    csParent = 0;
  }

  render_priority = csEngine::current_engine->GetObjectRenderPriority ();

  defered_num_lights = 0;
  defered_lighting_flags = 0;
  last_anim_time = 0;
  imposter_active = false;
  imposter_mesh = 0;

  factory = 0;
  zbufMode = CS_ZBUF_USE;
  children.SetMesh (this);
  cast_hardware_shadow = true;
  draw_after_fancy_stuff = false;
}

void csMeshWrapper::SetParentContainer (iMeshWrapper* newParent)
{
  Parent = newParent;
  if (Parent)
    csParent = ((csMeshWrapper::MeshWrapper*)Parent)->scfParent;
  else
    csParent = 0;
}

void csMeshWrapper::ClearFromSectorPortalLists ()
{
  if (portal_container)
  {
    int i;
    const iSectorList *sectors = movable.GetSectors ();
    for (i = 0; i < sectors->GetCount (); i++)
    {
      iSector *ss = sectors->Get (i);
      if (ss) ss->UnregisterPortalMesh (&scfiMeshWrapper);
    }
  }
}

void csMeshWrapper::SetMeshObject (iMeshObject *meshobj)
{
  ClearFromSectorPortalLists ();

  csMeshWrapper::meshobj = meshobj;
  if (meshobj)
  {
    light_info = SCF_QUERY_INTERFACE (meshobj, iLightingInfo);
    shadow_receiver = SCF_QUERY_INTERFACE (meshobj, iShadowReceiver);
    portal_container = SCF_QUERY_INTERFACE (meshobj, iPortalContainer);
    if (portal_container)
    {
      int i;
      const iSectorList *sectors = movable.GetSectors ();
      for (i = 0; i < sectors->GetCount (); i++)
      {
        iSector *ss = sectors->Get (i);
        if (ss) ss->UnregisterPortalMesh (&scfiMeshWrapper);
      }
    }
  }
  else
  {
    light_info = 0;
    shadow_receiver = 0;
    portal_container = 0;
  }
}

csMeshWrapper::~csMeshWrapper ()
{
  delete imposter_mesh;
  ClearFromSectorPortalLists ();
  SCF_DESTRUCT_EMBEDDED_IBASE (scfiImposter);
  SCF_DESTRUCT_EMBEDDED_IBASE (scfiMeshWrapper);
}

void csMeshWrapper::UpdateMove ()
{
  int i;
  for (i = 0; i < children.GetCount (); i++)
  {
    iMeshWrapper *spr = children.Get (i);
    spr->GetMovable ()->UpdateMove ();
  }
}

void csMeshWrapper::MoveToSector (iSector *s)
{
  // Only add this mesh to a sector if the parent is the engine.
  // Otherwise we have a hierarchical object and in that case
  // the parent object controls this.
  if (!Parent) s->GetMeshes ()->Add (&scfiMeshWrapper);
  if (portal_container)
    s->RegisterPortalMesh (&scfiMeshWrapper);
}

void csMeshWrapper::RemoveFromSectors ()
{
  if (Parent) return ;

  int i;
  const iSectorList *sectors = movable.GetSectors ();
  for (i = 0; i < sectors->GetCount (); i++)
  {
    iSector *ss = sectors->Get (i);
    if (ss)
    {
      ss->GetMeshes ()->Remove (&scfiMeshWrapper);
      ss->UnregisterPortalMesh (&scfiMeshWrapper);
    }
  }
}

void csMeshWrapper::SetRenderPriority (long rp)
{
  render_priority = rp;

  if (Parent) return ;

  int i;
  const iSectorList *sectors = movable.GetSectors ();
  for (i = 0; i < sectors->GetCount (); i++)
  {
    iSector *ss = sectors->Get (i);
    if (ss) ss->GetPrivateObject ()->RelinkMesh (&scfiMeshWrapper);
  }
}

/// The list of lights that hit the mesh
typedef csDirtyAccessArray<iLight*> engine3d_LightWorkTable;
CS_IMPLEMENT_STATIC_VAR (GetStaticLightWorkTable, engine3d_LightWorkTable,())

void csMeshWrapper::UpdateDeferedLighting (const csBox3 &box)
{
  static engine3d_LightWorkTable &light_worktable = *GetStaticLightWorkTable ();
  const iSectorList *movable_sectors = movable.GetSectors ();
  if (defered_num_lights && movable_sectors->GetCount () > 0)
  {
    if (defered_num_lights > light_worktable.Length ())
      light_worktable.SetLength (defered_num_lights);

    iSector *sect = movable_sectors->Get (0);
    int num_lights = csEngine::current_iengine->GetNearbyLights (
        sect,
        box,
        defered_lighting_flags,
        light_worktable.GetArray (),
        defered_num_lights);
    UpdateLighting (light_worktable.GetArray (), num_lights);
  }
}

void csMeshWrapper::DeferUpdateLighting (int flags, int num_lights)
{
  defered_num_lights = num_lights;
  defered_lighting_flags = flags;
}

void csMeshWrapper::Draw (iRenderView *rview)
{
  if (flags.Check (CS_ENTITY_INVISIBLE)) return;
  if (csParent && !csParent->IsChildVisible (&scfiMeshWrapper, rview)) return;
  DrawInt (rview);
}

csRenderMesh** csMeshWrapper::GetRenderMeshes (int& n)
{
//  iMeshWrapper *meshwrap = &scfiMeshWrapper;

  //int i;
  // Callback are traversed in reverse order so that they can safely
  // delete themselves.
/*  i = draw_cb_vector.Length ()-1;
  while (i >= 0)
  {
    iMeshDrawCallback* cb = draw_cb_vector.Get (i);
    if (!cb->BeforeDrawing (meshwrap, rview)) return 0;
    i--;
  }*/

  /*draw_test = meshobj->DrawTest (rview, &movable.scfiMovable);
  if (draw_test)
  {
    csTicks lt = csEngine::current_engine->GetLastAnimationTime ();
    if (lt != 0)
    {
      if (lt != last_anim_time)
      {
        meshobj->NextFrame (lt,movable.GetPosition ());
        last_anim_time = lt;
      }
    }*/

    csTicks lt = csEngine::current_engine->GetLastAnimationTime ();
    meshobj->NextFrame (lt,movable.GetPosition ());
    UpdateDeferedLighting (movable.GetFullPosition ());
    return meshobj->GetRenderMeshes (n);
/*  }
  return 0;*/

  /*
  for (i = 0; i < children.GetCount (); i++)
  {
    iMeshWrapper *spr = children.Get (i);
    spr->DrawZ (rview);
  }
  */
}

void csMeshWrapper::DrawShadow (iRenderView* rview, iLight* light)
{
  /*
  if (cast_hardware_shadow)
    meshobj->DrawShadow (rview, &movable.scfiMovable, zbufMode, light);
  */
}

void csMeshWrapper::DrawLight (iRenderView* rview, iLight* light)
{
  /*
  if (draw_test) 
    meshobj->DrawLight (rview, &movable.scfiMovable, zbufMode, light);
  */
}

void csMeshWrapper::CastHardwareShadow (bool castShadow)
{
  cast_hardware_shadow = castShadow;
}

void csMeshWrapper::SetDrawAfterShadow (bool drawAfter)
{
  draw_after_fancy_stuff = drawAfter;
}

bool csMeshWrapper::GetDrawAfterShadow ()
{
  return draw_after_fancy_stuff;
}

//----- Static LOD ----------------------------------------------------------

iLODControl* csMeshWrapper::CreateStaticLOD ()
{
  static_lod = csPtr<csStaticLODMesh> (new csStaticLODMesh ());
  return static_lod;
}

void csMeshWrapper::DestroyStaticLOD ()
{
  static_lod = 0;
}

iLODControl* csMeshWrapper::GetStaticLOD ()
{
  return (iLODControl*)static_lod;
}

void csMeshWrapper::RemoveMeshFromStaticLOD (iMeshWrapper* mesh)
{
  if (!static_lod) return;	// No static lod, nothing to do here.
  int lod;
  for (lod = 0 ; lod < static_lod->GetLODCount () ; lod++)
  {
    csArray<iMeshWrapper*>& meshes_for_lod = static_lod->GetMeshesForLOD (lod);
    meshes_for_lod.Delete (mesh);
  }
}

void csMeshWrapper::AddMeshToStaticLOD (int lod, iMeshWrapper* mesh)
{
  if (!static_lod) return;	// No static lod, nothing to do here.
  csArray<iMeshWrapper*>& meshes_for_lod = static_lod->GetMeshesForLOD (lod);
  meshes_for_lod.Push (mesh);
}

//---------------------------------------------------------------------------

void csMeshWrapper::DrawInt (iRenderView *rview)
{
  if (imposter_active && CheckImposterRelevant (rview))
    if (DrawImposter (rview))
      return;

  DrawIntFull (rview);
}

bool csMeshWrapper::CheckImposterRelevant (iRenderView *rview)
{
  float wor_sq_dist = GetSquaredDistance (rview);
  float dist = min_imposter_distance->Get ();
  return (wor_sq_dist > dist*dist);
}

bool csMeshWrapper::IsChildVisible (iMeshWrapper* child, iRenderView* rview)
{
  if (flags.Check (CS_ENTITY_INVISIBLE)) return false;

  if (static_lod)
  {
    uint32 visnr = ((csSector::eiSector*)rview->GetThisSector ())->
      GetCsSector ()->current_visnr;

    // If we have static lod we only draw the children for the right LOD level.
    if (cached_lod_visnr != visnr)
    {
      float distance = qsqrt (GetSquaredDistance (rview));
      cached_lod = static_lod->GetLODValue (distance);
      cached_lod_visnr = visnr;
    }
    csArray<iMeshWrapper*>& meshes = static_lod->GetMeshesForLOD (cached_lod);
    // @@@ This loop is not very efficient!
    int i;
    for (i = 0 ; i < meshes.Length () ; i++)
    {
      if (meshes[i] == child) return true;
    }
    return false;
  }

  if (csParent && !csParent->IsChildVisible (&scfiMeshWrapper, rview))
    return false;

  return true;
}

void csMeshWrapper::DrawIntFull (iRenderView *rview)
{
  iMeshWrapper *meshwrap = &scfiMeshWrapper;

  int i;
  // Callback are traversed in reverse order so that they can safely
  // delete themselves.
  i = draw_cb_vector.Length ()-1;
  while (i >= 0)
  {
    iMeshDrawCallback* cb = draw_cb_vector.Get (i);
    if (!cb->BeforeDrawing (meshwrap, rview)) return ;
    i--;
  }

  if (meshobj->DrawTest (rview, &movable.scfiMovable))
  {
    csTicks lt = csEngine::current_engine->GetLastAnimationTime ();
    if (lt != 0)
    {
      if (lt != last_anim_time)
      {
	meshobj->NextFrame (lt, movable.GetPosition ());
	last_anim_time = lt;
	if(GetParentContainer() != 0)
	  GetParentContainer()->GetMeshObject()->PositionChild(meshobj,lt);
      }
    }

    csBox3 bbox;
    GetFullBBox (bbox);
    UpdateDeferedLighting (bbox);
    meshobj->Draw (rview, &movable.scfiMovable, zbufMode);
  }

#if 0
  if (static_lod)
  {
    // If we have static lod we only draw the children for the right LOD level.
    float distance = qsqrt (GetSquaredDistance (rview));
    float lod = static_lod->GetLODValue (distance);
    csArray<iMeshWrapper*>& meshes = static_lod->GetMeshesForLOD (lod);
    for (i = 0 ; i < meshes.Length () ; i++)
    {
      meshes[i]->Draw (rview);
    }
  }
#endif
}

bool csMeshWrapper::DrawImposter (iRenderView *rview)
{
  // Check for imposter existence.  If not, create it.
  if (!imposter_mesh)
  {
    return false;
  }

  // Check for imposter already ready
  if (!imposter_mesh->GetImposterReady ())
    return false;

  // Check for too much camera movement since last imposter render
  if (!imposter_mesh->CheckIncidenceAngle (rview,
	imposter_rotation_tolerance->Get ()))
    return false;

  // Else draw imposter as-is.
  imposter_mesh->Draw (rview);
  return true;
}

void csMeshWrapper::SetImposterActive (bool flag)
{
  imposter_active = flag;
  if (flag)
  {
    imposter_mesh = new csImposterMesh (this);
    imposter_mesh->SetImposterReady (false);
  }
}

void csMeshWrapper::UpdateLighting (iLight **lights, int num_lights)
{
  defered_num_lights = 0;

  //if (num_lights <= 0) return;
  meshobj->UpdateLighting (lights, num_lights, &movable.scfiMovable);

  int i;
  for (i = 0; i < children.GetCount (); i++)
  {
    iMeshWrapper *spr = children.Get (i);
    spr->UpdateLighting (lights, num_lights);
  }
}

bool csMeshWrapper::HitBeamOutline (
  const csVector3 &start,
  const csVector3 &end,
  csVector3 &isect,
  float *pr)
{
  return meshobj->HitBeamOutline (start, end, isect, pr);
}

bool csMeshWrapper::HitBeamObject (
  const csVector3 &start,
  const csVector3 &end,
  csVector3 &isect,
  float *pr, int* polygon_idx)
{
  return meshobj->HitBeamObject (start, end, isect, pr, polygon_idx);
}

bool csMeshWrapper::HitBeam (
  const csVector3 &start,
  const csVector3 &end,
  csVector3 &isect,
  float *pr)
{
  csVector3 startObj;
  csVector3 endObj;
  csReversibleTransform trans;
  if (movable.IsFullTransformIdentity ())
  {
    startObj = start;
    endObj = end;
  }
  else
  {
    trans = movable.GetFullTransform ();
    startObj = trans.Other2This (start);
    endObj = trans.Other2This (end);
  }
  bool rc = false;
  if (HitBeamBBox (startObj, endObj, isect, 0) > -1)
  {
    rc = HitBeamOutline (startObj, endObj, isect, pr);
    if (rc)
    {
      if (!movable.IsFullTransformIdentity ())
        isect = trans.This2Other (isect);
    }
  }

  return rc;
}

void csMeshWrapper::HardTransform (const csReversibleTransform &t)
{
  meshobj->HardTransform (t);

  int i;
  for (i = 0; i < children.GetCount (); i++)
  {
    iMeshWrapper *spr = children.Get (i);
    spr->HardTransform (t);
  }
}

void csMeshWrapper::GetRadius (csVector3 &rad, csVector3 &cent) const
{
  meshobj->GetObjectModel ()->GetRadius (rad, cent);
  if (children.GetCount () > 0)
  {
    float max_radius = rad.x;
    if (max_radius < rad.y) max_radius = rad.y;
    if (max_radius < rad.z) max_radius = rad.z;

    csSphere sphere (cent, max_radius);
    int i;
    for (i = 0; i < children.GetCount (); i++)
    {
      iMeshWrapper *spr = children.Get (i);
      csVector3 childrad, childcent;
      spr->GetRadius (childrad, childcent);

      float child_max_radius = childrad.x;
      if (child_max_radius < childrad.y) child_max_radius = childrad.y;
      if (child_max_radius < childrad.z) child_max_radius = childrad.z;

      csSphere childsphere (childcent, child_max_radius);

      // @@@ Is this the right transform?
      childsphere *= spr->GetMovable ()->GetTransform ();
      sphere += childsphere;
    }

    rad.Set (sphere.GetRadius (), sphere.GetRadius (), sphere.GetRadius ());
    cent.Set (sphere.GetCenter ());
  }
}

float csMeshWrapper::MeshWrapper::GetScreenBoundingBox (
  iCamera *camera,
  csBox2 &sbox,
  csBox3 &cbox)
{
  return scfParent->GetScreenBoundingBox (camera, sbox, cbox);
}

float csMeshWrapper::GetSquaredDistance (iRenderView *rview)
{
  iCamera* camera = rview->GetCamera ();
  // calculate distance from camera to mesh
  csBox3 obox;
  GetObjectModel ()->GetObjectBoundingBox (obox, CS_BBOX_MAX);
  csVector3 obj_center = (obox.Min () + obox.Max ()) / 2;
  csVector3 wor_center;
  if (movable.IsFullTransformIdentity ())
    wor_center = obj_center;
  else
    wor_center = movable.GetFullTransform ().This2Other (obj_center);
  csVector3 cam_origin = camera->GetTransform ().GetOrigin ();
  float wor_sq_dist = csSquaredDist::PointPoint (cam_origin, wor_center);
  return wor_sq_dist;
}

void csMeshWrapper::GetFullBBox (csBox3& box)
{
  GetObjectModel ()->GetObjectBoundingBox (box, CS_BBOX_MAX);
  iMovable* mov = &movable.scfiMovable;
  while (mov)
  {
    if (!mov->IsTransformIdentity ())
    {
      const csReversibleTransform& trans = mov->GetTransform ();
      csBox3 b (trans.This2Other (box.GetCorner (0)));
      b.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (1)));
      b.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (2)));
      b.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (3)));
      b.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (4)));
      b.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (5)));
      b.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (6)));
      b.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (7)));
      box = b;
    }
    mov = mov->GetParent ();
  }
}


void csMeshWrapper::PlaceMesh ()
{
  iSectorList *movable_sectors = movable.GetSectors ();
  if (movable_sectors->GetCount () == 0) return ; // Do nothing
  csSphere sphere;
  csVector3 radius;
  GetObjectModel ()->GetRadius (radius, sphere.GetCenter ());

  iSector *sector = movable_sectors->Get (0);
  movable.SetSector (sector);       // Make sure all other sectors are removed

  // Transform the sphere from object to world space.
  float max_radius = radius.x;
  if (max_radius < radius.y) max_radius = radius.y;
  if (max_radius < radius.z) max_radius = radius.z;
  sphere.SetRadius (max_radius);
  if (!movable.IsFullTransformIdentity ())
    sphere = movable.GetFullTransform ().This2Other (sphere);
  max_radius = sphere.GetRadius ();
  float max_sq_radius = max_radius * max_radius;

  csRef<iMeshWrapperIterator> it = csEngine::current_engine
  	->GetNearbyMeshes (sector, sphere.GetCenter (), max_radius, true);

  int j;
  while (it->HasNext ())
  {
    iMeshWrapper* mesh = it->Next ();
    iPortalContainer* portals = mesh->GetPortalContainer ();
    if (!portals) continue;	// No portals.
    int pc_count = portals->GetPortalCount ();

    for (j = 0 ; j < pc_count ; j++)
    {
      iPortal *portal = portals->GetPortal (j);
      iSector *dest_sector = portal->GetSector ();
      if (movable_sectors->Find (dest_sector) == -1)
      {
	iMovable* movable = mesh->GetMovable ();
        const csPlane3 &pl = portal->GetWorldPlane (movable);

        float sqdist = csSquaredDist::PointPlane (sphere.GetCenter (), pl);
        if (sqdist <= max_sq_radius)
        {
          // Plane of portal is close enough.
          // If N is the normal of the portal plane then we
          // can use that to calculate the point on the portal plane.
          csVector3 testpoint = sphere.GetCenter () + pl.Normal () * qsqrt (
                  sqdist);
          if (portal->PointOnPolygon (testpoint, movable))
            movable_sectors->Add (dest_sector);
        }
      }
    }
  }
}

int csMeshWrapper::HitBeamBBox (
  const csVector3 &start,
  const csVector3 &end,
  csVector3 &isect,
  float *pr)
{
  csBox3 b;
  GetObjectModel ()->GetObjectBoundingBox (b, CS_BBOX_MAX);

  csSegment3 seg (start, end);
  return csIntersect3::BoxSegment (b, seg, isect, pr);
}

void csMeshWrapper::GetWorldBoundingBox (csBox3 &cbox)
{
  if (wor_bbox_movablenr != movable.GetUpdateNumber ())
  {
    wor_bbox_movablenr = movable.GetUpdateNumber ();

    if (movable.IsFullTransformIdentity ())
      GetObjectModel ()->GetObjectBoundingBox (wor_bbox, CS_BBOX_MAX);
    else
    {
      csBox3 obj_bbox;
      GetObjectModel ()->GetObjectBoundingBox (obj_bbox, CS_BBOX_MAX);

      // @@@ Maybe it would be better to really calculate the bounding box
      // here instead of just transforming the object space bounding box?
      csReversibleTransform mt = movable.GetFullTransform ();
      wor_bbox.StartBoundingBox (mt.This2Other (obj_bbox.GetCorner (0)));
      wor_bbox.AddBoundingVertexSmart (mt.This2Other (obj_bbox.GetCorner (1)));
      wor_bbox.AddBoundingVertexSmart (mt.This2Other (obj_bbox.GetCorner (2)));
      wor_bbox.AddBoundingVertexSmart (mt.This2Other (obj_bbox.GetCorner (3)));
      wor_bbox.AddBoundingVertexSmart (mt.This2Other (obj_bbox.GetCorner (4)));
      wor_bbox.AddBoundingVertexSmart (mt.This2Other (obj_bbox.GetCorner (5)));
      wor_bbox.AddBoundingVertexSmart (mt.This2Other (obj_bbox.GetCorner (6)));
      wor_bbox.AddBoundingVertexSmart (mt.This2Other (obj_bbox.GetCorner (7)));
    }
  }

  cbox = wor_bbox;
}

void csMeshWrapper::GetTransformedBoundingBox (
  const csReversibleTransform &trans,
  csBox3 &cbox)
{
  csBox3 box;
  GetObjectModel ()->GetObjectBoundingBox (box);
  cbox.StartBoundingBox (trans * box.GetCorner (0));
  cbox.AddBoundingVertexSmart (trans * box.GetCorner (1));
  cbox.AddBoundingVertexSmart (trans * box.GetCorner (2));
  cbox.AddBoundingVertexSmart (trans * box.GetCorner (3));
  cbox.AddBoundingVertexSmart (trans * box.GetCorner (4));
  cbox.AddBoundingVertexSmart (trans * box.GetCorner (5));
  cbox.AddBoundingVertexSmart (trans * box.GetCorner (6));
  cbox.AddBoundingVertexSmart (trans * box.GetCorner (7));
}

float csMeshWrapper::GetScreenBoundingBox (
  const iCamera *camera,
  csBox2 &sbox,
  csBox3 &cbox)
{
  csVector2 oneCorner;
  csReversibleTransform tr_o2c = camera->GetTransform ();
  if (!movable.IsFullTransformIdentity ())
    tr_o2c /= movable.GetFullTransform ();
  GetTransformedBoundingBox (tr_o2c, cbox);

  // if the entire bounding box is behind the camera, we're done
  if ((cbox.MinZ () < 0) && (cbox.MaxZ () < 0))
  {
    return -1;
  }

  // Transform from camera to screen space.
  if (cbox.MinZ () <= 0)
  {
    // Mesh is very close to camera.
    // Just return a maximum bounding box.
    sbox.Set (-10000, -10000, 10000, 10000);
  }
  else
  {
    camera->Perspective (cbox.Max (), oneCorner);
    sbox.StartBoundingBox (oneCorner);

    csVector3 v (cbox.MinX (), cbox.MinY (), cbox.MaxZ ());
    camera->Perspective (v, oneCorner);
    sbox.AddBoundingVertexSmart (oneCorner);
    camera->Perspective (cbox.Min (), oneCorner);
    sbox.AddBoundingVertexSmart (oneCorner);
    v.Set (cbox.MaxX (), cbox.MaxY (), cbox.MinZ ());
    camera->Perspective (v, oneCorner);
    sbox.AddBoundingVertexSmart (oneCorner);
  }

  return cbox.MaxZ ();
}

// ---------------------------------------------------------------------------
// csMeshFactoryWrapper
// ---------------------------------------------------------------------------
SCF_IMPLEMENT_IBASE_EXT(csMeshFactoryWrapper)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE(iMeshFactoryWrapper)
  SCF_IMPLEMENTS_INTERFACE(csMeshFactoryWrapper)
SCF_IMPLEMENT_IBASE_EXT_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csMeshFactoryWrapper::MeshFactoryWrapper)
  SCF_IMPLEMENTS_INTERFACE(iMeshFactoryWrapper)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csMeshFactoryWrapper::csMeshFactoryWrapper (
  iMeshObjectFactory *meshFact)
{
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiMeshFactoryWrapper);
  csMeshFactoryWrapper::meshFact = meshFact;
  parent = 0;
  children.SetMeshFactory (this);
}

csMeshFactoryWrapper::csMeshFactoryWrapper ()
{
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiMeshFactoryWrapper);
  parent = 0;
  children.SetMeshFactory (this);
}

csMeshFactoryWrapper::~csMeshFactoryWrapper ()
{
  // This line MUST be here to ensure that the children are not
  // removed after the destructor has already finished.
  children.RemoveAll ();
  SCF_DESTRUCT_EMBEDDED_IBASE (scfiMeshFactoryWrapper);
}

void csMeshFactoryWrapper::SetMeshObjectFactory (iMeshObjectFactory *meshFact)
{
  csMeshFactoryWrapper::meshFact = meshFact;
}

iMeshWrapper *csMeshFactoryWrapper::NewMeshObject ()
{
  csRef<iMeshObject> basemesh (meshFact->NewInstance ());
  iMeshWrapper *mesh = &(new csMeshWrapper (0, basemesh))->scfiMeshWrapper;

  if (GetName ()) mesh->QueryObject ()->SetName (GetName ());
  mesh->SetFactory (&scfiMeshFactoryWrapper);

  if (static_lod)
  {
    iLODControl* lod = mesh->CreateStaticLOD ();
    iSharedVariable* varm, * vara;
    static_lod->GetLOD (varm, vara);
    if (varm)
    {
      lod->SetLOD (varm, vara);
    }
    else
    {
      float m, a;
      static_lod->GetLOD (m, a);
      lod->SetLOD (m, a);
    }
  }

  int i;
  for (i = 0; i < children.GetCount (); i++)
  {
    iMeshFactoryWrapper *childfact = children.Get (i);
    iMeshWrapper *child = childfact->CreateMeshWrapper ();
    mesh->GetChildren ()->Add (child);
    child->GetMovable ()->SetTransform (childfact->GetTransform ());
    child->GetMovable ()->UpdateMove ();

    if (static_lod)
    {
      // We have static lod so we need to put the child in the right
      // lod level.
      int l;
      for (l = 0 ; l < static_lod->GetLODCount () ; l++)
      {
        csArray<iMeshFactoryWrapper*>& facts_for_lod =
      	  static_lod->GetMeshesForLOD (l);
        int j;
	for (j = 0 ; j < facts_for_lod.Length () ; j++)
	{
	  if (facts_for_lod[j] == childfact)
	    mesh->AddMeshToStaticLOD (l, child);
	}
      }
    }

    child->DecRef ();
  }

  return mesh;
}

void csMeshFactoryWrapper::HardTransform (const csReversibleTransform &t)
{
  meshFact->HardTransform (t);
}

iLODControl* csMeshFactoryWrapper::CreateStaticLOD ()
{
  static_lod = csPtr<csStaticLODFactoryMesh> (new csStaticLODFactoryMesh ());
  return static_lod;
}

void csMeshFactoryWrapper::DestroyStaticLOD ()
{
  static_lod = 0;
}

iLODControl* csMeshFactoryWrapper::GetStaticLOD ()
{
  return (iLODControl*)static_lod;
}

void csMeshFactoryWrapper::SetStaticLOD (float m, float a)
{
  if (static_lod) static_lod->SetLOD (m, a);
}

void csMeshFactoryWrapper::GetStaticLOD (float& m, float& a) const
{
  if (static_lod)
    static_lod->GetLOD (m, a);
  else
  {
    m = 0;
    a = 0;
  }
}

void csMeshFactoryWrapper::RemoveFactoryFromStaticLOD (
	iMeshFactoryWrapper* fact)
{
  if (!static_lod) return;	// No static lod, nothing to do here.
  int lod;
  for (lod = 0 ; lod < static_lod->GetLODCount () ; lod++)
  {
    csArray<iMeshFactoryWrapper*>& meshes_for_lod =
    	static_lod->GetMeshesForLOD (lod);
    meshes_for_lod.Delete (fact);
  }
}

void csMeshFactoryWrapper::AddFactoryToStaticLOD (int lod,
	iMeshFactoryWrapper* fact)
{
  if (!static_lod) return;	// No static lod, nothing to do here.
  csArray<iMeshFactoryWrapper*>& meshes_for_lod =
  	static_lod->GetMeshesForLOD (lod);
  meshes_for_lod.Push (fact);
}

//--------------------------------------------------------------------------
// csMeshList
//--------------------------------------------------------------------------
SCF_IMPLEMENT_IBASE(csMeshList)
  SCF_IMPLEMENTS_INTERFACE(iMeshList)
SCF_IMPLEMENT_IBASE_END

csMeshList::csMeshList ()
{
  SCF_CONSTRUCT_IBASE (0);
}

csMeshList::~csMeshList ()
{
  RemoveAll ();
  SCF_DESTRUCT_IBASE ();
}

iMeshWrapper* csMeshList::FindByNameWithChild (const char *Name) const
{
  char const* p = strchr (Name, ':');
  if (!p) return list.FindByName (Name);

  int i;
  for (i = 0 ; i < list.Length () ; i++)
  {
    iMeshWrapper* m = list.Get (i);
    if (!strncmp (m->QueryObject ()->GetName (), Name, p-Name))
    {
      return m->GetChildren ()->FindByName (p+1);
    }
  }
  return 0;
}

int csMeshList::Add (iMeshWrapper *obj)
{
  PrepareMesh (obj);
  list.Push (obj);
  return true;
}

bool csMeshList::Remove (iMeshWrapper *obj)
{
  FreeMesh (obj);
  list.Delete (obj);
  return true;
}

bool csMeshList::Remove (int n)
{
  FreeMesh (list[n]);
  list.DeleteIndex (n);
  return true;
}

void csMeshList::RemoveAll ()
{
  int i;
  for (i = 0 ; i < list.Length () ; i++)
  {
    FreeMesh (list[i]);
  }
  list.DeleteAll ();
}

int csMeshList::Find (iMeshWrapper *obj) const
{
  return list.Find (obj);
}

iMeshWrapper *csMeshList::FindByName (const char *Name) const
{
  if (strchr (Name, ':'))
    return FindByNameWithChild (Name);
  else
    return list.FindByName (Name);
}

//--------------------------------------------------------------------------
// csMeshMeshList
//--------------------------------------------------------------------------
void csMeshMeshList::PrepareMesh (iMeshWrapper* child)
{
  CS_ASSERT (mesh != 0);
  csMeshList::PrepareMesh (child);

  // unlink the mesh from the engine or another parent.
  iMeshWrapper *oldParent = child->GetParentContainer ();
  if (oldParent)
    oldParent->GetChildren ()->Remove (child);
  else
    csEngine::current_engine->GetMeshes ()->Remove (child);

  /* csSector->PrepareMesh tells the culler about the mesh
     (since removing the mesh above also removes it from the culler...) */
  // First we find the top-level parent.
  iMeshWrapper* toplevel = &(mesh->scfiMeshWrapper);
  while (toplevel->GetParentContainer ())
    toplevel = toplevel->GetParentContainer ();
  iMovable* mov = toplevel->GetMovable ();
  iSectorList* sl = mov->GetSectors ();
  for (int i = 0 ; i < sl->GetCount() ; i++)
  {
    csSector* sector = sl->Get (i)->GetPrivateObject ();
    sector->UnprepareMesh (child);
    sector->PrepareMesh (child);
  }

  child->SetParentContainer (&mesh->scfiMeshWrapper);
  child->GetMovable ()->SetParent (&mesh->GetCsMovable ().scfiMovable);
}

void csMeshMeshList::FreeMesh (iMeshWrapper* item)
{
  CS_ASSERT (mesh != 0);

  for (int i = 0 ; i < mesh->GetCsMovable().GetSectors()->GetCount() ; i++)
  {
    csSector* sector = mesh->GetCsMovable ().GetSectors ()->Get (i)
        ->GetPrivateObject ();
    sector->UnprepareMesh (item);
  }

  item->SetParentContainer (0);
  item->GetMovable ()->SetParent (0);

  mesh->RemoveMeshFromStaticLOD (item);

  csMeshList::FreeMesh (item);
}

//--------------------------------------------------------------------------
// csMeshFactoryList
//--------------------------------------------------------------------------
SCF_IMPLEMENT_IBASE(csMeshFactoryList)
  SCF_IMPLEMENTS_INTERFACE(iMeshFactoryList)
SCF_IMPLEMENT_IBASE_END

csMeshFactoryList::csMeshFactoryList ()
{
  SCF_CONSTRUCT_IBASE (0);
}

csMeshFactoryList::~csMeshFactoryList ()
{
  RemoveAll ();
  SCF_DESTRUCT_IBASE ();
}

int csMeshFactoryList::Add (iMeshFactoryWrapper *obj)
{
  PrepareFactory (obj);
  list.Push (obj);
  return true;
}

bool csMeshFactoryList::Remove (iMeshFactoryWrapper *obj)
{
  FreeFactory (obj);
  list.Delete (obj);
  return true;
}

bool csMeshFactoryList::Remove (int n)
{
  FreeFactory (list[n]);
  list.Delete (Get (n));
  return true;
}

void csMeshFactoryList::RemoveAll ()
{
  int i;
  for (i = 0 ; i < list.Length () ; i++)
  {
    FreeFactory (list[i]);
  }
  list.DeleteAll ();
}

int csMeshFactoryList::Find (iMeshFactoryWrapper *obj) const
{
  return list.Find (obj);
}

iMeshFactoryWrapper *csMeshFactoryList::FindByName (
  const char *Name) const
{
  return list.FindByName (Name);
}

//--------------------------------------------------------------------------
// csMeshFactoryFactoryList
//--------------------------------------------------------------------------
void csMeshFactoryFactoryList::PrepareFactory (iMeshFactoryWrapper* child)
{
  CS_ASSERT (meshfact != 0);
  csMeshFactoryList::PrepareFactory (child);

  // unlink the factory from another possible parent.
  if (child->GetParentContainer ())
    child->GetParentContainer ()->GetChildren ()->Remove (child);

  child->SetParentContainer (&meshfact->scfiMeshFactoryWrapper);
}

void csMeshFactoryFactoryList::FreeFactory (iMeshFactoryWrapper* item)
{
  CS_ASSERT (meshfact != 0);
  item->SetParentContainer (0);
  meshfact->RemoveFactoryFromStaticLOD (item);
  csMeshFactoryList::FreeFactory (item);
}
