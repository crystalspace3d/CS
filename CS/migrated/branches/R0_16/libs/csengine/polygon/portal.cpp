/*
    Copyright (C) 1998 by Jorrit Tyberghein
  
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
#include "csengine/portal.h"
#include "csengine/polygon.h"
#include "csengine/pol2d.h"
#include "csengine/sector.h"
#include "csengine/world.h"
#include "csengine/tranman.h"
#include "csengine/stats.h"
#include "itexture.h"

csPortal::csPortal ()
{
  cfg_alpha = 0;
  do_warp_space = false;
  do_mirror = false;
  filter_texture = NULL;
  filter_r = 1;
  filter_g = 1;
  filter_b = 1;
  static_dest = false;
  sector = NULL;
  do_clip_portal = false;
}

void csPortal::ObjectToWorld (const csReversibleTransform& t)
{
  if (static_dest)
    warp_wor = warp_obj * t;
  else
    warp_wor = t.GetInverse () * warp_obj * t;
}

void csPortal::SetWarp (const csTransform& t)
{
  do_warp_space = true;
  warp_obj = t;
  csMatrix3 m = warp_obj.GetO2T ();
  do_mirror = ( ( ( m.Col1() % m.Col2() ) * m.Col3() ) < 0 );

  warp_wor = warp_obj;
}

void csPortal::SetWarp (const csMatrix3& m_w, const csVector3& v_w_before, const csVector3& v_w_after)
{
  do_warp_space = true;

  warp_obj = csTransform (m_w.GetInverse (), v_w_after - m_w * v_w_before);

  // If the three colunms of the transformation matrix are taken
  // as vectors, V1, V2, and V3, then V1 x V2 = ( + or - ) V3.
  // The result is positive for non-mirroring transforms, and
  // negative for mirroring transforms.  Thus, (V1 x V2) * V3 
  // will equal +1 or -1, depending on whether the transform is
  // mirroring.
  csMatrix3 m = warp_obj.GetO2T ();
  do_mirror = ( ( ( m.Col1() % m.Col2() ) * m.Col3() ) < 0 );

  warp_wor = warp_obj;
}

void csPortal::WarpSpace (csReversibleTransform& t, bool& mirror)
{
  // warp_wor is a world -> warp space transformation.
  // t is a world -> camera space transformation.
  // Set t to equal a warp -> camera space transformation by
  // reversing warp and then applying the old t.
  t /= warp_wor;
  if (do_mirror) mirror = !mirror;
}

bool csPortal::Draw (csPolygon2D* new_clipper, csPolygon3D* portal_polygon,
	csRenderView& rview)
{
  if (sector->draw_busy >= 5)
    return false;

  Stats::portals_drawn++;
  if (!new_clipper->GetNumVertices ())
    return false;

  csPolygonClipper new_view (new_clipper, rview.IsMirrored (), true);

  csRenderView new_rview = rview;
  new_rview.view = &new_view;
  new_rview.added_fog_info = false;
  new_rview.portal_polygon = portal_polygon;
  new_rview.clip_plane = portal_polygon->GetPlane ()->GetCameraPlane();
  new_rview.clip_plane.Invert ();
  if (do_clip_portal) new_rview.do_clip_plane = true;

  csTranCookie old_cookie = 0;
  if (do_warp_space)
  {
    bool mirror = new_rview.IsMirrored ();
    WarpSpace (new_rview, mirror);
    new_rview.SetMirrored (mirror);
    old_cookie = csWorld::current_world->tr_manager.NewCameraFrame ();
  }

  sector->Draw (new_rview);

  if (do_warp_space)
    csWorld::current_world->tr_manager.RestoreCameraFrame (old_cookie);

  return true;
}

csPolygon3D* csPortal::HitBeam (const csVector3& start, const csVector3& end,
	csVector3& isect)
{
  if (sector->draw_busy >= 5)
    return NULL;
  if (do_warp_space)
  {
    csVector3 new_start = warp_wor.Other2This (start);
    csVector3 new_end = warp_wor.Other2This (end);
    csVector3 new_isect;
    csPolygon3D* p = sector->HitBeam (new_start, new_end, new_isect);
    if (p)
      isect = warp_wor.This2Other (new_isect);
    return p;
  }
  else return sector->HitBeam (start, end, isect);
}

csPolygon3D* csPortal::IntersectSphere (csVector3& center, float radius, float* pr)
{
  return sector->IntersectSphere (center, radius, pr);
}

csSector* csPortal::FollowSegment (csReversibleTransform& t,
				  csVector3& new_position, bool& mirror)
{
  if (do_warp_space)
  {
    WarpSpace (t, mirror); 
    new_position = warp_wor.Other2This (new_position);
  }
  return sector ? sector->FollowSegment (t, new_position, mirror) : (csSector*)NULL;
}

void csPortal::CheckFrustum (csFrustumView& lview)
{
  if (sector->draw_busy > csSector::cfg_reflections) return;

  csFrustumView new_lview = lview;
  if (lview.light_frustum)
    new_lview.light_frustum = new csFrustum (*lview.light_frustum);

  // If copied_frustums is true we copied the frustums and we need to delete them
  // later.
  bool copied_frustums = false;

  csTranCookie old_cookie = 0;
  if (do_warp_space)
  {
    old_cookie = csWorld::current_world->tr_manager.NewCameraFrame ();
    new_lview.light_frustum->Transform (&warp_wor);

    if (do_mirror) new_lview.mirror = !lview.mirror;
    new_lview.light_frustum->SetMirrored (new_lview.mirror);

    // Transform all shadow frustums. First make a copy.
    // Note that we only copy the relevant shadow frustums.
    // We know that csPolygon3D::CalculateLighting() called
    // csPolygon3D::MarkRelevantShadowFrustums() some time before
    // calling this function so the 'relevant' flags are still valid.
    new_lview.shadows.Clear ();	// Don't delete elements.
    csShadowFrustum* sf, * copy_sf;
    sf = lview.shadows.GetFirst ();
    while (sf)
    {
      if (sf->relevant)
      {
        copy_sf = new csShadowFrustum (*sf);
        new_lview.shadows.AddLast (copy_sf);
      }
      sf = sf->next;
    }
    copied_frustums = true;
    new_lview.shadows.Transform (&warp_wor);

    if (cfg_alpha)
    {
      //float a = cfg_alpha;
      //new_lview.r = (a * lview.r + (100-a) * filter_r) / 100.;
      //new_lview.g = (a * lview.g + (100-a) * filter_g) / 100.;
      //new_lview.b = (a * lview.b + (100-a) * filter_b) / 100.;
      float fr, fg, fb;
      if (filter_texture)
      {
        UByte mr, mg, mb;
        filter_texture->GetMeanColor (mr, mg, mb);
        fr = mr / 255.; fg = mg / 255.; fb = mb / 255.;
      }
      else
      {
        fr = filter_r;
        fg = filter_g;
        fb = filter_b;
      }
      new_lview.r = lview.r * fr;
      new_lview.g = lview.g * fg;
      new_lview.b = lview.b * fb;
    }

    // Don't go further if the light intensity is almost zero.
    if (new_lview.r < SMALL_EPSILON && new_lview.g < SMALL_EPSILON && new_lview.b < SMALL_EPSILON)
      return;
  }
  else if (lview.shadows.GetFirst ())
  {
    // There is no space warping. In this case we still want to
    // remove all non-relevant shadow frustums if there are any.
    // We know that csPolygon3D::CalculateLighting() called
    // csPolygon3D::MarkRelevantShadowFrustums() some time before
    // calling this function so the 'relevant' flags are still valid.
    new_lview.shadows.Clear ();	// Don't delete elements.
    csShadowFrustum* sf, * copy_sf;
    sf = lview.shadows.GetFirst ();
    while (sf)
    {
      if (sf->relevant)
      {
        copy_sf = new csShadowFrustum (*sf);
        new_lview.shadows.AddLast (copy_sf);
      }
      sf = sf->next;
    }
    copied_frustums = true;
  }

  sector->RealCheckFrustum (new_lview);

  if (do_warp_space)
    csWorld::current_world->tr_manager.RestoreCameraFrame (old_cookie);

  if (copied_frustums)
  {
    // Delete all copied frustums.
    new_lview.shadows.DeleteFrustums ();
    new_lview.shadows.Clear ();
  }
}


