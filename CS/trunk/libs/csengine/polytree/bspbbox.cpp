/*
    Copyright (C) 1998-2000 by Jorrit Tyberghein

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
#include "csengine/bspbbox.h"
#include "csengine/camera.h"
#include "csengine/engine.h"

//---------------------------------------------------------------------------

csPolygonInt* csBspPolygonFactory::Create ()
{
  csBspPolygon* pol = new csBspPolygon ();
  return (csPolygonInt*)pol;
}

void csBspPolygonFactory::Init (csPolygonInt* pi)
{
  csBspPolygon* pol = (csBspPolygon*)pi;
  pol->polygon.MakeEmpty ();
  pol->parent = NULL;
  pol->originator = NULL;
}

//---------------------------------------------------------------------------

csPolygonIntPool& csBspPolygon::GetPolygonPool()
{
  static csBspPolygonFactory poly_fact;
  static csPolygonIntPool poly_pool (&poly_fact);
  return poly_pool;
}

void csBspPolygon::Dump ()
{
  int i, j;
  csVector3Array& verts = parent->GetVertices ();
  csVector3Array& cverts = parent->GetCameraVertices ();
  for (i = 0 ; i < polygon.GetNumVertices () ; i++)
  {
    j=polygon[i];
    printf("  vt %d:(%f,%f,%f) cam:(%f,%f,%f)\n", i, verts[j].x,
	verts[j].y, verts[j].z, cverts[j].x, cverts[j].y,
	cverts[j].z);
  }
}

int csBspPolygon::Classify (const csPlane3& pl)
{
  if (GetPolyPlane () == &pl) return POL_SAME_PLANE;
  if (csMath3::PlanesEqual (pl, *GetPolyPlane ())) return POL_SAME_PLANE;

  int i;
  int front = 0, back = 0;
  csVector3* verts = parent->GetVertices ().GetVertices ();

  for (i = 0 ; i < polygon.GetNumVertices () ; i++)
  {
    float dot = pl.Classify (verts[polygon[i]]);
    if (ABS (dot) < EPSILON) dot = 0;
    if (dot > 0) back++;
    else if (dot < 0) front++;
  }
  if (back == 0) return POL_FRONT;
  if (front == 0) return POL_BACK;
  return POL_SPLIT_NEEDED;
}

int csBspPolygon::ClassifyX (float x)
{
  int i;
  int front = 0, back = 0;
  csVector3* verts = parent->GetVertices ().GetVertices ();

  for (i = 0 ; i < polygon.GetNumVertices () ; i++)
  {
    float xx = verts[polygon[i]].x-x;
    if (xx < -EPSILON) front++;
    else if (xx > EPSILON) back++;
  }
  if (back == 0) return POL_FRONT;
  if (front == 0) return POL_BACK;
  return POL_SPLIT_NEEDED;
}

int csBspPolygon::ClassifyY (float y)
{
  int i;
  int front = 0, back = 0;
  csVector3* verts = parent->GetVertices ().GetVertices ();

  for (i = 0 ; i < polygon.GetNumVertices () ; i++)
  {
    float yy = verts[polygon[i]].y-y;
    if (yy < -EPSILON) front++;
    else if (yy > EPSILON) back++;
  }
  if (back == 0) return POL_FRONT;
  if (front == 0) return POL_BACK;
  return POL_SPLIT_NEEDED;
}

int csBspPolygon::ClassifyZ (float z)
{
  int i;
  int front = 0, back = 0;
  csVector3* verts = parent->GetVertices ().GetVertices ();

  for (i = 0 ; i < polygon.GetNumVertices () ; i++)
  {
    float zz = verts[polygon[i]].z-z;
    if (zz < -EPSILON) front++;
    else if (zz > EPSILON) back++;
  }
  if (back == 0) return POL_FRONT;
  if (front == 0) return POL_BACK;
  return POL_SPLIT_NEEDED;
}


void csBspPolygon::SplitWithPlane (csPolygonInt** poly1, csPolygonInt** poly2,
				  const csPlane3& split_plane)
{
  csBspPolygon* np1 = (csBspPolygon*)GetPolygonPool().Alloc ();
  csBspPolygon* np2 = (csBspPolygon*)GetPolygonPool().Alloc ();
  *poly1 = (csPolygonInt*)np1; // Front
  *poly2 = (csPolygonInt*)np2; // Back
  csPolyIndexed& polygon1 = np1->GetPolygon ();
  csPolyIndexed& polygon2 = np2->GetPolygon ();
  polygon1.MakeEmpty ();
  polygon2.MakeEmpty ();
  np1->SetPolyPlane (plane);
  np2->SetPolyPlane (plane);
  np1->SetOriginator (GetOriginator ());
  np2->SetOriginator (GetOriginator ());
  np1->SetParent (GetParent ());
  np2->SetParent (GetParent ());

  csVector3 ptB;
  float sideA, sideB;
  csVector3 ptA = GetParent ()->GetVertices ().GetVertices ()
  	[polygon[polygon.GetNumVertices () - 1]];
  sideA = split_plane.Classify (ptA);
  if (ABS (sideA) < SMALL_EPSILON) sideA = 0;
  int idx;

  for (int i = -1 ; ++i < polygon.GetNumVertices () ; )
  {
    ptB = GetParent ()->GetVertices ().GetVertices ()[polygon[i]];
    sideB = split_plane.Classify (ptB);
    if (ABS (sideB) < SMALL_EPSILON) sideB = 0;
    if (sideB > 0)
    {
      if (sideA < 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - split_plane.Classify (ptA) / ( split_plane.GetNormal () * v ) ;
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon2.AddVertex (polygon[i]);	// Add ptB
    }
    else if (sideB < 0)
    {
      if (sideA > 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - split_plane.Classify (ptA) / ( split_plane.GetNormal () * v );
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon1.AddVertex (polygon[i]);	// Add ptB
    }
    else
    {
      polygon1.AddVertex (polygon[i]);	// ptB
      polygon2.AddVertex (polygon[i]);	// ptB
    }
    ptA = ptB;
    sideA = sideB;
  }
}

void csBspPolygon::SplitWithPlaneX (csPolygonInt** poly1, csPolygonInt** poly2,
				  float x)
{
  csBspPolygon* np1 = (csBspPolygon*)GetPolygonPool().Alloc ();
  csBspPolygon* np2 = (csBspPolygon*)GetPolygonPool().Alloc ();
  *poly1 = (csPolygonInt*)np1; // Front
  *poly2 = (csPolygonInt*)np2; // Back
  csPolyIndexed& polygon1 = np1->GetPolygon ();
  csPolyIndexed& polygon2 = np2->GetPolygon ();
  polygon1.MakeEmpty ();
  polygon2.MakeEmpty ();
  np1->SetPolyPlane (plane);
  np2->SetPolyPlane (plane);
  np1->SetOriginator (GetOriginator ());
  np2->SetOriginator (GetOriginator ());
  np1->SetParent (GetParent ());
  np2->SetParent (GetParent ());

  csVector3 ptB;
  float sideA, sideB;
  csVector3 ptA = GetParent ()->GetVertices ().GetVertices ()
  	[polygon[polygon.GetNumVertices () - 1]];
  sideA = ptA.x - x;
  if (ABS (sideA) < SMALL_EPSILON) sideA = 0;
  int idx;

  for (int i = -1 ; ++i < polygon.GetNumVertices () ; )
  {
    ptB = GetParent ()->GetVertices ().GetVertices ()[polygon[i]];
    sideB = ptB.x - x;
    if (ABS (sideB) < SMALL_EPSILON) sideB = 0;
    if (sideB > 0)
    {
      if (sideA < 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - (ptA.x - x) / v.x ;
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon2.AddVertex (polygon[i]);	// Add ptB
    }
    else if (sideB < 0)
    {
      if (sideA > 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - (ptA.x - x) / v.x;
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon1.AddVertex (polygon[i]);	// Add ptB
    }
    else
    {
      polygon1.AddVertex (polygon[i]);	// ptB
      polygon2.AddVertex (polygon[i]);	// ptB
    }
    ptA = ptB;
    sideA = sideB;
  }
}

void csBspPolygon::SplitWithPlaneY (csPolygonInt** poly1, csPolygonInt** poly2,
				  float y)
{
  csBspPolygon* np1 = (csBspPolygon*)GetPolygonPool().Alloc ();
  csBspPolygon* np2 = (csBspPolygon*)GetPolygonPool().Alloc ();
  *poly1 = (csPolygonInt*)np1; // Front
  *poly2 = (csPolygonInt*)np2; // Back
  csPolyIndexed& polygon1 = np1->GetPolygon ();
  csPolyIndexed& polygon2 = np2->GetPolygon ();
  polygon1.MakeEmpty ();
  polygon2.MakeEmpty ();
  np1->SetPolyPlane (plane);
  np2->SetPolyPlane (plane);
  np1->SetOriginator (GetOriginator ());
  np2->SetOriginator (GetOriginator ());
  np1->SetParent (GetParent ());
  np2->SetParent (GetParent ());

  csVector3 ptB;
  float sideA, sideB;
  csVector3 ptA = GetParent ()->GetVertices ().GetVertices ()
  	[polygon[polygon.GetNumVertices () - 1]];
  sideA = ptA.y - y;
  if (ABS (sideA) < SMALL_EPSILON) sideA = 0;
  int idx;

  for (int i = -1 ; ++i < polygon.GetNumVertices () ; )
  {
    ptB = GetParent ()->GetVertices ().GetVertices ()[polygon[i]];
    sideB = ptB.y - y;
    if (ABS (sideB) < SMALL_EPSILON) sideB = 0;
    if (sideB > 0)
    {
      if (sideA < 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - (ptA.y - y) / v.y ;
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon2.AddVertex (polygon[i]);	// Add ptB
    }
    else if (sideB < 0)
    {
      if (sideA > 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - (ptA.y - y) / v.y;
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon1.AddVertex (polygon[i]);	// Add ptB
    }
    else
    {
      polygon1.AddVertex (polygon[i]);	// ptB
      polygon2.AddVertex (polygon[i]);	// ptB
    }
    ptA = ptB;
    sideA = sideB;
  }
}

void csBspPolygon::SplitWithPlaneZ (csPolygonInt** poly1, csPolygonInt** poly2,
				  float z)
{
  csBspPolygon* np1 = (csBspPolygon*)GetPolygonPool().Alloc ();
  csBspPolygon* np2 = (csBspPolygon*)GetPolygonPool().Alloc ();
  *poly1 = (csPolygonInt*)np1; // Front
  *poly2 = (csPolygonInt*)np2; // Back
  csPolyIndexed& polygon1 = np1->GetPolygon ();
  csPolyIndexed& polygon2 = np2->GetPolygon ();
  polygon1.MakeEmpty ();
  polygon2.MakeEmpty ();
  np1->SetPolyPlane (plane);
  np2->SetPolyPlane (plane);
  np1->SetOriginator (GetOriginator ());
  np2->SetOriginator (GetOriginator ());
  np1->SetParent (GetParent ());
  np2->SetParent (GetParent ());

  csVector3 ptB;
  float sideA, sideB;
  csVector3 ptA = GetParent ()->GetVertices ().GetVertices ()
  	[polygon[polygon.GetNumVertices () - 1]];
  sideA = ptA.z - z;
  if (ABS (sideA) < SMALL_EPSILON) sideA = 0;
  int idx;

  for (int i = -1 ; ++i < polygon.GetNumVertices () ; )
  {
    ptB = GetParent ()->GetVertices ().GetVertices ()[polygon[i]];
    sideB = ptB.z - z;
    if (ABS (sideB) < SMALL_EPSILON) sideB = 0;
    if (sideB > 0)
    {
      if (sideA < 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - (ptA.z - z) / v.z ;
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon2.AddVertex (polygon[i]);	// Add ptB
    }
    else if (sideB < 0)
    {
      if (sideA > 0)
      {
	// Compute the intersection point of the line
	// from point A to point B with the partition
	// plane. This is a simple ray-plane intersection.
	csVector3 v = ptB; v -= ptA;
	float sect = - (ptA.z - z) / v.z;
	v *= sect; v += ptA;
	idx = GetParent ()->GetVertices ().AddVertexSmart (v);
	polygon1.AddVertex (idx);
	polygon2.AddVertex (idx);
      }
      polygon1.AddVertex (polygon[i]);	// Add ptB
    }
    else
    {
      polygon1.AddVertex (polygon[i]);	// ptB
      polygon2.AddVertex (polygon[i]);	// ptB
    }
    ptA = ptB;
    sideA = sideB;
  }
}

void csBspPolygon::Transform (const csTransform& trans)
{
  plane = trans.Other2This (plane);
  //@@@ We should avoid this if not needed!
  // Maybe we can mark transforms which do not change normalization
  // with some special flag.
  plane.Normalize ();
}

bool csBspPolygon::ClipToPlane (csPlane3* portal_plane, const csVector3& v_w2c,
	csVector3*& pverts, int& num_verts, bool cw)
{
  int i, i1, cnt_vis;
  float r;
  bool zs, z1s;

  // Assume maximum 100 vertices! (@@@ HARDCODED LIMIT)
  static csVector3 verts[100];
  bool vis[100];

  // Count the number of visible vertices for this polygon (note
  // that the transformation from world to camera space for all the
  // vertices has been done earlier).
  // If there are no visible vertices this polygon need not be drawn.
  csVector3* vertices = GetParent ()->GetCameraVertices ().GetVertices ();
  cnt_vis = 0;
  for (i = 0 ; i < polygon.GetNumVertices () ; i++)
    if (vertices[polygon[i]].z >= 0) cnt_vis++;
  if (cnt_vis == 0) return false;

  // Perform backface culling.
  float cl = plane.Classify (v_w2c);
  if (cw)
  {
    if (cl > 0) return false;
  }
  else
  {
    if (cl < 0) return false;
  }

  // Copy the vertices to verts.
  int num_vertices = polygon.GetNumVertices ();
  for (i = 0 ; i < num_vertices ; i++) verts[i] = vertices[polygon[i]];
  pverts = verts;

  // If there is no portal polygon then everything is ok.
  if (!portal_plane) { num_verts = num_vertices; return true; }

  // Otherwise we will have to clip this polygon in 3D against the
  // portal polygon. This is to make sure that objects behind the
  // portal polygon are not accidently rendered.

  // First count how many vertices are before the portal polygon
  // (so are visible as seen from the portal).
  cnt_vis = 0;
  for (i = 0 ; i < num_vertices ; i++)
  {
    //vis[i] = csMath3::Visible (Vcam (i), *portal_plane);
    vis[i] = portal_plane->Classify (vertices[polygon[i]]) <= SMALL_EPSILON;
    if (vis[i]) cnt_vis++;
  }

  if (cnt_vis == 0) return false; // Polygon is not visible.

  // If all vertices are visible then everything is ok.
  if (cnt_vis == num_vertices) { num_verts = num_vertices; return true; }

  // We really need to clip.
  num_verts = 0;

  float A = portal_plane->A ();
  float B = portal_plane->B ();
  float C = portal_plane->C ();
  float D = portal_plane->D ();

  i1 = num_vertices-1;
  for (i = 0 ; i < num_vertices ; i++)
  {
    zs = !vis[i];
    z1s = !vis[i1];

    if (z1s && !zs)
    {
      csIntersect3::Plane (vertices[polygon[i1]], vertices[polygon[i]], A, B, C, D,
      	verts[num_verts], r);
      num_verts++;
      verts[num_verts++] = vertices[polygon[i]];
    }
    else if (!z1s && zs)
    {
      csIntersect3::Plane (vertices[polygon[i1]], vertices[polygon[i]], A, B, C, D,
      	verts[num_verts], r);
      num_verts++;
    }
    else if (!z1s && !zs)
    {
      verts[num_verts++] = vertices[polygon[i]];
    }
    i1 = i;
  }

  return true;
}

#define EXPERIMENTAL_BUG_FIX 1
bool csBspPolygon::DoPerspective (const csTransform& trans,
  csVector3* source, int num_verts, csPolygon2D* dest, bool mirror)
{
  csVector3 *ind, *end = source+num_verts;

  if (num_verts==0) return false;
  dest->MakeEmpty ();

  // Classify all points as NORMAL (z>=SMALL_Z), NEAR (0<=z<SMALL_Z), or
  // BEHIND (z<0).  Use several processing algorithms: trivially accept if all
  // points are NORMAL, mixed process if some points are NORMAL and some
  // are not, special process if there are no NORMAL points, but some are
  // NEAR.  Assume that the polygon has already been culled if all points
  // are BEHIND.

  // Handle the trivial acceptance case:
  ind = source;
  while (ind < end)
  {
    if (ind->z >= SMALL_Z) dest->AddPerspective (*ind);
    else break;
    ind++;
  }

  // Check if special or mixed processing is required
  if (ind != end)
  {
    csVector3 *exit = NULL, *exitn = NULL, *reenter = NULL, *reentern = NULL;
    csVector2 *evert = NULL;

    if (ind == source)
    {
      while (ind < end)
      {
        if (ind->z >= SMALL_Z) { reentern = ind;  reenter = ind-1;  break; }
        ind++;
      }
    }
    else
    {
      exit = ind;
      exitn = ind-1;
      evert = dest->GetLast ();
    }

    // Check if mixed processing is required
    if (exit || reenter)
    {
     bool needfinish = false;

     if (exit)
     {
      // we know where the polygon is no longer NORMAL, now we need to
      // to find out on which edge it becomes NORMAL again.
      while (ind < end)
      {
       if (ind->z >= SMALL_Z) { reentern = ind;  reenter = ind-1;  break; }
       ind++;
      }
      if (ind == end) { reentern = source;  reenter = ind-1; }
       else needfinish = true;
     } /* if (exit) */
     else
     {
      // we know where the polygon becomes NORMAL, now we need to
      // to find out on which edge it ceases to be NORMAL.
      while (ind < end)
      {
       if (ind->z >= SMALL_Z) dest->AddPerspective (*ind);
       else { exit = ind;  exitn = ind-1;  break; }
       ind++;
      }
      if (ind == end) { exit = source;  exitn = ind-1; }
      evert = dest->GetLast ();
     }

     // Add the NEAR points appropriately.
#    define MAX_VALUE 1000000.

     // First, for the exit point.
     float ex, ey, epointx, epointy;
     ex = exitn->z * exit->x - exitn->x * exit->z;
     ey = exitn->z * exit->y - exitn->y * exit->z;
     if (ABS(ex) < SMALL_EPSILON && ABS(ey) < SMALL_EPSILON)
     {
      // Uncommon special case:  polygon passes through origin.
      csPlane3 cam_plane = trans.Other2This (plane);
      ex = cam_plane.A();
      ey = cam_plane.B();
      if (ABS(ex) < SMALL_EPSILON && ABS(ey) < SMALL_EPSILON)
      {
       // Downright rare case:  polygon near parallel with viewscreen.
       ex = exit->x - exitn->x;
       ey = exit->y - exitn->y;
      }
     }
     if (ABS(ex) > ABS(ey))
     {
       if (ex>0) epointx = MAX_VALUE;
       else epointx = -MAX_VALUE;
       epointy = (epointx - evert->x)*ey/ex + evert->y;
     }
     else
     {
       if (ey>0) epointy = MAX_VALUE;
       else epointy = -MAX_VALUE;
       epointx = (epointy - evert->y)*ex/ey + evert->x;
     }

     // Next, for the reentry point.
     float rx, ry, rpointx, rpointy;

     // Perspective correct the point.
     float iz = csEngine::current_engine->current_camera->GetFOV ()/reentern->z;
     csVector2 rvert;
     rvert.x = reentern->x * iz + csEngine::current_engine->current_camera->GetShiftX ();
     rvert.y = reentern->y * iz + csEngine::current_engine->current_camera->GetShiftY ();

     if (reenter == exit && reenter->z > -SMALL_EPSILON)
     { rx = ex;  ry = ey; }
     else
     {
       rx = reentern->z * reenter->x - reentern->x * reenter->z;
       ry = reentern->z * reenter->y - reentern->y * reenter->z;
     }
     if (ABS(rx) < SMALL_EPSILON && ABS(ry) < SMALL_EPSILON)
     {
      // Uncommon special case:  polygon passes through origin.
      csPlane3 cam_plane = trans.Other2This (plane);
      rx = cam_plane.A();
      ry = cam_plane.B();
      if (ABS(rx) < SMALL_EPSILON && ABS(ry) < SMALL_EPSILON)
      {
       // Downright rare case:  polygon near parallel with viewscreen.
       rx = reenter->x - reentern->x;
       ry = reenter->y - reentern->y;
      }
     }
     if (ABS(rx) > ABS(ry))
     {
       if (rx>0) rpointx = MAX_VALUE;
       else rpointx = -MAX_VALUE;
       rpointy = (rpointx - rvert.x)*ry/rx + rvert.y;
     }
     else
     {
       if (ry>0) rpointy = MAX_VALUE;
       else rpointy = -MAX_VALUE;
       rpointx = (rpointy - rvert.y)*rx/ry + rvert.x;
     }

#    define QUADRANT(x,y) ( (y<x?1:0)^(x<-y?3:0) )
#    define MQUADRANT(x,y) ( (y<x?3:0)^(x<-y?1:0) )

    dest->AddVertex (epointx,epointy);
#   if EXPERIMENTAL_BUG_FIX
    if (mirror)
    {
      int quad = MQUADRANT(epointx, epointy);
      int rquad = MQUADRANT(rpointx, rpointy);
      if ((quad==0 && -epointx==epointy)||(quad==1 && epointx==epointy))
        quad++;
      if ((rquad==0 && -rpointx==rpointy)||(rquad==1 && rpointx==rpointy))
        rquad++;
      while (quad != rquad)
      {
        epointx = (quad&2)           ?  MAX_VALUE : -MAX_VALUE;
        epointy = (quad==0||quad==3) ?  MAX_VALUE : -MAX_VALUE;
        dest->AddVertex (epointx, epointy);
        quad = (quad+1)&3;
      }
    }
    else
    {
      int quad = QUADRANT(epointx, epointy);
      int rquad = QUADRANT(rpointx, rpointy);
      if ((quad==0 && epointx==epointy)||(quad==1 && -epointx==epointy))
        quad++;
      if ((rquad==0 && rpointx==rpointy)||(rquad==1 && -rpointx==rpointy))
        rquad++;
      while (quad != rquad)
      {
        epointx = (quad&2)           ? -MAX_VALUE :  MAX_VALUE;
        epointy = (quad==0||quad==3) ?  MAX_VALUE : -MAX_VALUE;
        dest->AddVertex (epointx, epointy);
        quad = (quad+1)&3;
      }
    }
#   endif
    dest->AddVertex (rpointx,rpointy);

     // Add the rest of the vertices, which are all NORMAL points.
     if (needfinish) while (ind < end)
      dest->AddPerspective (*ind++);

    } /* if (exit || reenter) */

    // Do special processing (all points are NEAR or BEHIND)
    else
    {
      if (mirror)
      {
        csVector3* ind2 = end - 1;
        for (ind = source;  ind < end;  ind2=ind, ind++)
          if ((ind->x - ind2->x)*(ind2->y) - (ind->y - ind2->y)*(ind2->x) > -SMALL_EPSILON)
            return false;
        dest->AddVertex ( MAX_VALUE,-MAX_VALUE);
        dest->AddVertex ( MAX_VALUE, MAX_VALUE);
        dest->AddVertex (-MAX_VALUE, MAX_VALUE);
        dest->AddVertex (-MAX_VALUE,-MAX_VALUE);
      }
      else
      {
        csVector3* ind2 = end - 1;
        for (ind = source;  ind < end;  ind2=ind, ind++)
          if ((ind->x - ind2->x)*(ind2->y) - (ind->y - ind2->y)*(ind2->x) < SMALL_EPSILON)
            return false;
        dest->AddVertex (-MAX_VALUE,-MAX_VALUE);
        dest->AddVertex (-MAX_VALUE, MAX_VALUE);
        dest->AddVertex ( MAX_VALUE, MAX_VALUE);
        dest->AddVertex ( MAX_VALUE,-MAX_VALUE);
      }
    }

  } /* if (ind != end) */
  return true;
}

//---------------------------------------------------------------------------

csPolygonStubFactory csPolyTreeBBox::stub_fact (&csBspPolygon::GetPolygonPool());

csPolyTreeBBox::csPolyTreeBBox () : csDetailedPolyTreeObject (&stub_fact)
{
  base_stub = (csPolygonStub*)csDetailedPolyTreeObject::stub_pool.Alloc (&stub_fact);
  base_stub->IncRef (); // Make sure this object is locked.
  is_cam_transf = false;
}

csPolyTreeBBox::~csPolyTreeBBox ()
{
  if (base_stub)
  {
    csDetailedPolyTreeObject::stub_pool.Free (base_stub);
    csDetailedPolyTreeObject::stub_pool.Free (base_stub);
  }
}

void csPolyTreeBBox::World2Camera (const csTransform& trans)
{
  int i;
  cam_vertices.MakeRoom (vertices.GetNumVertices ());
  for (i = 0 ; i < vertices.GetNumVertices () ; i++)
    cam_vertices[i] = trans.Other2This (vertices[i]);
  is_cam_transf = true;
}

void csPolyTreeBBox::Update (const csBox3& object_bbox, const csTransform& o2w,
	csVisObjInfo* originator)
{
  RemoveFromTree ();

  const csBox3& b = object_bbox;
  csVector3Array& va = GetVertices ();
  va.MakeEmpty ();

  // Clear the polygons from the base stub.
  base_stub->Initialize ();

  // Add the eight corner points of the bounding box to the container.
  // Transform from object to world space here.
  world_bbox.StartBoundingBox ();
  csVector3 v;
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_xyz));
  int pt_xyz = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_Xyz));
  int pt_Xyz = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_xYz));
  int pt_xYz = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_XYz));
  int pt_XYz = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_xyZ));
  int pt_xyZ = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_XyZ));
  int pt_XyZ = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_xYZ));
  int pt_xYZ = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);
  v = o2w.Other2This (b.GetCorner (BOX_CORNER_XYZ));
  int pt_XYZ = va.AddVertex (v);
  world_bbox.AddBoundingVertex (v);

  csBspPolygon* poly;

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xYz);
  poly->GetPolygon ().AddVertex (pt_XYz);
  poly->GetPolygon ().AddVertex (pt_Xyz);
  poly->GetPolygon ().AddVertex (pt_xyz);
  poly->SetPolyPlane (csPlane3 (0, 0, 1, -b.MinZ ()));
  poly->Transform (o2w);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_XYz);
  poly->GetPolygon ().AddVertex (pt_XYZ);
  poly->GetPolygon ().AddVertex (pt_XyZ);
  poly->GetPolygon ().AddVertex (pt_Xyz);
  poly->SetPolyPlane (csPlane3 (-1, 0, 0, b.MaxX ()));
  poly->Transform (o2w);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_XYZ);
  poly->GetPolygon ().AddVertex (pt_xYZ);
  poly->GetPolygon ().AddVertex (pt_xyZ);
  poly->GetPolygon ().AddVertex (pt_XyZ);
  poly->SetPolyPlane (csPlane3 (0, 0, -1, b.MaxZ ()));
  poly->Transform (o2w);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xYZ);
  poly->GetPolygon ().AddVertex (pt_xYz);
  poly->GetPolygon ().AddVertex (pt_xyz);
  poly->GetPolygon ().AddVertex (pt_xyZ);
  poly->SetPolyPlane (csPlane3 (1, 0, 0, -b.MinX ()));
  poly->Transform (o2w);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xYZ);
  poly->GetPolygon ().AddVertex (pt_XYZ);
  poly->GetPolygon ().AddVertex (pt_XYz);
  poly->GetPolygon ().AddVertex (pt_xYz);
  poly->SetPolyPlane (csPlane3 (0, -1, 0, b.MaxY ()));
  poly->Transform (o2w);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xyz);
  poly->GetPolygon ().AddVertex (pt_Xyz);
  poly->GetPolygon ().AddVertex (pt_XyZ);
  poly->GetPolygon ().AddVertex (pt_xyZ);
  poly->SetPolyPlane (csPlane3 (0, 1, 0, -b.MinY ()));
  poly->Transform (o2w);
}

void csPolyTreeBBox::Update (const csBox3& world_bbox, csVisObjInfo* originator)
{
  csPolyTreeObject::world_bbox = world_bbox;
// printf ("%f,%f,%f %f,%f,%f\n", world_bbox.MinX (), world_bbox.MinY (), world_bbox.MinZ (),
// world_bbox.MaxX (), world_bbox.MaxY (), world_bbox.MaxZ ());

  RemoveFromTree ();

  csVector3Array& va = GetVertices ();
  va.MakeEmpty ();

  // Identity transformation.
  csTransform trans;
  const csBox3& b = world_bbox;

  // Add the eight corner points of the bounding box to the container.
  int pt_xyz = va.AddVertex (b.GetCorner (BOX_CORNER_xyz));
  int pt_Xyz = va.AddVertex (b.GetCorner (BOX_CORNER_Xyz));
  int pt_xYz = va.AddVertex (b.GetCorner (BOX_CORNER_xYz));
  int pt_XYz = va.AddVertex (b.GetCorner (BOX_CORNER_XYz));
  int pt_xyZ = va.AddVertex (b.GetCorner (BOX_CORNER_xyZ));
  int pt_XyZ = va.AddVertex (b.GetCorner (BOX_CORNER_XyZ));
  int pt_xYZ = va.AddVertex (b.GetCorner (BOX_CORNER_xYZ));
  int pt_XYZ = va.AddVertex (b.GetCorner (BOX_CORNER_XYZ));

  csBspPolygon* poly;

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xYz);
  poly->GetPolygon ().AddVertex (pt_XYz);
  poly->GetPolygon ().AddVertex (pt_Xyz);
  poly->GetPolygon ().AddVertex (pt_xyz);
  poly->SetPolyPlane (csPlane3 (0, 0, 1, -b.MinZ ()));
  poly->Transform (trans);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_XYz);
  poly->GetPolygon ().AddVertex (pt_XYZ);
  poly->GetPolygon ().AddVertex (pt_XyZ);
  poly->GetPolygon ().AddVertex (pt_Xyz);
  poly->SetPolyPlane (csPlane3 (-1, 0, 0, b.MaxX ()));
  poly->Transform (trans);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_XYZ);
  poly->GetPolygon ().AddVertex (pt_xYZ);
  poly->GetPolygon ().AddVertex (pt_xyZ);
  poly->GetPolygon ().AddVertex (pt_XyZ);
  poly->SetPolyPlane (csPlane3 (0, 0, -1, b.MaxZ ()));
  poly->Transform (trans);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xYZ);
  poly->GetPolygon ().AddVertex (pt_xYz);
  poly->GetPolygon ().AddVertex (pt_xyz);
  poly->GetPolygon ().AddVertex (pt_xyZ);
  poly->SetPolyPlane (csPlane3 (1, 0, 0, -b.MinX ()));
  poly->Transform (trans);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xYZ);
  poly->GetPolygon ().AddVertex (pt_XYZ);
  poly->GetPolygon ().AddVertex (pt_XYz);
  poly->GetPolygon ().AddVertex (pt_xYz);
  poly->SetPolyPlane (csPlane3 (0, -1, 0, b.MaxY ()));
  poly->Transform (trans);

  poly = (csBspPolygon*)csBspPolygon::GetPolygonPool().Alloc ();
  AddPolygon (poly);
  poly->SetOriginator (originator);
  poly->GetPolygon ().AddVertex (pt_xyz);
  poly->GetPolygon ().AddVertex (pt_Xyz);
  poly->GetPolygon ().AddVertex (pt_XyZ);
  poly->GetPolygon ().AddVertex (pt_xyZ);
  poly->SetPolyPlane (csPlane3 (0, 1, 0, -b.MinY ()));
  poly->Transform (trans);
}

//---------------------------------------------------------------------------

void* csSphereStub::Visit (csThing* sector, csTreeVisitFunc* func, void* data)
{
  (void)sector; (void)func; (void)data;
  return NULL;
}

//---------------------------------------------------------------------------
csSphereStubFactory csSphereTreeObject::stub_fact;
csObjectStubPool csSphereTreeObject::stub_pool;

csSphereTreeObject::csSphereTreeObject (const csVector3& center, float rad)
	: csPolyTreeObject (&stub_fact), radius (rad), center (center)
{
  base_stub = (csSphereStub*)stub_pool.Alloc (&stub_fact);
  base_stub->IncRef (); // Make sure this object is locked.
  // @@@@@@ WE NEED TO FILL WORLD_BBOX HERE!
}

void csSphereTreeObject::SplitWithPlane (csObjectStub* stub,
  	csObjectStub** stub_on, csObjectStub** stub_front,
	csObjectStub** stub_back,
	const csPlane3& plane)
{
  (void)stub; (void)stub_on; (void)stub_front; (void)stub_back;
  (void)plane;
}

void csSphereTreeObject::SplitWithPlaneX (csObjectStub* stub,
  	csObjectStub** stub_on, csObjectStub** stub_front,
	csObjectStub** stub_back,
	float x)
{
  (void)stub; (void)stub_on; (void)stub_front; (void)stub_back;
  (void)x;
}

void csSphereTreeObject::SplitWithPlaneY (csObjectStub* stub,
  	csObjectStub** stub_on, csObjectStub** stub_front,
	csObjectStub** stub_back,
	float y)
{
  (void)stub; (void)stub_on; (void)stub_front; (void)stub_back;
  (void)y;
}

void csSphereTreeObject::SplitWithPlaneZ (csObjectStub* stub,
  	csObjectStub** stub_on, csObjectStub** stub_front,
	csObjectStub** stub_back,
	float z)
{
  (void)stub; (void)stub_on; (void)stub_front; (void)stub_back;
  (void)z;
}
