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

#include "sysdef.h"
#include "csengine/polygon/portal.h"
#include "csengine/polygon/polygon.h"
#include "csengine/polygon/pol2d.h"
#include "csengine/sector.h"
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

bool csPortalCS::Draw (csPolygon2D* new_clipper, csPlane* portal_plane, bool loose_end, csRenderView& rview)
{
  if (sector->draw_busy >= 5)
    return false;

  Stats::portals_drawn++;
  if (!new_clipper->GetNumVertices ())
    return false;

  csPolygonClipper new_view (new_clipper->GetVertices (),
    new_clipper->GetNumVertices (), rview.IsMirrored (), true);

  csRenderView new_rview = rview;
  new_rview.view = &new_view;

  new_rview.clip_plane = *portal_plane;
  new_rview.clip_plane.Invert ();
  if (loose_end || sector->IsBSP ()) new_rview.do_clip_plane = true;

  if (do_warp_space)
  {
    bool mirror = new_rview.IsMirrored ();
    WarpSpace (new_rview, mirror);
    new_rview.SetMirrored (mirror);
  }
  sector->Draw (new_rview);

  return true;
}

csPolygon3D* csPortalCS::HitBeam (csVector3& start, csVector3& end)
{
  return sector->HitBeam (start, end);
}

csPolygon3D* csPortalCS::IntersectSphere (csVector3& center, float radius, float* pr)
{
  return sector->IntersectSphere (center, radius, pr);
}

csSector* csPortalCS::FollowSegment (csReversibleTransform& t,
				  csVector3& new_position, bool& mirror)
{
  if (do_warp_space)
  {
    WarpSpace (t, mirror); 
    new_position = warp_wor.Other2This (new_position);
  }
  return sector ? sector->FollowSegment (t, new_position, mirror) : (csSector*)NULL;
}

bool csPortalCS::BlockingThings (csVector3& start, csVector3& end, csPolygon3D** poly)
{
  if (sector->beam_busy >= csSector::cfg_reflections) return false;
  if (do_warp_space)
  {
    csVector3 start2 = warp_wor.Other2This (start);
    csVector3 end2 = warp_wor.Other2This (end);
    return sector->BlockingThings (start2, end2, poly);
  }
  else
    return sector->BlockingThings (start, end, poly);
}

csPolygon3D* csPortalCS::FollowBeam (csVector3& start, csVector3& end, csPolygon3D* poly, float* sqdist)
{
  if (sector->beam_busy >= csSector::cfg_reflections) return NULL;
  if (do_warp_space)
  {
    csVector3 start2 = warp_wor.Other2This (start);
    csVector3 end2 = warp_wor.Other2This (end);
    return sector->FollowBeam (start2, end2, poly, sqdist);
  }
  else
    return sector->FollowBeam (start, end, poly, sqdist);
}

void csPortalCS::ShineLightmaps (csLightView& lview)
{
  if (do_warp_space)
  {
    csLightView new_lview;
    new_lview.Copy (lview);

    new_lview.center = warp_wor.Other2This (lview.center);
    new_lview.beam2source /= warp_wor;

    int i;
    for (i = 0 ; i < lview.num_frustrum ; i++)
      new_lview.frustrum[i] = warp_wor.Other2This (lview.frustrum[i]);

    if (do_mirror) new_lview.mirror = !lview.mirror;
    if (cfg_alpha)
    {
      //float a = cfg_alpha;
      //new_lview.r = (a * lview.r + (100-a) * filter_r) / 100.;
      //new_lview.g = (a * lview.g + (100-a) * filter_g) / 100.;
      //new_lview.b = (a * lview.b + (100-a) * filter_b) / 100.;
      float fr, fg, fb;
      if (filter_texture)
      {
        filter_texture->GetMeanColor (fr, fg, fb);
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
    if (new_lview.r >= SMALL_EPSILON || new_lview.g >= SMALL_EPSILON || new_lview.b >= SMALL_EPSILON)
      sector->ShineLightmaps (new_lview);
  }
  else sector->ShineLightmaps (lview);
}

void csPortalCS::CalculateLightmaps (csLightView& lview)
{
  if (do_warp_space)
  {
    csLightView new_lview;
    new_lview.Copy (lview);

    new_lview.center = warp_wor.Other2This (lview.center);
    new_lview.beam2source /= warp_wor;
    new_lview.light_frustrum->Transform (&warp_wor);

    if (do_mirror)
    {
      new_lview.mirror = !lview.mirror;
      new_lview.light_frustrum->SetMirrored (new_lview.mirror);
    }
    if (cfg_alpha)
    {
      //float a = cfg_alpha;
      //new_lview.r = (a * lview.r + (100-a) * filter_r) / 100.;
      //new_lview.g = (a * lview.g + (100-a) * filter_g) / 100.;
      //new_lview.b = (a * lview.b + (100-a) * filter_b) / 100.;
      float fr, fg, fb;
      if (filter_texture)
      {
        filter_texture->GetMeanColor (fr, fg, fb);
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
    if (new_lview.r >= SMALL_EPSILON || new_lview.g >= SMALL_EPSILON || new_lview.b >= SMALL_EPSILON)
      sector->CalculateLightmaps (new_lview);
  }
  else sector->CalculateLightmaps (lview);
}

void csPortalCS::DumpFrustrum (csStatLight* light, csVector3* frustrum, int num_frustrum,
	csTransform& t)
{
  if (do_warp_space)
  {
    // @@@ Currently not supported yet.
    return;
  }
  else sector->DumpFrustrum (light, frustrum, num_frustrum, t);
}

