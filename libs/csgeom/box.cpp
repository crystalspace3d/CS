/*
    Copyright (C) 1998 by Jorrit Tyberghein
    Largely rewritten by Ivan Avramovic <ivan@avramovic.com>
  
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

#include <math.h>
#include "cssysdef.h"
#include "csgeom/box.h"

//---------------------------------------------------------------------------

csVector2 csBox2::GetCorner (int corner) const
{
  switch (corner)
  {
    case BOX_CORNER_xy: return Min ();
    case BOX_CORNER_xY: return csVector2 (MinX (), MaxY ());
    case BOX_CORNER_Xy: return csVector2 (MaxX (), MinY ());
    case BOX_CORNER_XY: return Max ();
  }
  return csVector2 (0, 0);
}

csSegment2 csBox2::GetEdge (int edge) const
{
  switch (edge)
  {
    case 0: return csSegment2 (GetCorner (0), GetCorner (1));
    case 1: return csSegment2 (GetCorner (0), GetCorner (2));
    case 2: return csSegment2 (GetCorner (1), GetCorner (3));
    case 3: return csSegment2 (GetCorner (2), GetCorner (3));
  }
  return csSegment2 ();
}

csBox2& csBox2::operator+= (const csBox2& box)
{
  if (box.minbox.x < minbox.x) minbox.x = box.minbox.x;
  if (box.minbox.y < minbox.y) minbox.y = box.minbox.y;
  if (box.maxbox.x > maxbox.x) maxbox.x = box.maxbox.x;
  if (box.maxbox.y > maxbox.y) maxbox.y = box.maxbox.y;
  return *this;
}

csBox2& csBox2::operator+= (const csVector2& point)
{
  if (point.x < minbox.x) minbox.x = point.x;
  if (point.x > maxbox.x) maxbox.x = point.x;
  if (point.y < minbox.y) minbox.y = point.y;
  if (point.y > maxbox.y) maxbox.y = point.y;
  return *this;
}

csBox2& csBox2::operator*= (const csBox2& box)
{
  if (box.minbox.x > minbox.x) minbox.x = box.minbox.x;
  if (box.minbox.y > minbox.y) minbox.y = box.minbox.y;
  if (box.maxbox.x < maxbox.x) maxbox.x = box.maxbox.x;
  if (box.maxbox.y < maxbox.y) maxbox.y = box.maxbox.y;
  return *this;
}

csBox2 operator+ (const csBox2& box1, const csBox2& box2)
{
  return csBox2( MIN(box1.minbox.x,box2.minbox.x), MIN(box1.minbox.y,box2.minbox.y),
              MAX(box1.maxbox.x,box2.maxbox.x), MAX(box1.maxbox.y,box2.maxbox.y) );
}

csBox2 operator+ (const csBox2& box, const csVector2& point)
{
  return csBox2( MIN(box.minbox.x,point.x), MIN(box.minbox.y,point.y),
              MAX(box.maxbox.x,point.x), MAX(box.maxbox.y,point.y) );
}

csBox2 operator* (const csBox2& box1, const csBox2& box2)
{
  return csBox2( MAX(box1.minbox.x,box2.minbox.x), MAX(box1.minbox.y,box2.minbox.y),
              MIN(box1.maxbox.x,box2.maxbox.x), MIN(box1.maxbox.y,box2.maxbox.y) );
}

bool operator== (const csBox2& box1, const csBox2& box2)
{
  return ( (box1.minbox.x == box2.minbox.x) && (box1.minbox.y == box2.minbox.y) &&
           (box1.maxbox.x == box2.maxbox.x) && (box1.maxbox.y == box2.maxbox.y) );
}

bool operator!= (const csBox2& box1, const csBox2& box2)
{
  return ( (box1.minbox.x != box2.minbox.x) || (box1.minbox.y != box2.minbox.y) ||
           (box1.maxbox.x != box2.maxbox.x) || (box1.maxbox.y != box2.maxbox.y) );
}

bool operator< (const csBox2& box1, const csBox2& box2)
{
  return ( (box1.minbox.x >= box2.minbox.x) && (box1.minbox.y >= box2.minbox.y) &&
           (box1.maxbox.x <= box2.maxbox.x) && (box1.maxbox.y <= box2.maxbox.y) );
}

bool operator> (const csBox2& box1, const csBox2& box2)
{
  return ( (box2.minbox.x >= box1.minbox.x) && (box2.minbox.y >= box1.minbox.y) &&
           (box2.maxbox.x <= box1.maxbox.x) && (box2.maxbox.y <= box1.maxbox.y) );
}

bool operator< (const csVector2& point, const csBox2& box)
{
  return ( (point.x >= box.minbox.x) && (point.x <= box.maxbox.x) &&
           (point.y >= box.minbox.y) && (point.y <= box.maxbox.y) );
}

bool csBox2::Intersect (float minx, float miny, float maxx, float maxy,
    csVector2* poly, int num_poly)
{
  int i, i1;
  for (i = 0 ; i < num_poly ; i++)
    if (poly[i].x <= maxx && poly[i].y <= maxy &&
	poly[i].x >= minx && poly[i].y >= miny)
      return true;

  float r, x, y;
  i1 = num_poly-1;
  for (i = 0 ; i < num_poly ; i++)
  {
    bool do_hor_test1 = (poly[i].x < minx && poly[i1].x > minx);
    bool do_hor_test2 = (poly[i].x < maxx && poly[i1].x > maxx);
    if (do_hor_test1 || do_hor_test2)
    {
      r = (poly[i1].y - poly[i].y) / (poly[i1].x - poly[i].x);
      if (do_hor_test1)
      {
        y = r * (minx - poly[i].x) + poly[i].y;
        if (y >= miny && y <= maxy) return true;
      }
      if (do_hor_test2)
      {
        y = r * (maxx - poly[i].x) + poly[i].y;
        if (y >= miny && y <= maxy) return true;
      }
    }
    bool do_ver_test1 = (poly[i].y < miny && poly[i1].y > miny);
    bool do_ver_test2 = (poly[i].y < maxy && poly[i1].y > maxy);
    if (do_ver_test1 || do_ver_test2)
    {
      r = (poly[i1].x - poly[i].x) / (poly[i1].y - poly[i].y);
      if (do_ver_test1)
      {
        x = r * (miny - poly[i].y) + poly[i].x;
        if (x >= minx && x <= maxx) return true;
      }
      if (do_ver_test2)
      {
        x = r * (maxy - poly[i].y) + poly[i].x;
        if (x >= minx && x <= maxx) return true;
      }
    }
    i1 = i;
  }

  return false;
}

//---------------------------------------------------------------------------

// We have a coordinate system around our box which is
// divided into 27 regions. The center region at coordinate (1,1,1)
// is the node itself. Every one of the 26 remaining regions
// defines an number of vertices which are the convex outline
// as seen from a camera view point in that region.
// The numbers inside the outlines table are indices from 0 to
// 7 which describe the 8 vertices outlining the node:
//	0: left/down/front vertex
//	1: left/down/back
//	2: left/up/front
//	3: left/up/back
//	4: right/down/front
//	5: right/down/back
//	6: right/up/front
//	7: right/up/back
struct Outline
{
  int num;
  int vertices[6];
};
/// Outline lookup table.
static Outline outlines[27] =
{
  { 6, { 3, 2, 6, 4, 5, 1 } },		// 0,0,0
  { 6, { 3, 2, 0, 4, 5, 1 } },		// 0,0,1
  { 6, { 7, 3, 2, 0, 4, 5 } },		// 0,0,2
  { 6, { 3, 2, 6, 4, 0, 1 } },		// 0,1,0
  { 4, { 3, 2, 0, 1, -1, -1 } },	// 0,1,1
  { 6, { 7, 3, 2, 0, 1, 5 } },		// 0,1,2
  { 6, { 3, 7, 6, 4, 0, 1 } },		// 0,2,0
  { 6, { 3, 7, 6, 2, 0, 1 } },		// 0,2,1
  { 6, { 7, 6, 2, 0, 1, 5 } },		// 0,2,2
  { 6, { 2, 6, 4, 5, 1, 0 } },		// 1,0,0
  { 4, { 0, 4, 5, 1, -1, -1 } },	// 1,0,1
  { 6, { 3, 1, 0, 4, 5, 7 } },		// 1,0,2
  { 4, { 2, 6, 4, 0, -1, -1 } },	// 1,1,0
  { 0, { -1, -1, -1, -1, -1, -1 } },	// 1,1,1
  { 4, { 7, 3, 1, 5, -1, -1 } },	// 1,1,2
  { 6, { 3, 7, 6, 4, 0, 2 } },		// 1,2,0
  { 4, { 3, 7, 6, 2, -1, -1 } },	// 1,2,1
  { 6, { 2, 3, 1, 5, 7, 6 } },		// 1,2,2
  { 6, { 2, 6, 7, 5, 1, 0 } },		// 2,0,0
  { 6, { 6, 7, 5, 1, 0, 4 } },		// 2,0,1
  { 6, { 6, 7, 3, 1, 0, 4 } },		// 2,0,2
  { 6, { 2, 6, 7, 5, 4, 0 } },		// 2,1,0
  { 4, { 6, 7, 5, 4, -1, -1 } },	// 2,1,1
  { 6, { 6, 7, 3, 1, 5, 4 } },		// 2,1,2
  { 6, { 2, 3, 7, 5, 4, 0 } },		// 2,2,0
  { 6, { 2, 3, 7, 5, 4, 6 } },		// 2,2,1
  { 6, { 6, 2, 3, 1, 5, 4 } }		// 2,2,2
};

csVector3 csBox3::GetCorner (int corner) const
{
  switch (corner)
  {
    case BOX_CORNER_xyz: return Min ();
    case BOX_CORNER_xyZ: return csVector3 (MinX (), MinY (), MaxZ ());
    case BOX_CORNER_xYz: return csVector3 (MinX (), MaxY (), MinZ ());
    case BOX_CORNER_xYZ: return csVector3 (MinX (), MaxY (), MaxZ ());
    case BOX_CORNER_Xyz: return csVector3 (MaxX (), MinY (), MinZ ());
    case BOX_CORNER_XyZ: return csVector3 (MaxX (), MinY (), MaxZ ());
    case BOX_CORNER_XYz: return csVector3 (MaxX (), MaxY (), MinZ ());
    case BOX_CORNER_XYZ: return Max ();
  }
  return csVector3 (0, 0, 0);
}

csSegment3 csBox3::GetEdge (int edge) const
{
  switch (edge)
  {
    case 0: return csSegment3 (GetCorner (0), GetCorner (1));
    case 1: return csSegment3 (GetCorner (0), GetCorner (2));
    case 2: return csSegment3 (GetCorner (0), GetCorner (4));
    case 3: return csSegment3 (GetCorner (4), GetCorner (5));
    case 4: return csSegment3 (GetCorner (4), GetCorner (6));
    case 5: return csSegment3 (GetCorner (1), GetCorner (5));
    case 6: return csSegment3 (GetCorner (1), GetCorner (3));
    case 7: return csSegment3 (GetCorner (5), GetCorner (7));
    case 8: return csSegment3 (GetCorner (3), GetCorner (7));
    case 9: return csSegment3 (GetCorner (6), GetCorner (7));
    case 10: return csSegment3 (GetCorner (2), GetCorner (3));
    case 11: return csSegment3 (GetCorner (2), GetCorner (6));
  }
  return csSegment3 ();
}

csBox2 csBox3::GetSide (int side) const
{
  switch (side)
  {
    case BOX_SIDE_x:
    case BOX_SIDE_X:
      return csBox2 (MinY (), MinZ (), MaxY (), MaxZ ());
    case BOX_SIDE_y:
    case BOX_SIDE_Y:
      return csBox2 (MinX (), MinZ (), MaxX (), MaxZ ());
    case BOX_SIDE_z:
    case BOX_SIDE_Z:
      return csBox2 (MinX (), MinY (), MaxX (), MaxY ());
  }
  return csBox2 ();
}

bool csBox3::AdjacentX (const csBox3& other) const
{
  if (ABS (other.MinX () - MaxX ()) < SMALL_EPSILON ||
      ABS (other.MaxX () - MinX ()) < SMALL_EPSILON)
  {
    if (MaxY () < other.MinY () || MinY () > other.MaxY ()) return false;
    if (MaxZ () < other.MinZ () || MinZ () > other.MaxZ ()) return false;
    return true;
  }
  return false;
}

bool csBox3::AdjacentY (const csBox3& other) const
{
  if (ABS (other.MinY () - MaxY ()) < SMALL_EPSILON ||
      ABS (other.MaxY () - MinY ()) < SMALL_EPSILON)
  {
    if (MaxX () < other.MinX () || MinX () > other.MaxX ()) return false;
    if (MaxZ () < other.MinZ () || MinZ () > other.MaxZ ()) return false;
    return true;
  }
  return false;
}

bool csBox3::AdjacentZ (const csBox3& other) const
{
  if (ABS (other.MinZ () - MaxZ ()) < SMALL_EPSILON ||
      ABS (other.MaxZ () - MinZ ()) < SMALL_EPSILON)
  {
    if (MaxX () < other.MinX () || MinX () > other.MaxX ()) return false;
    if (MaxY () < other.MinY () || MinY () > other.MaxY ()) return false;
    return true;
  }
  return false;
}

void csBox3::GetConvexOutline (const csVector3& pos,
	csVector3* ar, int& num_array) const
{
  const csVector3& bmin = Min ();
  const csVector3& bmax = Max ();
  int idx;
  // First select x part of coordinate.
  if (pos.x < bmin.x)		idx = 0*9;
  else if (pos.x > bmax.x)	idx = 2*9;
  else				idx = 1*9;
  // Then y part.
  if (pos.y < bmin.y)		idx += 0*3;
  else if (pos.y > bmax.y)	idx += 2*3;
  else				idx += 1*3;
  // Then z part.
  if (pos.z < bmin.z)		idx += 0;
  else if (pos.z > bmax.z)	idx += 2;
  else				idx += 1;

  const Outline& ol = outlines[idx];
  num_array = ol.num;
  int i;
  for (i = 0 ; i < num_array ; i++)
  {
    switch (ol.vertices[i])
    {
      case 0: ar[i].x = bmin.x; ar[i].y = bmin.y; ar[i].z = bmin.z; break;
      case 1: ar[i].x = bmin.x; ar[i].y = bmin.y; ar[i].z = bmax.z; break;
      case 2: ar[i].x = bmin.x; ar[i].y = bmax.y; ar[i].z = bmin.z; break;
      case 3: ar[i].x = bmin.x; ar[i].y = bmax.y; ar[i].z = bmax.z; break;
      case 4: ar[i].x = bmax.x; ar[i].y = bmin.y; ar[i].z = bmin.z; break;
      case 5: ar[i].x = bmax.x; ar[i].y = bmin.y; ar[i].z = bmax.z; break;
      case 6: ar[i].x = bmax.x; ar[i].y = bmax.y; ar[i].z = bmin.z; break;
      case 7: ar[i].x = bmax.x; ar[i].y = bmax.y; ar[i].z = bmax.z; break;
    }
  }
}

bool csBox3::Between (const csBox3& box1, const csBox3& box2) const
{
  // First the trival test to see if the coordinates are
  // at least within the right intervals.
  if (((maxbox.x >= box1.minbox.x && minbox.x <= box2.maxbox.x) ||
       (maxbox.x >= box2.minbox.x && minbox.x <= box1.maxbox.x)) &&
      ((maxbox.y >= box1.minbox.y && minbox.y <= box2.maxbox.y) ||
       (maxbox.y >= box2.minbox.y && minbox.y <= box1.maxbox.y)) &&
      ((maxbox.z >= box1.minbox.z && minbox.z <= box2.maxbox.z) ||
       (maxbox.z >= box2.minbox.z && minbox.z <= box1.maxbox.z)))
  {
    // @@@ Ok, let's just return true here. Maybe this test is already
    // enough? We could have used the planes as well.
    return true;
  }
  return false;
}

void csBox3::ManhattanDistance (const csBox3& other, csVector3& dist) const
{
  if (other.MinX () >= MaxX ()) dist.x = other.MinX () - MaxX ();
  else if (MinX () >= other.MaxX ()) dist.x = MinX () - other.MaxX ();
  else dist.x = 0;
  if (other.MinY () >= MaxY ()) dist.y = other.MinY () - MaxY ();
  else if (MinY () >= other.MaxY ()) dist.y = MinY () - other.MaxY ();
  else dist.y = 0;
  if (other.MinZ () >= MaxZ ()) dist.z = other.MinZ () - MaxZ ();
  else if (MinZ () >= other.MaxZ ()) dist.z = MinZ () - other.MaxZ ();
  else dist.z = 0;
}

csBox3& csBox3::operator+= (const csBox3& box)
{
  if (box.minbox.x < minbox.x) minbox.x = box.minbox.x;
  if (box.minbox.y < minbox.y) minbox.y = box.minbox.y;
  if (box.minbox.z < minbox.z) minbox.z = box.minbox.z;
  if (box.maxbox.x > maxbox.x) maxbox.x = box.maxbox.x;
  if (box.maxbox.y > maxbox.y) maxbox.y = box.maxbox.y;
  if (box.maxbox.z > maxbox.z) maxbox.z = box.maxbox.z;
  return *this;
}

csBox3& csBox3::operator+= (const csVector3& point)
{
  if (point.x < minbox.x) minbox.x = point.x;
  if (point.x > maxbox.x) maxbox.x = point.x;
  if (point.y < minbox.y) minbox.y = point.y;
  if (point.y > maxbox.y) maxbox.y = point.y;
  if (point.z < minbox.z) minbox.z = point.z;
  if (point.z > maxbox.z) maxbox.z = point.z;
  return *this;
}

csBox3& csBox3::operator*= (const csBox3& box)
{
  if (box.minbox.x > minbox.x) minbox.x = box.minbox.x;
  if (box.minbox.y > minbox.y) minbox.y = box.minbox.y;
  if (box.minbox.z > minbox.z) minbox.z = box.minbox.z;
  if (box.maxbox.x < maxbox.x) maxbox.x = box.maxbox.x;
  if (box.maxbox.y < maxbox.y) maxbox.y = box.maxbox.y;
  if (box.maxbox.z < maxbox.z) maxbox.z = box.maxbox.z;
  return *this;
}

csBox3 operator+ (const csBox3& box1, const csBox3& box2)
{
  return csBox3(
  	MIN(box1.minbox.x,box2.minbox.x),
	MIN(box1.minbox.y,box2.minbox.y),
	MIN(box1.minbox.z,box2.minbox.z),
	MAX(box1.maxbox.x,box2.maxbox.x),
	MAX(box1.maxbox.y,box2.maxbox.y),
	MAX(box1.maxbox.z,box2.maxbox.z) );
}

csBox3 operator+ (const csBox3& box, const csVector3& point)
{
  return csBox3(
  	MIN(box.minbox.x,point.x),
	MIN(box.minbox.y,point.y),
	MIN(box.minbox.z,point.z),
	MAX(box.maxbox.x,point.x),
	MAX(box.maxbox.y,point.y),
	MAX(box.maxbox.z,point.z) );
}

csBox3 operator* (const csBox3& box1, const csBox3& box2)
{
  return csBox3(
  	MAX(box1.minbox.x,box2.minbox.x),
	MAX(box1.minbox.y,box2.minbox.y),
	MAX(box1.minbox.z,box2.minbox.z),
	MIN(box1.maxbox.x,box2.maxbox.x),
	MIN(box1.maxbox.y,box2.maxbox.y),
	MIN(box1.maxbox.z,box2.maxbox.z));
}

bool operator== (const csBox3& box1, const csBox3& box2)
{
  return ( (box1.minbox.x == box2.minbox.x)
  	&& (box1.minbox.y == box2.minbox.y)
  	&& (box1.minbox.z == box2.minbox.z)
	&& (box1.maxbox.x == box2.maxbox.x)
	&& (box1.maxbox.y == box2.maxbox.y)
	&& (box1.maxbox.z == box2.maxbox.z) );
}

bool operator!= (const csBox3& box1, const csBox3& box2)
{
  return ( (box1.minbox.x != box2.minbox.x)
  	|| (box1.minbox.y != box2.minbox.y)
  	|| (box1.minbox.z != box2.minbox.z)
	|| (box1.maxbox.x != box2.maxbox.x)
	|| (box1.maxbox.y != box2.maxbox.y)
	|| (box1.maxbox.z != box2.maxbox.z) );
}

bool operator< (const csBox3& box1, const csBox3& box2)
{
  return ( (box1.minbox.x >= box2.minbox.x)
  	&& (box1.minbox.y >= box2.minbox.y)
  	&& (box1.minbox.z >= box2.minbox.z)
	&& (box1.maxbox.x <= box2.maxbox.x)
	&& (box1.maxbox.y <= box2.maxbox.y)
	&& (box1.maxbox.z <= box2.maxbox.z) );
}

bool operator> (const csBox3& box1, const csBox3& box2)
{
  return ( (box2.minbox.x >= box1.minbox.x)
  	&& (box2.minbox.y >= box1.minbox.y)
  	&& (box2.minbox.z >= box1.minbox.z)
	&& (box2.maxbox.x <= box1.maxbox.x)
	&& (box2.maxbox.y <= box1.maxbox.y)
	&& (box2.maxbox.z <= box1.maxbox.z) );
}

bool operator< (const csVector3& point, const csBox3& box)
{
  return ( (point.x >= box.minbox.x)
  	&& (point.x <= box.maxbox.x)
	&& (point.y >= box.minbox.y)
	&& (point.y <= box.maxbox.y)
	&& (point.z >= box.minbox.z)
	&& (point.z <= box.maxbox.z) );
}

//---------------------------------------------------------------------------
