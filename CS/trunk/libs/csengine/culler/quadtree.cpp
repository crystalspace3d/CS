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
#include "csgeom/frustum.h"
#include "csgeom/poly2d.h"
#include "csengine/quadtree.h"
#include "csengine/world.h"
#include "isystem.h"


bool BoxEntirelyInPolygon (csVector2* verts, int num_verts, const csBox2& bbox)
{
  return (csPoly2D::In (verts, num_verts, bbox.GetCorner (0)) &&
          csPoly2D::In (verts, num_verts, bbox.GetCorner (1)) &&
          csPoly2D::In (verts, num_verts, bbox.GetCorner (2)) &&
          csPoly2D::In (verts, num_verts, bbox.GetCorner (3)));
}


#if 0
//// dead code now...

csQuadtreeNode::csQuadtreeNode ()
{
  children = NULL;
  state = CS_QUAD_EMPTY;
}

csQuadtreeNode::~csQuadtreeNode ()
{
  delete [] children;
}

//--------------------------------------------------------------------------

void csQuadtree::Build (csQuadtreeNode* node, const csBox2& box, int depth)
{
  node->SetCenter ((box.Min () + box.Max ())/2);
  if (depth <= 0) return;
  const csVector2& center = node->GetCenter ();

  csBox2 childbox;

  node->children = new csQuadtreeNode [4];

  csQuadtreeNode* children = node->children;

  childbox.Set (box.Min (), center);
  Build (&children[0], childbox, depth-1);

  childbox.Set (center.x, box.MinY (), box.MaxX (), center.y);
  Build (&children[1], childbox, depth-1);

  childbox.Set (center, box.Max ());
  Build (&children[2], childbox, depth-1);

  childbox.Set (box.MinX (), center.y, center.x, box.MaxY ());
  Build (&children[3], childbox, depth-1);
}

csQuadtree::csQuadtree (const csBox2& box, int depth)
{
  bbox = box;
  root = new csQuadtreeNode ();
  Build (root, box, depth-1);
}

csQuadtree::~csQuadtree ()
{
  delete root;
}

bool csQuadtree::InsertPolygon (csQuadtreeNode* node,
	csVector2* verts, int num_verts,
	const csBox2& cur_bbox, const csBox2& pol_bbox)
{
  // If node is completely full already then nothing can happen.
  if (node->GetState () == CS_QUAD_FULL) return false;

  csQuadtreeNode* children = node->children;
  // If there are no children then this node is set to state partial.
  if (!children)
  {
    node->SetState (CS_QUAD_PARTIAL);
    return true;
  }

  // If there are children and we are empty then we propagate
  // the empty state to the children. This is because our
  // state is going to change so the children need to be valid.
  if (node->GetState () == CS_QUAD_EMPTY)
  {
    children[0].SetState (CS_QUAD_EMPTY);
    children[1].SetState (CS_QUAD_EMPTY);
    children[2].SetState (CS_QUAD_EMPTY);
    children[3].SetState (CS_QUAD_EMPTY);
  }

  csBox2 childbox;
  const csVector2& center = node->GetCenter ();
  bool vis, rc1, rc2, rc3, rc4;
  csVector2 v;
  rc1 = rc2 = rc3 = rc4 = false;

  // center_vis contains visibility info about the visibility
  // of the center inside the polygon.
  bool center_vis = csPoly2D::In (verts, num_verts, center);
  // Visibility information for the four central points of the sides
  // of every edge of the total area. We precompute this because
  // we're going to need this information anyway.
  v.x = cur_bbox.MinX ();
  v.y = center.y;
  bool left_vis = csPoly2D::In (verts, num_verts, v);
  v.x = cur_bbox.MaxX ();
  bool right_vis = csPoly2D::In (verts, num_verts, v);
  v.x = center.x;
  v.y = cur_bbox.MinY ();
  bool top_vis = csPoly2D::In (verts, num_verts, v);
  v.y = cur_bbox.MaxY ();
  bool bottom_vis = csPoly2D::In (verts, num_verts, v);

  // Check the bounding box of the polygon against all four
  // child areas (by comparing the bounding box against the center).
  // If the bounding box overlaps the child area then we continue
  // the check to see if the polygon also overlaps the child area.

  // Child 0 (top/left).
  if (children[0].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MinX () <= center.x && pol_bbox.MinY () <= center.y)
  {
    vis = false;
    // If any of the three corners is in polygon then polygon is visible.
    if (center_vis || left_vis || top_vis) vis = true;
    // If bbox is entirely in child area then polygon is visible.
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MaxY () <= center.y) vis = true;
    // If bbox crosses child area vertically but does not cross
    // it horizontally (both left and right) then polygon is visible.
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MinX () >= cur_bbox.MinX ()) vis = true;
    else if (pol_bbox.MaxY () <= center.y && pol_bbox.MinY () >= cur_bbox.MinY ()) vis = true;
    // Most general case: just do the intersection test for the area and the polygon.
    else vis = csBox2::Intersect (cur_bbox.Min (), center, verts, num_verts);

    if (vis)
    {
      // We already calculated wether or not three of the four corners
      // of the child area are inside the polygon. We only need
      // to test the fourth one. If all are in the polygon then
      // the node is full and we can stop recursion.
      if (center_vis && left_vis && top_vis && csPoly2D::In (verts, num_verts, cur_bbox.Min ()))
      {
        children[0].SetState (CS_QUAD_FULL);
	rc1 = true;
      }
      else
      {
        childbox.Set (cur_bbox.Min (), center);
        rc1 = InsertPolygon (&children[0], verts, num_verts, childbox, pol_bbox);
      }
    }
  }

  // Child 1 (top/right).
  if (children[1].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MaxX () > center.x && pol_bbox.MinY () <= center.y)
  {
    vis = false;
    if (center_vis || right_vis || top_vis) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MaxY () <= center.y) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MaxX () <= cur_bbox.MaxX ()) vis = true;
    else if (pol_bbox.MaxY () <= center.y && pol_bbox.MinY () >= cur_bbox.MinY ()) vis = true;
    else vis = csBox2::Intersect (center.x, cur_bbox.MinY (), cur_bbox.MaxX (), center.y, verts, num_verts);

    if (vis)
    {
      v.x = cur_bbox.MaxX ();
      v.y = cur_bbox.MinY ();
      if (center_vis && right_vis && top_vis && csPoly2D::In (verts, num_verts, v))
      {
        children[1].SetState (CS_QUAD_FULL);
	rc2 = true;
      }
      else
      {
        childbox.Set (center.x, cur_bbox.MinY (), cur_bbox.MaxX (), center.y);
        rc2 = InsertPolygon (&children[1], verts, num_verts, childbox, pol_bbox);
      }
    }
  }

  // Child 2 (bottom/right).
  if (children[2].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MaxX () > center.x && pol_bbox.MaxY () > center.y)
  {
    vis = false;
    if (center_vis || right_vis || bottom_vis) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MinY () >= center.y) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MaxX () <= cur_bbox.MaxX ()) vis = true;
    else if (pol_bbox.MinY () >= center.y && pol_bbox.MaxY () <= cur_bbox.MaxY ()) vis = true;
    else vis = csBox2::Intersect (center, cur_bbox.Max (), verts, num_verts);

    if (vis)
    {
      if (center_vis && right_vis && bottom_vis && csPoly2D::In (verts, num_verts, cur_bbox.Max ()))
      {
        children[2].SetState (CS_QUAD_FULL);
	rc3 = true;
      }
      else
      {
        childbox.Set (center, cur_bbox.Max ());
        rc3 = InsertPolygon (&children[2], verts, num_verts, childbox, pol_bbox);
      }
    }
  }

  // Child 3 (bottom/left).
  if (children[3].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MinX () <= center.x && pol_bbox.MaxY () > center.y)
  {
    vis = false;
    if (center_vis || left_vis || bottom_vis) vis = true;
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MinY () >= center.y) vis = true;
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MinX () >= cur_bbox.MinX ()) vis = true;
    else if (pol_bbox.MinY () >= center.y && pol_bbox.MaxY () <= cur_bbox.MaxY ()) vis = true;
    else vis = csBox2::Intersect (cur_bbox.MinX (), center.y, center.x, cur_bbox.MaxY (), verts, num_verts);

    if (vis)
    {
      v.x = cur_bbox.MinX ();
      v.y = cur_bbox.MaxY ();
      if (center_vis && left_vis && bottom_vis && csPoly2D::In (verts, num_verts, v))
      {
        children[3].SetState (CS_QUAD_FULL);
	rc4 = true;
      }
      else
      {
  	childbox.Set (cur_bbox.MinX (), center.y, center.x, cur_bbox.MaxY ());
        rc4 = InsertPolygon (&children[3], verts, num_verts, childbox, pol_bbox);
      }
    }
  }

  if (children[0].GetState () == CS_QUAD_FULL &&
      children[1].GetState () == CS_QUAD_FULL &&
      children[2].GetState () == CS_QUAD_FULL &&
      children[3].GetState () == CS_QUAD_FULL)
  {
    node->SetState (CS_QUAD_FULL);
    return true;
  }

  if (rc1 || rc2 || rc3 || rc4)
  {
    node->SetState (CS_QUAD_PARTIAL);
    return true;
  }
  return false;
}

bool csQuadtree::InsertPolygon (csVector2* verts, int num_verts, const csBox2& pol_bbox)
{
  // If root is already full then there is nothing that can happen further.
  if (root->GetState () == CS_QUAD_FULL) return false;

  // If the bounding box of the tree does not overlap with the bounding box of
  // the polygon then we can return false here.
  if (!bbox.Overlap (pol_bbox)) return false;

  // If bounding box of tree is completely inside bounding box of polygon then
  // it is possible that tree is completely in polygon. We test that condition
  // further.
  if (bbox < pol_bbox)
  {
    if (BoxEntirelyInPolygon (verts, num_verts, bbox))
    {
      // Polygon completely covers tree. In that case set state
      // of tree to full and return true.
      root->SetState (CS_QUAD_FULL);
      return true;
    }
  }

  return InsertPolygon (root, verts, num_verts, bbox, pol_bbox);
}

bool csQuadtree::TestPolygon (csQuadtreeNode* node,
	csVector2* verts, int num_verts,
	const csBox2& cur_bbox, const csBox2& pol_bbox)
{
  // If node is completely full already then nothing can happen.
  if (node->GetState () == CS_QUAD_FULL) return false;
  // If node is completely empty then polygon is always visible.
  if (node->GetState () == CS_QUAD_EMPTY) return true;

  csQuadtreeNode* children = node->children;
  // If there are no children then we assume polygon is not visible.
  // This is an optimization which is not entirely correct@@@
  if (!children) return false;

  csBox2 childbox;
  const csVector2& center = node->GetCenter ();
  bool vis;
  csVector2 v;

  // center_vis contains visibility info about the visibility
  // of the center inside the polygon.
  bool center_vis = csPoly2D::In (verts, num_verts, center);
  // Visibility information for the four central points of the sides
  // of every edge of the total area. We precompute this because
  // we're going to need this information anyway.
  v.x = cur_bbox.MinX ();
  v.y = center.y;
  bool left_vis = csPoly2D::In (verts, num_verts, v);
  v.x = cur_bbox.MaxX ();
  bool right_vis = csPoly2D::In (verts, num_verts, v);
  v.x = center.x;
  v.y = cur_bbox.MinY ();
  bool top_vis = csPoly2D::In (verts, num_verts, v);
  v.y = cur_bbox.MaxY ();
  bool bottom_vis = csPoly2D::In (verts, num_verts, v);

  // Check the bounding box of the polygon against all four
  // child areas (by comparing the bounding box against the center).
  // If the bounding box overlaps the child area then we continue
  // the check to see if the polygon also overlaps the child area.

  // Child 0 (top/left).
  if (children[0].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MinX () <= center.x && pol_bbox.MinY () <= center.y)
  {
    vis = false;
    // If any of the three corners is in polygon then polygon is visible.
    if (center_vis || left_vis || top_vis) vis = true;
    // If bbox is entirely in child area then polygon is visible.
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MaxY () <= center.y) vis = true;
    // If bbox crosses child area vertically but does not cross
    // it horizontally (both left and right) then polygon is visible.
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MinX () >= cur_bbox.MinX ()) vis = true;
    else if (pol_bbox.MaxY () <= center.y && pol_bbox.MinY () >= cur_bbox.MinY ()) vis = true;
    // Most general case: just do the intersection test for the area and the polygon.
    else vis = csBox2::Intersect (cur_bbox.Min (), center, verts, num_verts);

    if (vis)
    {
      // We already calculated wether or not three of the four corners
      // of the child area are inside the polygon. We only need
      // to test the fourth one. If all are in the polygon then
      // we cover the node and we can stop recursion. Polygon is
      // visible in that case becase node is not full.
      if (center_vis && left_vis && top_vis && csPoly2D::In (verts, num_verts, cur_bbox.Min ()))
      {
        return true;
      }
      else
      {
        childbox.Set (cur_bbox.Min (), center);
        if (TestPolygon (&children[0], verts, num_verts, childbox, pol_bbox)) return true;
      }
    }
  }

  // Child 1 (top/right).
  if (children[1].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MaxX () > center.x && pol_bbox.MinY () <= center.y)
  {
    vis = false;
    if (center_vis || right_vis || top_vis) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MaxY () <= center.y) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MaxX () <= cur_bbox.MaxX ()) vis = true;
    else if (pol_bbox.MaxY () <= center.y && pol_bbox.MinY () >= cur_bbox.MinY ()) vis = true;
    else vis = csBox2::Intersect (center.x, cur_bbox.MinY (), cur_bbox.MaxX (), center.y, verts, num_verts);

    if (vis)
    {
      v.x = cur_bbox.MaxX ();
      v.y = cur_bbox.MinY ();
      if (center_vis && right_vis && top_vis && csPoly2D::In (verts, num_verts, v))
      {
        return true;
      }
      else
      {
        childbox.Set (center.x, cur_bbox.MinY (), cur_bbox.MaxX (), center.y);
        if (TestPolygon (&children[1], verts, num_verts, childbox, pol_bbox)) return true;
      }
    }
  }

  // Child 2 (bottom/right).
  if (children[2].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MaxX () > center.x && pol_bbox.MaxY () > center.y)
  {
    vis = false;
    if (center_vis || right_vis || bottom_vis) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MinY () >= center.y) vis = true;
    else if (pol_bbox.MinX () >= center.x && pol_bbox.MaxX () <= cur_bbox.MaxX ()) vis = true;
    else if (pol_bbox.MinY () >= center.y && pol_bbox.MaxY () <= cur_bbox.MaxY ()) vis = true;
    else vis = csBox2::Intersect (center, cur_bbox.Max (), verts, num_verts);

    if (vis)
    {
      if (center_vis && right_vis && bottom_vis && csPoly2D::In (verts, num_verts, cur_bbox.Max ()))
      {
        return true;
      }
      else
      {
        childbox.Set (center, cur_bbox.Max ());
        if (TestPolygon (&children[2], verts, num_verts, childbox, pol_bbox)) return true;
      }
    }
  }

  // Child 3 (bottom/left).
  if (children[3].GetState () != CS_QUAD_FULL &&
  	pol_bbox.MinX () <= center.x && pol_bbox.MaxY () > center.y)
  {
    vis = false;
    if (center_vis || left_vis || bottom_vis) vis = true;
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MinY () >= center.y) vis = true;
    else if (pol_bbox.MaxX () <= center.x && pol_bbox.MinX () >= cur_bbox.MinX ()) vis = true;
    else if (pol_bbox.MinY () >= center.y && pol_bbox.MaxY () <= cur_bbox.MaxY ()) vis = true;
    else vis = csBox2::Intersect (cur_bbox.MinX (), center.y, center.x, cur_bbox.MaxY (), verts, num_verts);

    if (vis)
    {
      v.x = cur_bbox.MinX ();
      v.y = cur_bbox.MaxY ();
      if (center_vis && left_vis && bottom_vis && csPoly2D::In (verts, num_verts, v))
      {
        return true;
      }
      else
      {
  	childbox.Set (cur_bbox.MinX (), center.y, center.x, cur_bbox.MaxY ());
        if (TestPolygon (&children[3], verts, num_verts, childbox, pol_bbox)) return true;
      }
    }
  }

  return false;
}

bool csQuadtree::TestPolygon (csVector2* verts, int num_verts, const csBox2& pol_bbox)
{
  // If root is already full then there is nothing that can happen further.
  if (root->GetState () == CS_QUAD_FULL) return false;

  // If the bounding box of the tree does not overlap with the bounding box of
  // the polygon then we can return false here.
  if (!bbox.Overlap (pol_bbox)) return false;

  // If bounding box of tree is completely inside bounding box of polygon then
  // it is possible that tree is completely in polygon. We test that condition
  // further.
  if (bbox < pol_bbox)
  {
    if (BoxEntirelyInPolygon (verts, num_verts, bbox))
    {
      // Polygon completely covers tree. In that case return
      // true because polygon will be visible (node is not full).
      return true;
    }
  }

  return TestPolygon (root, verts, num_verts, bbox, pol_bbox);
}

int csQuadtree::TestPoint (csQuadtreeNode* node, const csVector2& point)
{
  csQuadtreeNode* children = node->children;
  if (!children) return node->GetState ();

  const csVector2& center = node->GetCenter ();
  if (point.x <= center.x)
  {
    if (point.y <= center.y)
    {
      if (children[0].GetState () != CS_QUAD_PARTIAL) return children[0].GetState ();
      return TestPoint (&children[0], point);
    }
    else
    {
      if (children[3].GetState () != CS_QUAD_PARTIAL) return children[3].GetState ();
      return TestPoint (&children[3], point);
    }
  }
  else
  {
    if (point.y <= center.y)
    {
      if (children[1].GetState () != CS_QUAD_PARTIAL) return children[1].GetState ();
      return TestPoint (&children[1], point);
    }
    else
    {
      if (children[2].GetState () != CS_QUAD_PARTIAL) return children[2].GetState ();
      return TestPoint (&children[2], point);
    }
  }
  return CS_QUAD_EMPTY;
}


int csQuadtree::TestPoint (const csVector2& point)
{
  if (!bbox.In (point)) return CS_QUAD_UNKNOWN;
  if (root->GetState () != CS_QUAD_PARTIAL) return root->GetState ();
  return TestPoint (root, point);
}

#endif

/*----------------------------------------------------------------*/
/* Wouter's Wild QuadTree implementation */

#if 0

/// computes 2**x
static int Pow2(int x)
{
  int res = 1;
  for(int i=0; i<x; i++)
    res <<= 1;
  return res;
}

csQuadTree :: csQuadTree (const csBox2& the_box, int the_depth)
{
  bbox = the_box;
  max_depth= the_depth;
  root_state = CS_QUAD_EMPTY;
  if(depth < 1)
  {
    CsPrintf(MSG_FATAL_ERROR, "QuadTree: Depth too small\n");
    exit(1);
  }
  /// first calculate the number of nodes.
  /// each depth 4* the nodes at the previous depth are added.
  /// depth 1 has the root node.
  int nr_leaves = 1;
  int nr_nodes = 0;
  for(int i=1; i<=max_depth; i++)
  {
    nr_nodes += nr_leaves;
    nr_leaves *= 4;
  }
  /// 4 nodes per byte, the root node is stored seperately
  state_size = (nr_nodes - 1) / 4;
  CsPrintf(MSG_STDOUT, "QuadTree: depth %d, nodes %d, statesize %d bytes\n", 
    max_depth, nr_nodes, state_size);

  if(state_size > 0)
  {
    states = new unsigned char[state_size];
    /// and every node is EMPTY at the start
    memset(states, CS_QUAD_ALL_EMPTY, state_size);  
  }
  else states = NULL;
}


csQuadTree :: ~csQuadTree ()
{
  delete[] states;
}


void csQuadTree :: CallChildren(int (csQuadTree::*func)(
    const csBox2& node_bbox, int node_depth, int node_state, int offset, 
    int node_nr, void* data), 
  const csBox2& box, int depth, int offset, int node_nr, void *data)
{
  int retval[4];
  CallChildren(func, box, depth, offset, node_nr, data, retval);
}

void csQuadTree :: CallChildren(int (csQuadTree::*func)(
    const csBox2& node_bbox, int node_depth, int node_state, int offset, 
    int node_nr, void* data), 
  const csBox2& box, int depth, int offset, int node_nr, 
  void *data, int retval[4])
{
  if(depth >= max_depth)
  {
    CsPrintf(MSG_INTERNAL_ERROR, "QuadTree: leaf trying to recurse.\n");
    return;
  }
  // call func for each child of parent node with (offset, node_nr)
  csBox2 childbox;
  csVector2 center = box.GetCenter();
  int childstate, childoffset, childnr;

  /*
   * states are ordered like this:
   * root has children in byte 0.
   * nodes in byte 0 have children in byte 0+node_nr+1(1,2,3,4).
   * nodes in byte 1 have children in byte 4+node_nr+1.
   * nodes in byte n have children in byte 4*n+node_nr+1
   * So for byte n, take 4*n + node_nr+1 as the new byte
   * that new byte has the states of it's four children.
  */
  if(offset == -1)
    childoffset = 0; // root node's children
  else childoffset = 4 * offset + node_nr + 1;
  for(childnr=0; childnr<4; childnr++)
  {
    // compute new bounding box.
    switch(childnr)
    {
      case 0 /*topleft*/ : 
        childbox.Set(box.Min(), center); break;
      case 1 /*topright*/ : 
        childbox.Set(center.x, box.MinY(), box.MaxX(), center.y); break;
      case 2 /*bottomright*/ : 
        childbox.Set(center, box.Max()); break;
      case 3 /*bottomleft*/ : 
        childbox.Set(box.MinX(), center.y, center.x, box.MaxY()); break;
      default: CsPrintf(MSG_FATAL_ERROR, "QuadTree: Unknown child\n");
    }
    childstate = GetNodeState(childoffset, childnr);
    retval[childnr] = func(childbox, depth+1, childstate, childoffset, 
      childnr, data);
  }
}

/// masks and shifts for GetNodeState and SetNodeState
static const int node_masks[4] = {0xC0, 0x30, 0x0C, 0x03};
static const int node_shifts[4] = {6, 4, 2, 0};

int csQuadTree :: GetNodeState(int offset, int nodenr)
{
  if(offset > state_size)
  {
    CsPrintf(MSG_INTERNAL_ERROR, "QuadTree: state out of range\n");
    return 0;
  }
  if(offset == -1)
    return root_state;
  unsigned char bits = states[offset];
  bits &= node_masks[nodenr];
  bits >>= node_shifts[nodenr];
  return bits;
}


void csQuadTree :: SetNodeState(int offset, int nodenr, int newstate)
{
  if(offset > state_size)
  {
    CsPrintf(MSG_INTERNAL_ERROR, "QuadTree: setstate out of range\n");
    return;
  }
  if(offset == -1)
  {
    root_state = newstate;
    return;
  }
  unsigned char bits = states[offset];     // get all bits
  bits &= ~node_masks[nodenr];             // purge this node's bits
  bits |= newstate << node_shifts[nodenr]; // insert new bits
  states[offset] = bits;                   // store bits
}


void csQuadTree :: MakeEmpty()
{
  root_state = CS_QUAD_EMPTY;
  if(states)
    memset(states, CS_QUAD_ALL_EMPTY, state_size);  
}


int csQuadTree :: mark_node_func (const csBox2& node_bbox,
  int node_depth, int node_state, int offset, int node_nr, void* data)
{
  SetNodeState(offset, node_nr, (int)data);
}

int csQuadTree :: insert_polygon_func (const csBox2& node_bbox,
  int node_depth, int node_state, int offset, int node_nr, void* data)
{
  struct insert_poly_info info& = *(struct insert_poly_info*)data;
  if(node_state == CS_QUAD_UNKNOWN)
  {
    CsPrintf(MSG_INTERNAL_ERROR, "QuadTree: insertpoly quad_unknown.\n");
    return false;
  }
  if(node_state == CS_QUAD_FULL) // full already, skip it.
    return false;

  /// perhaps none of the node is covered?
  if (!node_bbox.Overlap (info.pol_bbox)) return false;

  /// So, the polygon bbox overlaps this node. How much?

  /// is the whole node covered?
  /// first check bounding boxes then precisely.
  if(info.pol_bbox.Contains(node_bbox) &&
    BoxEntirelyInPolygon(info.verts, info.num_verts, node_bbox))
  {
    SetNodeState(offset, node_nr, CS_QUAD_FULL);
    /// mark children (if any) as unknown, since they should not be reached.
    if(node_depth < max_depth)
      CallChildren(mark_node_func, node_bbox, node_depth, node_offset, 
        node_nr, (void*)CS_QUAD_UNKNOWN);
    return true;
  }
  
  /// So a part of the node may be covered, perhaps none.
  /// could test some more points here.
  if( csPoly2D::In(info.verts, info.num_verts, node_bbox->GetCenter())
    || node_bbox.Intersect(info.verts, info.num_verts))
  { 
    // so it overlaps a bit.
    if(node_state == CS_QUAD_EMPTY && node_depth < max_depth)
    {
      // mark children as empty now, since they should be empty, and
      // can be reached now...
      CallChildren(mark_node_func, node_bbox, node_depth, node_offset, 
        node_nr, (void*)CS_QUAD_EMPTY);
    }
    // this node is partially covered.
    SetNodeState(offset, node_nr, CS_QUAD_PARTIAL);
    // if any children they can process the polygon too.
    if(node_depth < max_depth)
      CallChildren(insert_polygon_func, node_bbox, node_depth, node_offset, 
        node_nr, data);
    return true; /// @@@ This could conceivably be false, as PARTIAL->PARTIAL
  }
  /// polygon bound overlaps, but polygon itself does not intersect us
  /// i.e. the polygon is not in this node. No change, nothing added here.
  return false;
}


bool csQuadTree :: InsertPolygon (csVector2* verts, int num_verts,
  const csBox2& pol_bbox)
{
  struct insert_poly_info info;
  info.verts = verts;
  info.num_verts = num_verts;
  info.pol_bbox = pol_bbox;
  return insert_polygon_func(bbox, 1, root_state, -1, 0, (void*)&info);
}


bool csQuadTree :: TestPolygon (csVector2* verts, int num_verts,
  const csBox2& pol_bbox)
{
}


int csQuadTree :: GetTestPointResult(int retval[4])
{
  /// returns UNKNOWN if all retval are UNKNOWN.
  /// returns PARTIAL if at least one is PARTIAL, or
  ///   both FULL and EMPTY are present.
  /// returns FULL when all non-unknown values are FULL
  /// returns EMPTY when all non-unknown values are EMPTY
  int res = CS_QUAD_UNKNOWN;
  for(int i=0; i<4; i++)
    if(retval[i] != CS_QUAD_UNKNOWN)
    {
      if(res == CS_QUAD_UNKNOWN)
        res = retval[i];
      else if(res == CS_QUAD_PARTIAL || retval[i] == CS_QUAD_PARTIAL)
        res = CS_QUAD_PARTIAL;
      else // res must be EMPTY or FULL now. so must retval[i].
        if(res != retval[i])
          res = CS_QUAD_PARTIAL;
    }
  return res;
}


int csQuadTree :: test_point_func (const csBox2& node_bbox,
  int node_depth, int node_state, int offset, int node_nr, void* data)
{
  if(node_state == CS_QUAD_UNKNOWN)
  {
    CsPrintf(MSG_INTERNAL_ERROR, "QuadTree: testpoint quad_unknown.\n");
    return CS_QUAD_UNKNOWN;
  }
  if(!node_bbox.In(*(csVector2*)data))
    return CS_QUAD_UNKNOWN;
  if(node_state != CS_QUAD_PARTIAL || node_depth == max_depth)
    return node_state;
  // for a partial covered node with children, call the children
  int retval[4];
  CallChildren(test_point_func, node_bbox, node_depth, node_offset, node_nr,
    data, retval);
  return GetTestPointResult(retval);
}


int csQuadTree :: TestPoint (const csVector2& point)
{
  return test_point_func(bbox, 1, root_state, -1, 0, (void*)&point);
}

#endif
