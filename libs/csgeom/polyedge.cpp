/*
    Copyright (C) 2000 by Jorrit Tyberghein

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
#include "csgeom/polyedge.h"
#include "csgeom/polyclip.h"

csPoly2DEdges::csPoly2DEdges (int start_size)
{
  max_edges = start_size;
  CHK (edges = new csEdge [max_edges]);
  MakeEmpty ();
}

csPoly2DEdges::csPoly2DEdges (csPoly2DEdges& copy)
{
  max_edges = copy.max_edges;
  CHK (edges = new csEdge [max_edges]);
  num_edges = copy.num_edges;
  memcpy (edges, copy.edges, sizeof (csEdge)*num_edges);
}

csPoly2DEdges::~csPoly2DEdges ()
{
  CHK (delete [] edges);
}

void csPoly2DEdges::MakeEmpty ()
{
  num_edges = 0;
}

bool csPoly2DEdges::In (const csVector2& v)
{
  int i;
  for (i = 0 ; i < num_edges ; i++)
  {
    if (csMath2::WhichSide2D (v, edges[i].v1, edges[i].v2) < 0) return false;
  }
  return true;
}

bool csPoly2DEdges::In (csEdge* poly, int num_edge, const csVector2& v)
{
  int i;
  for (i = 0 ; i < num_edge ; i++)
  {
    if (csMath2::WhichSide2D (v, poly[i].v1, poly[i].v2) < 0) return false;
  }
  return true;
}

void csPoly2DEdges::MakeRoom (int new_max)
{
  if (new_max <= max_edges) return;
  CHK (csEdge* new_edges = new csEdge [new_max]);
  memcpy (new_edges, edges, num_edges*sizeof (csEdge));
  CHK (delete [] edges);
  edges = new_edges;
  max_edges = new_max;
}

int csPoly2DEdges::AddEdge (const csVector2& v1, const csVector2& v2)
{
  if (num_edges >= max_edges)
    MakeRoom (max_edges+5);
  edges[num_edges].v1 = v1;
  edges[num_edges].v2 = v2;
  num_edges++;
  return num_edges-1;
}

#define ONPLANE(c) ((c) > -EPSILON && (c) < EPSILON)
#define ATLEFT(c) ((c) <= -EPSILON)
#define ATLEFTORPLANE(c) ((c) < EPSILON)
#define ATRIGHT(c) ((c) >= EPSILON)
#define ATRIGHTORPLANE(c) ((c) > -EPSILON)

void csPoly2DEdges::Intersect (const csPlane2& plane,
	csPoly2DEdges* left, csPoly2DEdges* right)
{
  int i;
  float c1, c2;
  csVector2 isect;
  float dist;

  // If 0 then don't know, else if 1 then prefer right,
  // else prefer left. This is used for trying to put edges
  // that coincide with the splitter plane on the prefered
  // side (i.e. a side that contains other edges).
  int preferred_dir = 0;

  // If skip is > 0 then we skipped the 'skip' edges because
  // we didn't have enough information for putting them
  // on the 'right' polygon.
  int skip = 0;

  left->SetNumEdges (0);
  right->SetNumEdges (0);

  for (i = 0 ; i < num_edges ; i++)
  {
    c1 = plane.Classify (edges[i].v1);
    c2 = plane.Classify (edges[i].v2);

    if (ONPLANE (c1))
    {
      if (ONPLANE (c2))
      {
        // Both vertices are on the plane. In this case
	// we add the edge to the preferred side. If we don't
	// have a preferred side yet then we skip it.
	if (preferred_dir == 0)
	  skip++;
	else if (preferred_dir < 0)
	  left->AddEdge (edges[i]);
	else
	  right->AddEdge (edges[i]);
      }
      else if (ATLEFT(c2))
      {
        left->AddEdge (edges[i]);
	preferred_dir = -1;
      }
      else
      {
        right->AddEdge (edges[i]);
	preferred_dir = 1;
      }
    }
    else if (ATLEFT (c1))
    {
      if (ATLEFTORPLANE (c2))
      {
        left->AddEdge (edges[i]);
        preferred_dir = -1;
      }
      else
      {
        csIntersect2::Plane (edges[i].v1, edges[i].v2,
      	  plane, isect, dist);
	left->AddEdge (edges[i].v1, isect);
	right->AddEdge (isect, edges[i].v2);
        preferred_dir = 1;
      }
    }
    else // ATRIGHT (c1)
    {
      if (ATRIGHTORPLANE (c2))
      {
        right->AddEdge (edges[i]);
	preferred_dir = 1;
      }
      else
      {
        csIntersect2::Plane (edges[i].v1, edges[i].v2,
      	  plane, isect, dist);
	right->AddEdge (edges[i].v1, isect);
	left->AddEdge (isect, edges[i].v2);
        preferred_dir = -1;
      }
    }
  }

  // If skip > 0 then there are a number of edges in
  // the beginning that we ignored. These edges are all coplanar
  // with 'plane'. We will add them to the preferred side.
  i = 0;
  while (skip > 0)
  {
    if (preferred_dir == -1)
      left->AddEdge (edges[i]);
    else
      right->AddEdge (edges[i]);
    i++;
    skip--;
  }
}

//---------------------------------------------------------------------------
