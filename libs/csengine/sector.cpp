/*
    Copyright (C) 1998-2001 by Jorrit Tyberghein

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
#include "qint.h"
#include "csutil/csstring.h"
#include "csutil/hashmap.h"
#include "csengine/dumper.h"
#include "csengine/sector.h"
#include "csengine/thing.h"
#include "csengine/meshobj.h"
#include "csengine/polygon.h"
#include "csengine/pol2d.h"
#include "csengine/polytext.h"
#include "csengine/polytmap.h"
#include "csengine/cscoll.h"
#include "csengine/light.h"
#include "csengine/camera.h"
#include "csengine/engine.h"
#include "csengine/stats.h"
#include "csengine/csppulse.h"
#include "csengine/cbuffer.h"
#include "csengine/quadtr3d.h"
#include "csengine/covtree.h"
#include "csengine/bspbbox.h"
#include "csengine/terrobj.h"
#include "csengine/covcube.h"
#include "csengine/cbufcube.h"
#include "csengine/bsp.h"
#include "csengine/octree.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "ivideo/txtmgr.h"
#include "ivideo/texture.h"
#include "iengine/texture.h"
#include "isys/vfs.h"
#include "iengine/statlght.h"
#include "iengine/viscull.h"
#include "iengine/rview.h"

// Option variable: render portals?
bool csSector::do_portals = true;
// Option variable: render things?
bool csSector::do_things = true;
// Configuration variable: number of allowed reflections for static lighting.
int csSector::cfg_reflections = 1;
// Option variable: do pseudo radiosity?
bool csSector::do_radiosity = false;

//---------------------------------------------------------------------------

IMPLEMENT_CSOBJTYPE (csSector,csPObject);

IMPLEMENT_IBASE_EXT (csSector)
  IMPLEMENTS_EMBEDDED_INTERFACE (iSector)
IMPLEMENT_IBASE_EXT_END

IMPLEMENT_EMBEDDED_IBASE (csSector::eiSector)
  IMPLEMENTS_INTERFACE (iSector)
IMPLEMENT_EMBEDDED_IBASE_END

csSector::csSector (csEngine* engine) : csPObject ()
{
  CONSTRUCT_EMBEDDED_IBASE (scfiSector);
  csSector::engine = engine;
  level_r = level_g = level_b = 0;
  static_thing = NULL;
  engine->AddToCurrentRegion (this);
  fog.enabled = false;
  draw_busy = 0;
  culler = NULL;
}

csSector::~csSector ()
{
  // Meshes, things, and collections are not deleted by the calls below. They
  // belong to csEngine.
  things.DeleteAll ();
  skies.DeleteAll ();
  meshes.DeleteAll ();
  collections.DeleteAll ();

  lights.DeleteAll ();
  terrains.DeleteAll ();
}

void csSector::Prepare ()
{
  int i;
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* th = (csThing*)things[i];
    th->Prepare (this);
  }
  for (i = 0 ; i < skies.Length () ; i++)
  {
    csThing* th = (csThing*)skies[i];
    th->Prepare (this);
  }
}

//----------------------------------------------------------------------

void csSector::AddMesh (csMeshWrapper* mesh)
{
  meshes.Push ((csSome)mesh);
  if (culler)
  {
    iVisibilityObject* vo = QUERY_INTERFACE (mesh, iVisibilityObject);
    vo->DecRef ();
    culler->RegisterVisObject (vo);
  }
}

void csSector::UnlinkMesh (csMeshWrapper* mesh)
{
  int idx = meshes.Find ((csSome)mesh);
  if (idx != -1)
  {
    meshes.Delete (idx);
    if (culler)
    {
      iVisibilityObject* vo = QUERY_INTERFACE (mesh, iVisibilityObject);
      vo->DecRef ();
      culler->UnregisterVisObject (vo);
    }
  }
}

csMeshWrapper* csSector::GetMesh (const char* name)
{
  int i;
  for (i = 0 ; i < meshes.Length () ; i++)
  {
    csMeshWrapper* s = (csMeshWrapper*)meshes[i];
    if (!strcmp (name, s->GetName ()))
      return s;
  }
  return NULL;
}

//----------------------------------------------------------------------

void csSector::AddThing (csThing* thing)
{
  things.Push ((csSome)thing);
  if (culler)
  {
    iVisibilityObject* vo = QUERY_INTERFACE (thing, iVisibilityObject);
    vo->DecRef ();
    culler->RegisterVisObject (vo);
  }
}

void csSector::UnlinkThing (csThing* thing)
{
  int idx = things.Find ((csSome)thing);
  if (idx != -1)
  {
    things.Delete (idx);
    if (culler)
    {
      iVisibilityObject* vo = QUERY_INTERFACE (thing, iVisibilityObject);
      vo->DecRef ();
      culler->UnregisterVisObject (vo);
    }
  }
}

csThing* csSector::GetThing (const char* name)
{
  int i;
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* s = (csThing*)things[i];
    if (!strcmp (name, s->GetName ()))
      return s;
  }
  return NULL;
}

//----------------------------------------------------------------------

void csSector::AddSky (csThing* thing)
{
  skies.Push ((csSome)thing);
  if (culler)
  {
    iVisibilityObject* vo = QUERY_INTERFACE (thing, iVisibilityObject);
    vo->DecRef ();
    culler->RegisterVisObject (vo);
  }
}

void csSector::UnlinkSky (csThing* thing)
{
  int idx = skies.Find ((csSome)thing);
  if (idx != -1)
  {
    skies.Delete (idx);
    if (culler)
    {
      iVisibilityObject* vo = QUERY_INTERFACE (thing, iVisibilityObject);
      vo->DecRef ();
      culler->UnregisterVisObject (vo);
    }
  }
}

csThing* csSector::GetSky (const char* name)
{
  int i;
  for (i = 0 ; i < skies.Length () ; i++)
  {
    csThing* s = (csThing*)skies[i];
    if (!strcmp (name, s->GetName ()))
      return s;
  }
  return NULL;
}

//----------------------------------------------------------------------

void csSector::AddCollection (csCollection* col)
{
  collections.Push ((csSome)col);
}

void csSector::UnlinkCollection (csCollection* col)
{
  int idx = collections.Find ((csSome)col);
  if (idx != -1) collections.Delete (idx);
}

csCollection* csSector::GetCollection (const char* name)
{
  int i;
  for (i = 0 ; i < collections.Length () ; i++)
  {
    csCollection* s = (csCollection*)collections[i];
    if (!strcmp (name, s->GetName ()))
      return s;
  }
  return NULL;
}

//----------------------------------------------------------------------

void csSector::AddLight (csStatLight* light)
{
  lights.Push ((csSome)light);
  light->SetSector (this);
}

void csSector::UnlinkLight (csStatLight* light)
{
  int idx = lights.Find ((csSome)light);
  if (idx != -1) { lights[idx] = NULL; lights.Delete (idx); }
}

csStatLight* csSector::FindLight (float x, float y, float z, float dist)
{
  int i;
  for (i = 0 ; i < lights.Length () ; i++)
  {
    csStatLight* l = (csStatLight*)lights[i];
    if (ABS (x-l->GetCenter ().x) < SMALL_EPSILON &&
        ABS (y-l->GetCenter ().y) < SMALL_EPSILON &&
        ABS (z-l->GetCenter ().z) < SMALL_EPSILON &&
        ABS (dist-l->GetRadius ()) < SMALL_EPSILON)
      return l;
  }
  return NULL;
}

csStatLight* csSector::FindLight (CS_ID id)
{
  int i;
  for (i = 0 ; i < lights.Length () ; i++)
  {
    csStatLight* l = (csStatLight*)lights[i];
    if (l->GetID () == id) return l;
  }
  return NULL;
}

//----------------------------------------------------------------------

void csSector::AddTerrain (csTerrainWrapper *pTerrain)
{
  terrains.Push ((csSome) pTerrain );
}

void csSector::UnlinkTerrain (csTerrainWrapper *pTerrain)
{
  int idx = terrains.Find ((csSome)pTerrain);
  if (idx != -1)
  { 
    terrains[idx] = NULL; 
    terrains.Delete (idx);
  }
}

csTerrainWrapper* csSector::GetTerrain (const char* name)
{
  int i;
  for (i = 0 ; i < terrains.Length () ; i++)
  {
    csTerrainWrapper* s = (csTerrainWrapper*)terrains[i];
    if (!strcmp (name, s->GetName ()))
      return s;
  }
  return NULL;
}

//----------------------------------------------------------------------

void csSector::UseStaticTree (int mode, bool octree)
{
  if (static_thing) return;
  int i;
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* th = (csThing*)things[i];
    if (th->flags.Check (CS_THING_VISTREE))
    {
      static_thing = th;
      static_thing->BuildStaticTree (mode, octree);
      culler = QUERY_INTERFACE (static_thing, iVisibilityCuller);
      culler->DecRef ();
      break;//@@@@@@ Only support one static_thing for now!!!
    }
  }

  // Loop through all things and meshes and update their bounding box in the
  // polygon trees.
  // @@@ Unify when csThing becomes a mesh object.
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* th = (csThing*)things[i];
    th->GetMovable ().UpdateMove ();
  }
  for (i = 0 ; i < meshes.Length () ; i++)
  {
    csMeshWrapper* th = (csMeshWrapper*)meshes[i];
    th->GetMovable ().UpdateMove ();
  }
  
  CsPrintf (MSG_INITIALIZATION, "DONE!\n");
}

csPolygon3D* csSector::HitBeam (const csVector3& start, const csVector3& end,
	csVector3& isect)
{
  csPolygon3D* p = IntersectSegment (start, end, isect);
  if (p)
  {
    csPortal* po = p->GetPortal ();
    if (po)
    {
      draw_busy++;
      csVector3 new_start = isect;
      p = po->HitBeam (new_start, end, isect);
      draw_busy--;
      return p;
    }
    else return p;
  }
  else return NULL;
}

csObject* csSector::HitBeam (const csVector3& start, const csVector3& end,
	csPolygon3D** polygonPtr)
{
  float r, best_mesh_r = 10000000000.;
  csMeshWrapper* near_mesh = NULL;
  csVector3 isect;

  // First check all meshes in this sector.
  int i;
  for (i = 0 ; i < meshes.Length () ; i++)
  {
    csMeshWrapper* mesh = (csMeshWrapper*)meshes[i];
    if (mesh->HitBeam (start, end, isect, &r))
    {
      if (r < best_mesh_r)
      {
        best_mesh_r = r;
	near_mesh = mesh;
      }
    }
  }

  float best_poly_r;
  csPolygon3D* p = IntersectSegment (start, end, isect, &best_poly_r);
  // We hit a polygon and the polygon is closer than the mesh.
  if (p && best_poly_r < best_mesh_r)
  {
    csPortal* po = p->GetPortal ();
    if (po)
    {
      draw_busy++;
      csVector3 new_start = isect;
      csObject* obj = po->HitBeam (new_start, end, polygonPtr);
      draw_busy--;
      return obj;
    }
    else
    {
      if (polygonPtr) *polygonPtr = p;
      return (csObject*)(p->GetParent ());
    }
  }
  // The mesh is closer (or there is no mesh).
  if (polygonPtr) *polygonPtr = NULL;
  return (csObject*)near_mesh;
}

void csSector::CreateLightMaps (iGraphics3D* g3d)
{
  int i;
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* sp = (csThing*)things[i];
    sp->CreateLightMaps (g3d);
  }
  for (i = 0 ; i < skies.Length () ; i++)
  {
    csThing* sp = (csThing*)skies[i];
    sp->CreateLightMaps (g3d);
  }
}


struct ISectData
{
  csSegment3 seg;
  csVector3 isect;
  float r;
};

/*
 * @@@
 * This function does not yet do anything but it should
 * use the PVS as soon as we have that to make csSector::IntersectSegment
 * even faster.
 */
bool IntersectSegmentCull (csPolygonTree* /*tree*/,
	csPolygonTreeNode* node,
	const csVector3& /*pos*/, void* data)
{
  if (!node) return false;
  if (node->Type () != NODE_OCTREE) return true;

  ISectData* idata = (ISectData*)data;
  csOctreeNode* onode = (csOctreeNode*)node;
  csVector3 isect;
  if (csIntersect3::BoxSegment (onode->GetBox (), idata->seg, isect))
    return true;
  // Segment does not intersect with node so we return false here.
  return false;
}

void* IntersectSegmentTestPol (csThing*,
	csPolygonInt** polygon, int num, bool /*same_plane*/, void* data)
{
  ISectData* idata = (ISectData*)data;
  int i;
  for (i = 0 ; i < num ; i++)
  {
    // @@@ What about other types of polygons?
    if (polygon[i]->GetType () == 1)
    {
      csPolygon3D* p = (csPolygon3D*)polygon[i];
      if (p->IntersectSegment (idata->seg.Start (), idata->seg.End (),
      		idata->isect, &(idata->r)))
        return (void*)p;
    }
  }
  return NULL;
}

csPolygon3D* csSector::IntersectSegment (const csVector3& start,
  const csVector3& end, csVector3& isect, float* pr)
{
  float r, best_r = 10000000000.;
  csVector3 cur_isect;
  csPolygon3D* best_p = NULL;
  csVector3 obj_start, obj_end, obj_isect;
  csReversibleTransform movtrans;

  int i;
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* sp = (csThing*)things[i];
    if (sp != static_thing)
    {
      r = best_r;

      if (sp->GetMovingOption () == CS_THING_MOVE_NEVER)
      {
        obj_start = start;
	obj_end = end;
      }
      else
      {
        movtrans = sp->GetMovable ().GetFullTransform ();
        obj_start = movtrans.Other2This (start);
	obj_end = movtrans.Other2This (end);
      }
      csPolygon3D* p = sp->IntersectSegment (obj_start, obj_end, obj_isect, &r);
      if (sp->GetMovingOption () == CS_THING_MOVE_NEVER)
        cur_isect = obj_isect;
      else
        cur_isect = movtrans.This2Other (obj_isect);

      if (p && r < best_r)
      {
        best_r = r;
	best_p = p;
	isect = cur_isect;
      }
    }
  }

  if (static_thing)
  {
    // Static_thing has option CS_THING_MOVE_NEVER so
    // object space == world space.
    csPolygonTree* static_tree = static_thing->GetStaticTree ();
    // Handle the octree.
    ISectData idata;
    idata.seg.Set (start, end);
    void* rc = static_tree->Front2Back (start, IntersectSegmentTestPol,
      (void*)&idata, IntersectSegmentCull, (void*)&idata);
    if (rc && idata.r < best_r)
    {
      best_r = idata.r;
      best_p = (csPolygon3D*)rc;
      isect = idata.isect;
    }
  }


  if (pr) *pr = best_r;
  return best_p;
}

csSector* csSector::FollowSegment (csReversibleTransform& t,
  csVector3& new_position, bool& mirror)
{
  csVector3 isect;
  csPolygon3D* p = IntersectSegment (t.GetOrigin (), new_position, isect);
  csPortal* po;

  if (p)
  {
    po = p->GetPortal ();
    if (po)
    {
      if (!po->GetSector ()) po->CompleteSector ();
      if (po->flags.Check (CS_PORTAL_WARP))
      {
        po->WarpSpace (t, mirror);
	new_position = po->Warp (new_position);
      }
      csSector* dest_sect = po->GetSector ();
      return dest_sect ? dest_sect->FollowSegment (t, new_position, mirror) : (csSector*)NULL;
    }
    else
      new_position = isect;
  }

  return this;
}

csPolygon3D* csSector::IntersectSphere (csVector3& center, float radius,
  float* pr)
{
  float d, min_d = radius;
  int i;
  csPolygon3D* p, * min_p = NULL;
  csVector3 obj_center;
  csReversibleTransform movtrans;

  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* sp = (csThing*)things[i];
    if (sp->GetMovingOption () == CS_THING_MOVE_NEVER)
      obj_center = center;
    else
    {
      movtrans = sp->GetMovable ().GetFullTransform ();
      obj_center = movtrans.Other2This (center);
    }
    p = sp->IntersectSphere (obj_center, radius, &d);
    if (p && d < min_d)
    {
      min_d = d;
      min_p = p;
    }
  }

  if (pr) *pr = min_d;
  return min_p;
}

int compare_z_thing (const void* p1, const void* p2)
{
  csThing* sp1 = *(csThing**)p1;
  csThing* sp2 = *(csThing**)p2;
  float z1 = sp1->Vcam (sp1->GetCenter ()).z;
  float z2 = sp2->Vcam (sp2->GetCenter ()).z;
  if (z1 < z2) return -1;
  else if (z1 > z2) return 1;
  return 0;
}

// @@@ THE NOTES BELOW ARE MOSTLY OBSOLETE NOW. I DON'T REMOVE THEM
// BECAUSE THERE IS STILL A GRAIN OF USEFUL INFORMATION IN THEM.
//
// Some notes about drawing here. These notes are the start for
// a rethinking about how rendering objects in one sector actually
// should happen. Note that the current implementation actually
// implements very little of the things discussed here. Currently
// the entities are just rendered one after the other which can cause
// some problems.
//
// There are a few issues here:
//
// 1. Z-buffering/Z-filling.
// Some objects/entities are more efficiently rendered back
// to front using Z-filling instead of Z-buffering. In some cases
// Z-filling is also required because rendering a sector starts
// with an uninitialized Z-buffer (CS normally doesn't clear the
// Z buffer every frame). In some cases it might be more optimal
// to use Z buffering in any case (to avoid sorting back to front)
// (for hardware 3D) so we would like to have the option to clear
// the Z buffer every frame and use Z-buffering.
//
// 2. Alpha transparency.
// Some entities have alpha transparency. Alpha transparent surfaces
// actually need to be sorted back to front to render correctly.
// Also before rendering an alpha surface all objects behind it should
// already be rendered.
//
// 3. Floating portals.
// Floating portals also take some special consideration. First
// of all the assume a new intialize of the Z buffer for the 2D
// area of the portal in question. This is ok if the first entities
// that are rendered through the portal use Z-fill and cover the
// entire portal (this is the case if you use sector walls for
// example). If Z-fill cannot be used for the portal then an
// extra initial pass would have to clear the Z buffer for the portal
// area in 2D. Also geometry needs to be clipped in 3D if you have
// a floating portal. The reason is that the Z buffer information
// outside of the floating portal may actually contain information
// further than the contents of the portal. This would cause entities
// visible inside the portal to be rendered as if they are in the
// parent sector too.
// After rendering through a floating portal, the floating portal
// itself needs to be covered by the Z-buffer. i.e. we need to make
// sure that the Z-buffer thinks the portal is a regular polygon.
// This is to make sure that meshes or other entities rendered
// afterwards will not get rendered INSIDE the portal contents.
//
// Here is a list of all the entities that we can draw in a sector:
//
// 1. Sector walls.
// Sectors are always convex. So sectors walls are ideal for rendering
// first through Z-fill.
//
// 2. Static things in octree.
// In some cases all static things are collected into one big
// octree with mini-bsp trees. This structure ensures that we can
// actually easily sort polygon back to front or front to back if
// needed. This structure can also easily be rendered using Z-fill.
// The c-buffer/coverage mask tree can also be used to detect
// visibility before rendering. This pushes visible polygons into
// a queue. There is the issue here that it should be possible
// to ignore the mini-bsp trees and only use the octree information.
// This can be done on hardware where Z-buffering is fast. This
// of course implies either the use of a Z-filled sector or else
// a clear of the Z buffer every frame.
// A related issue is when there are portals between the polygons.
// Those portals need to be handled as floating portals (i.e. geometry
// needs to be clipped in 3D) because the Z buffer information
// will not be correct. If rendering the visible octree polygons
// back to front then rendering through the portals presents no
// other difficulties.
//
// 3. Terrain triangles.
// The terrain engine generates a set of triangles. These triangles
// can easily be sorted back to front so they are also suitable for
// Z-fill rendering. However, this conflicts with the use of the
// static octree. You cannot use Z-fill for both because that could
// cause wrong rendering. Using Z-buffer for one of them might be
// expensive but the only solution. Here there is also the issue
// if it isn't possible to combine visibility algorithms for landscape
// and octree stuff. i.e. cull octree nodes if occluded by a part
// of the landscape.
//
// 4. 3D Sprites.
// Sprites are entities that need to be rendered using the Z-buffer
// because the triangles cannot easily be sorted.
//
// 5. Dynamic things.
// Things that are not part of the static octree are handled much
// like normal 3D sprites. The most important exception is when
// such a thing has a floating portal. In this case all the normal
// floating portal issues are valid. However, there are is an important
// issue here: if you are rendering a floating portal that is BEHIND
// an already rendered entity then there is a problem. The contents
// of the portal may actually use Z-fill and thus would overrender
// the entity in front. One obvious solution is to sort ALL entities
// to make sure that everything is rendered back to front. That's of
// course not always efficient and easy to do. Also it is not possible
// in all cases to do it 100% correct (i.e. complex sprites with
// skeletal animation and so on). The ideal solution would be to have
// a way to clear the Z-buffer for an invisible polygon but only
// where the polygon itself is visible according to the old Z-buffer
// values. This is possible with software but I'm currently unsure
// about hardware. With such a routine you could draw the floating
// portal at any time you want. First you clear the Z-buffer for the
// visible area. Then you force Z-buffer use for the contents inside
// (i.e. everything normally rendered using Z-fill will use Z-buffer
// instead), then you render. Finally you update the Z-buffer with
// the Z-value of the polygon to make it 'hard'.
//
// If we can treat floating portals this way then we can in fact
// consider them as normal polygons that behave correctly for the
// Z buffer. Aside from the fact that they clip geometry in 3D
// that passes through the portal. Note that 3D sprites don't
// currently support 3D geometry clipping yet.

void csSector::Draw (iRenderView* rview)
{
  draw_busy++;
  int i;
  iCamera* icam = rview->GetCamera ();
  rview->SetThisSector (&scfiSector);

  G3D_FOGMETHOD fogmethod = G3DFOGMETHOD_NONE;

  //@@@@@ EDGESif (rview.GetCallback ())
  //{
    //rview.CallCallback (CALLBACK_SECTOR, (void*)this);
  //}
  /*else*/ if (HasFog ())
  {
    if ((fogmethod = csEngine::current_engine->fogmethod) == G3DFOGMETHOD_VERTEX)
    {
      csFogInfo* fog_info = new csFogInfo ();
      fog_info->next = rview->GetFirstFogInfo ();
      iPolygon3D* ipoly3d = rview->GetPortalPolygon ();
      if (ipoly3d)
      {
        fog_info->incoming_plane = ipoly3d->GetCameraPlane ();
        fog_info->incoming_plane.Invert ();
	fog_info->has_incoming_plane = true;
      }
      else fog_info->has_incoming_plane = false;
      fog_info->fog = &GetFog ();
      fog_info->has_outgoing_plane = true;
      rview->SetFirstFogInfo (fog_info);
    }
    else if (fogmethod != G3DFOGMETHOD_NONE)
    {
      rview->GetGraphics3D ()->OpenFogObject (GetID (), &GetFog ());
    }
  }

  // First draw all 'sky' things using Z-fill.
  for (i = 0 ; i < skies.Length () ; i++)
  {
    csThing* th = (csThing*)skies[i];
    th->Draw (rview, &th->GetMovable ().scfiMovable);
  }

  // In some cases this queue will be filled with all visible
  // meshes.
  csMeshWrapper** mesh_queue = NULL;
  int num_mesh_queue = 0;
  // For things we have a similar queue.
  csThing** thing_queue = NULL;
  int num_thing_queue = 0;
  // If the following flag is true the queues are actually used.
  bool use_object_queues = false;

  // If we have a visibility culler in this sector we use it here.
  if (culler)
  {
    if (culler->VisTest (rview))
    {
      // The visibility culler worked and marked all registered
      // visible things as visible.

      // Fill the mesh and thing queues for all meshes and things
      // that were visible.
      use_object_queues = true;
      if (meshes.Length () > 0)
      {
	// Push all visible meshes in a queue.
	// @@@ Avoid memory allocation?
	mesh_queue = new csMeshWrapper* [meshes.Length ()];
	num_mesh_queue = 0;
        for (i = 0 ; i < meshes.Length () ; i++)
        {
          csMeshWrapper* sp = (csMeshWrapper*)meshes[i];
	  if (sp->IsVisible ()) mesh_queue[num_mesh_queue++] = sp;
	}
      }
      if (things.Length () > 0)
      {
        // Push all visible things in a queue.
	// @@@ Avoid memory allocation?
	thing_queue = new csThing* [things.Length ()];
	num_thing_queue = 0;
	for (i = 0 ; i < things.Length () ; i++)
	{
	  csThing* th = (csThing*)things[i];
	  if (th->IsVisible ()) thing_queue[num_thing_queue++] = th;
	}
      }
    }
    else
    {
      // The visibility culler was either disabled or decided visibility
      // culling was not useful given some circumstances. In this case
      // all objects should be considered visible.
    }
  }

  // If we have a static thing we draw it here.
  if (static_thing)
    static_thing->Draw (rview, &static_thing->GetMovable ().scfiMovable);

  if (do_things)
  {
    // If the queues are not used for things we still fill the queue here
    // just to make the code below easier.
    if (!use_object_queues)
    {
      num_thing_queue = 0;
      if (things.Length ())
      {
        thing_queue = new csThing* [things.Length ()];
        for (i = 0 ; i < things.Length () ; i++)
        {
          csThing* th = (csThing*)things[i];
          thing_queue[num_thing_queue++] = th;
        }
      }
      else
        thing_queue = NULL;
    }

    // All csThings which are not merged with the static bsp still need to
    // be drawn. Unless they are fog objects (or transparent, this is a todo!)
    // we just render them using the Z-buffer. Fog or transparent objects
    // are z-sorted and rendered back to front.
    //
    // We should see if there are better alternatives to Z-sort which are
    // more accurate in more cases (@@@).
    csThing* sort_list[256];    // @@@HARDCODED == BAD == EASY!
    int sort_idx = 0;
    int i;

    // First we do z-sorting for fog objects so that they are rendered
    // correctly from back to front. All other objects are drawn using
    // the z-buffer.
    for (i = 0 ; i < num_thing_queue ; i++)
    {
      csThing* th = thing_queue[i];
      if (th != static_thing)
        if (th->GetFog ().enabled) sort_list[sort_idx++] = th;
        else th->Draw (rview, &th->GetMovable ().scfiMovable);
    }

    if (sort_idx)
    {
      // Now sort the objects in sort_list.
      qsort (sort_list, sort_idx, sizeof (csThing*), compare_z_thing);

      // Draw them back to front.
      for (i = 0 ; i < sort_idx ; i++)
      {
        csThing* th = sort_list[i];
        if (th->GetFog ().enabled) th->DrawFoggy (rview, &th->GetMovable ().scfiMovable);
        else th->Draw (rview, &th->GetMovable ().scfiMovable);
      }
    }

    delete [] thing_queue;

    // Draw meshes.
    // To correctly support meshes in multiple sectors we only draw a
    // mesh if the mesh is not in the sector we came from. If the
    // mesh is also present in the previous sector then we will still
    // draw it in any of the following cases:
    //    - the previous sector has fog
    //    - the portal we just came through has alpha transparency
    //    - the portal is a portal on a thing (i.e. a floating portal)
    //    - the portal does space warping
    // In those cases we draw the mesh anyway. @@@ Note that we should
    // draw it clipped (in 3D) to the portal polygon. This is currently not
    // done.
    iSector* previous_sector = rview->GetPreviousSector ();

    int spr_num;
    if (mesh_queue) spr_num = num_mesh_queue;
    else spr_num = meshes.Length ();

    if (rview->AddedFogInfo ())
      rview->GetFirstFogInfo ()->has_outgoing_plane = false;

    for (i = 0 ; i < spr_num ; i++)
    {
      csMeshWrapper* sp;
      if (mesh_queue) sp = mesh_queue[i];
      else sp = (csMeshWrapper*)meshes[i];

      if (!previous_sector ||
      		sp->GetMovable ().GetSectors ().Find (previous_sector->GetPrivateObject ()) == -1)
      {
        // Mesh is not in the previous sector or there is no previous sector.
        sp->Draw (rview);
      }
      else
      {
        if (
	  previous_sector->HasFog () ||
	  rview->GetPortalPolygon ()->IsTransparent () ||
	  rview->GetPortalPolygon ()->GetPortal ()->GetFlags ().Check (CS_PORTAL_WARP))
	{
	  // @@@ Here we should draw clipped to the portal.
          sp->Draw (rview);
	}
      }
    }
    delete [] mesh_queue;
  }

  // Draw all terrain surfaces.
  if (terrains.Length () > 0)
  {
    for (i = 0 ; i < terrains.Length () ; i++)
    {
      csTerrainWrapper* pTerrain = (csTerrainWrapper*)terrains[i];
      pTerrain->Draw (rview, true);
    }
  }

  // queue all halos in this sector to be drawn.
  //@@@@ EDGES if (!rview.GetCallback ())
    for (i = lights.Length () - 1; i >= 0; i--)
      // Tell the engine to try to add this light into the halo queue
      csEngine::current_engine->AddHalo ((csLight *)lights.Get (i));

  // Handle the fog, if any
  if (fogmethod != G3DFOGMETHOD_NONE)
  {
    G3DPolygonDFP g3dpoly;
    if (fogmethod == G3DFOGMETHOD_ZBUFFER)
    {
      g3dpoly.num = rview->GetClipper ()->GetNumVertices ();
      csVector2 *clipview = rview->GetClipper ()->GetClipPoly ();
      memcpy (g3dpoly.vertices, clipview, g3dpoly.num * sizeof (csVector2));
      if (icam->GetSector () == &scfiSector && draw_busy == 0)
      {
        // Since there is fog in the current camera sector we simulate
        // this by adding the view plane polygon.
        rview->GetGraphics3D ()->DrawFogPolygon (GetID (), g3dpoly, CS_FOG_VIEW);
      }
      else
      {
        // We must add a FRONT fog polygon for the clipper to this sector.
        rview->GetClipPlane (g3dpoly.normal);
	g3dpoly.normal.Invert ();
        g3dpoly.inv_aspect = icam->GetInvFOV ();
        rview->GetGraphics3D ()->DrawFogPolygon (GetID (), g3dpoly, CS_FOG_FRONT);
      }
    }
    else if (fogmethod == G3DFOGMETHOD_VERTEX && rview->AddedFogInfo ())
    {
      csFogInfo *fog_info = rview->GetFirstFogInfo ();
      rview->SetFirstFogInfo (rview->GetFirstFogInfo ()->next);
      delete fog_info;
    }
  }

  //@@@@@ EDGES if (rview.GetCallback ()) rview.CallCallback (CALLBACK_SECTOREXIT, (void*)this);

  draw_busy--;
}

csObject** csSector::GetVisibleObjects (csFrustumView& lview, int& num_objects)
{
  csFrustum* lf = lview.GetFrustumContext ()->GetLightFrustum ();
  bool infinite = lf->IsInfinite ();
  csVector3& c = lf->GetOrigin ();
  bool vis;
  int i, i1;
  int j;

  num_objects = things.Length () + meshes.Length ();
  if (!num_objects) { return NULL; }
  csObject** visible_objects = new csObject* [num_objects];

  num_objects = 0;
  // @@@ Unify both loops below once csThing becomes a mesh object.
  for (j = 0 ; j < things.Length () ; j++)
  {
    csThing* sp = (csThing*)things[j];
    // If the light frustum is infinite then every thing
    // in this sector is of course visible.
    if (infinite) vis = true;
    else
    {
      csBox3 bbox;
      sp->GetBoundingBox (&(sp->GetMovable ().scfiMovable), bbox);
      // Here we do a quick test to see if the bounding box is visible in
      // in the frustum. This test is not complete in the sense that it will
      // say that some bounding boxes are visible even if they are not. But
      // it is correct in the sense that if it says a bounding box
      // is invisible, then it certainly is invisible.
      //
      // It works by taking all vertices of the bounding box. If
      // ALL of them are on the outside of the same plane from the
      // frustum then the object is certainly not visible.
      vis = true;
      i1 = lf->GetNumVertices ()-1;
      for (i = 0 ; i < lf->GetNumVertices () ; i1 = i, i++)
      {
        csVector3& v1 = lf->GetVertex (i);
        csVector3& v2 = lf->GetVertex (i1);
        if (csMath3::WhichSide3D (bbox.GetCorner (0)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (1)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (2)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (3)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (4)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (5)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (6)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (7)-c, v1, v2) < 0) continue;
        // Here we have a case of all vertices of the bbox being on the
        // outside of the same plane.
        vis = false;
        break;
      }
      if (vis && lf->GetBackPlane ())
      {
        // If still visible then we can also check the back plane.
        // @@@ NOTE THIS IS UNTESTED CODE. LIGHT_FRUSTUMS CURRENTLY DON'T
        // HAVE A BACK PLANE YET.
        if (!csMath3::Visible (bbox.GetCorner (0)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (1)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (2)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (3)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (4)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (5)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (6)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (7)-c, *lf->GetBackPlane ()))
          vis = false;
      }
    }

    if (vis) visible_objects[num_objects++] = sp;
  }
  for (j = 0 ; j < meshes.Length () ; j++)
  {
    csMeshWrapper* sp = (csMeshWrapper*)meshes[j];
    // If the light frustum is infinite then every thing
    // in this sector is of course visible.
    if (infinite) vis = true;
    else
    {
      csBox3 bbox;
      sp->GetWorldBoundingBox (bbox);
      // Here we do a quick test to see if the bounding box is visible in
      // in the frustum. This test is not complete in the sense that it will
      // say that some bounding boxes are visible even if they are not. But
      // it is correct in the sense that if it says a bounding box
      // is invisible, then it certainly is invisible.
      //
      // It works by taking all vertices of the bounding box. If
      // ALL of them are on the outside of the same plane from the
      // frustum then the object is certainly not visible.
      vis = true;
      i1 = lf->GetNumVertices ()-1;
      for (i = 0 ; i < lf->GetNumVertices () ; i1 = i, i++)
      {
        csVector3& v1 = lf->GetVertex (i);
        csVector3& v2 = lf->GetVertex (i1);
        if (csMath3::WhichSide3D (bbox.GetCorner (0)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (1)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (2)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (3)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (4)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (5)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (6)-c, v1, v2) < 0) continue;
        if (csMath3::WhichSide3D (bbox.GetCorner (7)-c, v1, v2) < 0) continue;
        // Here we have a case of all vertices of the bbox being on the
        // outside of the same plane.
        vis = false;
        break;
      }
      if (vis && lf->GetBackPlane ())
      {
        // If still visible then we can also check the back plane.
        // @@@ NOTE THIS IS UNTESTED CODE. LIGHT_FRUSTUMS CURRENTLY DON'T
        // HAVE A BACK PLANE YET.
        if (!csMath3::Visible (bbox.GetCorner (0)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (1)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (2)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (3)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (4)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (5)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (6)-c, *lf->GetBackPlane ()) &&
            !csMath3::Visible (bbox.GetCorner (7)-c, *lf->GetBackPlane ()))
          vis = false;
      }
    }

    if (vis) visible_objects[num_objects++] = sp;
  }
  return visible_objects;
}

void csSector::CheckFrustum (csFrustumView& lview)
{
  csCBufferCube* cb = engine->GetCBufCube ();
  csCovcube* cc = engine->GetCovcube ();
  if (cb) cb->MakeEmpty ();
  else cc->MakeEmpty ();
  RealCheckFrustum (lview);
}

void csSector::RealCheckFrustum (csFrustumView& lview)
{
  if (draw_busy > cfg_reflections) return;
  draw_busy++;

  int i;

  // Translate this sector so that it is oriented around
  // the position of the light (position of the light becomes
  // the new origin).
  csVector3& center = lview.GetFrustumContext ()->GetLightFrustum ()->
  	GetOrigin ();

  iShadowBlockList* shadows = lview.GetFrustumContext ()->GetShadows ();

  // Remember the previous last shadow so that we can remove all
  // shadows that are added in this routine.
  iShadowBlock* previous_last = shadows->GetLastShadowBlock ();

  if (culler && culler->SupportsShadowCasting ())
  {
    culler->CastShadows ((iFrustumView*)&lview);
  }
  else
  {
    // Here we have no octree so we know the sector polygons are
    // convex. First find all objects that are visible in the frustum.
    int num_visible_objects;
    csObject** visible_objects = GetVisibleObjects (lview, num_visible_objects);

    // Append the shadows for these objects to the shadow list.
    // This list is appended to the one given in 'lview'. After
    // returning, the list in 'lview' will be restored.
    if (lview.ThingShadowsEnabled ())
    {
      for (i = 0 ; i < num_visible_objects ; i++)
      {
        // @@@ unify with other mesh objects as soon as possible
        csObject* o = visible_objects[i];
	if (o->GetType () >= csThing::Type)
	{
          csThing* sp = (csThing*)o;
	  // Only if the thing has right flags do we consider it for shadows.
	  if (lview.CheckShadowMask (sp->flags.Get ()))
	    sp->AppendShadows (shadows, center);
        }
      }
    }

    // Calculate lighting for all objects in the current sector.
    for (i = 0 ; i < num_visible_objects ; i++)
    {
      // @@@ unify with other mesh objects as soon as possible
      csObject* o = visible_objects[i];
      if (o->GetType () >= csThing::Type)
      {
        csThing* sp = (csThing*)o;
        if (lview.CheckProcessMask (sp->flags.Get ()))
          sp->RealCheckFrustum (lview);
      }
    }
      
    delete [] visible_objects;
  }

  // Restore the shadow list in 'lview' and then delete
  // all the shadow frustums that were added in this recursion
  // level.
  while (shadows->GetLastShadowBlock () != previous_last)
  {
    iShadowBlock* sh = shadows->GetLastShadowBlock ();
    shadows->RemoveLastShadowBlock ();
    sh->DecRef ();
  }

  draw_busy--;
}

void csSector::InitLightMaps (bool do_cache)
{
  int i;
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* sp = (csThing*)things[i];
    sp->InitLightMaps (do_cache);
  }
  for (i = 0 ; i < skies.Length () ; i++)
  {
    csThing* sp = (csThing*)skies[i];
    sp->InitLightMaps (do_cache);
  }
}

void csSector::CacheLightMaps ()
{
  int i;
  for (i = 0 ; i < things.Length () ; i++)
  {
    csThing* sp = (csThing*)things[i];
    sp->CacheLightMaps ();
  }
  for (i = 0 ; i < skies.Length () ; i++)
  {
    csThing* sp = (csThing*)skies[i];
    sp->CacheLightMaps ();
  }
}

void csSector::ShineLights (csProgressPulse* pulse)
{
  for (int i = 0 ; i < lights.Length () ; i++)
  {
    if (pulse != 0)
      pulse->Step();
    ((csStatLight*)lights[i])->CalculateLighting ();
  }
}

void csSector::ShineLights (csThing* th, csProgressPulse* pulse)
{
  for (int i = 0 ; i < lights.Length () ; i++)
  {
    if (pulse != 0)
      pulse->Step();
    ((csStatLight*)lights[i])->CalculateLighting (th);
  }
}

void csSector::CalculateSectorBBox (csBox3& bbox,
	bool do_things, bool do_meshes, bool /*do_terrain*/)
{
  bbox.StartBoundingBox ();
  csBox3 b;
  int i;
  if (do_things)
    for (i = 0 ; i < things.Length () ; i++)
    {
      csThing* th = (csThing*)things[i];
      th->GetBoundingBox (&(th->GetMovable ().scfiMovable), b);
      bbox += b;
    }
  if (do_meshes)
    for (i = 0 ; i < meshes.Length () ; i++)
    {
      csMeshWrapper* mesh = (csMeshWrapper*)meshes[i];
      mesh->GetTransformedBoundingBox (mesh->GetMovable ().GetTransform (), b);
      bbox += b;
    }
  // @@@ if (do_terrain) not yet supported
}

//---------------------------------------------------------------------------

iThing *csSector::eiSector::GetSkyThing (const char *name)
{
  return &scfParent->GetSky (name)->scfiThing;
}

iThing *csSector::eiSector::GetSkyThing (int iIndex)
{
  return &((csThing*)(scfParent->skies[iIndex]))->scfiThing;
}

iThing *csSector::eiSector::GetThing (const char *name)
{
  return &scfParent->GetThing (name)->scfiThing;
}

iThing *csSector::eiSector::GetThing (int iIndex)
{
  return &((csThing*)(scfParent->things[iIndex]))->scfiThing;
}

void csSector::eiSector::AddTerrain (iTerrainWrapper *pTerrain)
{
  scfParent->AddTerrain (pTerrain->GetPrivateObject ());
}

void csSector::eiSector::AddLight (iStatLight *light)
{
  scfParent->AddLight (light->GetPrivateObject ());
}

iStatLight *csSector::eiSector::FindLight (float x, float y, float z, float dist)
{
  return &scfParent->FindLight (x, y, z, dist)->scfiStatLight;
}
